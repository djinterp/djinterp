/*******************************************************************************
* djinterp [parse]                                          arg_option_tests.cpp
*
* The argument parser's C++ option suite's sections.
*   The sections held to CLI11 run what the CASES section defines: options
* made, set and read through the face's handle, and parsed, as a program
* would make them with CLI11's Option*. tools/cppref/cppref.py compiles
* that section against CLI11 2.7.2, through the face's names for CLI11's
* (cli11_shim.hpp), and the transcripts below are what CLI11 wrote; so the
* CASES section is written in the part of the face CLI11 shares -- `->` on
* every handle, no status taken from a call -- and C++98 unless a guard
* says otherwise. The sections that are the module's own are written apart,
* after it, each saying why CLI11 cannot write its transcript.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_option_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_option_tests.hpp"  // corresponding header
// std
#include <algorithm>  // std::sort
#include <cstddef>    // std::size_t
#include <map>        // std::map
#include <sstream>    // std::ostringstream
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/djinterp.hpp"  // framework root
#include "../../../../../inc/djinterp/parse/cli/arg_parser/arg_parser.hpp"  // the face
#include "./arg_tests_support.hpp"  // run, case tables, transcripts
#include "../../c/tests_section.h"  // d_tests_section


namespace
{

namespace cli = ::djinterp::parse::cli;

using cli::testing::run;
using cli::testing::case_entry;
using cli::testing::step_fn;
using cli::testing::define_fn;
using cli::testing::file_entry;
using cli::testing::variable_entry;
using cli::testing::escape;
using cli::testing::render;
using cli::testing::number;
using cli::testing::transcribe;
using cli::testing::scratch_directory;
using cli::testing::set_variable;
using cli::testing::cases_hold;
using cli::testing::steps_hold;
using cli::testing::text_holds;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SETTINGS
//   constant: what the steps of d_tests_arg_cpp_option_settings write, as
// CLI11 writes them.
const char SETTINGS[] =
    "bare switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "bare counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "bare text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "set switches: required=1 configurable=0 extra_args=1 trigger=1 force=1 "
    "for_default=1 inject=1 capture=1 no_override=1 case=1 underscore=1\n"
    "set counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "set text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "cleared switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "cleared counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "cleared text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "mandatory switches: required=1 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "mandatory counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "mandatory text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "take_last switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "take_last counts: policy=TakeLast priority=Normal delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "take_last text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "take_first switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "take_first counts: policy=TakeFirst priority=Normal delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "take_first text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "take_all switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "take_all counts: policy=TakeAll priority=Normal delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "take_all text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "join switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "join counts: policy=Join priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "join text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "join_semicolon switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "join_semicolon counts: policy=Join priority=Normal delimiter=59 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "join_semicolon text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "sum switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "sum counts: policy=Sum priority=Normal delimiter=44 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "sum text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "reverse switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "reverse counts: policy=Reverse priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "reverse text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "throw switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "throw counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "throw text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "first_pre_help switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "first_pre_help counts: policy=Throw priority=FirstPreHelp delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "first_pre_help text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "pre_requirements switches: required=0 configurable=1 extra_args=0 "
    "trigger=0 force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "pre_requirements counts: policy=Throw priority=PreRequirementsCheck "
    "delimiter=0 expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "pre_requirements text: description=[] has=0 option_text=[] "
    "group=[OPTIONS] type=[] env=[] default=[]\n"
    "last switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "last counts: policy=Throw priority=Last delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "last text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "normal switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "normal counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "normal text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "expected_3 switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "expected_3 counts: policy=Throw priority=Normal delimiter=0 "
    "expected=3,3,3 type_size=1,1,1 items=3,3,3\n"
    "expected_3 text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "expected_1_4 switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "expected_1_4 counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,4 type_size=1,1,1 items=1,1,4\n"
    "expected_1_4 text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "expected_unlimited switches: required=0 configurable=1 extra_args=1 "
    "trigger=0 force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "expected_unlimited counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,4 type_size=1,1,1 items=1,1,4\n"
    "expected_unlimited text: description=[] has=0 option_text=[] "
    "group=[OPTIONS] type=[] env=[] default=[]\n"
    "type_size_2 switches: required=0 configurable=1 extra_args=1 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "type_size_2 counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,4 type_size=2,2,2 items=2,2,8\n"
    "type_size_2 text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "type_size_1_3 switches: required=0 configurable=1 extra_args=1 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "type_size_1_3 counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,4 type_size=1,1,3 items=1,1,12\n"
    "type_size_1_3 text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "expected_0_1 switches: required=0 configurable=1 extra_args=1 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "expected_0_1 counts: policy=Throw priority=Normal delimiter=0 "
    "expected=0,0,1 type_size=1,1,3 items=0,0,3\n"
    "expected_0_1 text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "described switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "described counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "described text: description=[first description] has=1 option_text=[] "
    "group=[OPTIONS] type=[] env=[] default=[]\n"
    "set switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "set counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "set text: description=[second description] has=1 option_text=[TEXT ...] "
    "group=[Values] type=[VALUE] env=[VALUES_ENV] default=[]\n"
    "emptied switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "emptied counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "emptied text: description=[] has=0 option_text=[TEXT ...] group=[] "
    "type=[VALUE] env=[VALUES_ENV] default=[]\n"
    "made switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "made counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "made text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[MADE] env=[] default=[]\n"
    "bound switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=1 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "bound counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "bound text: description=[bound] has=1 option_text=[] group=[OPTIONS] "
    "type=[INT] env=[] default=[]\n";

// NAMES
//   constant: what the steps of d_tests_arg_cpp_option_names write, as CLI11
// writes them.
const char NAMES[] =
    "[-a]\n"
    "name=-a pos= all=-a both=-a bare=-a single=a l= s=a f= positional=0 "
    "nonpositional=1\n"
    "[--alpha]\n"
    "name=--alpha pos= all=--alpha both=--alpha bare=--alpha single=alpha "
    "l=alpha s= f= positional=0 nonpositional=1\n"
    "[-a,--alpha]\n"
    "name=--alpha pos= all=-a,--alpha both=-a,--alpha bare=-a,--alpha "
    "single=alpha l=alpha s=a f= positional=0 nonpositional=1\n"
    "[--alpha,-a,--beta,-b]\n"
    "name=--alpha pos= all=-a,-b,--alpha,--beta both=-a,-b,--alpha,--beta "
    "bare=-a,-b,--alpha,--beta single=alpha l=alpha,beta s=a,b f= positional=0 "
    "nonpositional=1\n"
    "[pos]\n"
    "name=pos pos=pos all=pos both=pos bare=pos single=pos l= s= f= "
    "positional=1 nonpositional=0\n"
    "[pos,-p]\n"
    "name=-p pos=pos all=-p both=pos,-p bare=-p single=p l= s=p f= "
    "positional=1 nonpositional=1\n"
    "[-p,pos,--place]\n"
    "name=--place pos=pos all=-p,--place both=pos,-p,--place bare=-p,--place "
    "single=place l=place s=p f= positional=1 nonpositional=1\n"
    "[ -c , --charlie ]\n"
    "name=--charlie pos= all=-c,--charlie both=-c,--charlie bare=-c,--charlie "
    "single=charlie l=charlie s=c f= positional=0 nonpositional=1\n"
    "[-d,-d]\n"
    "name=-d pos= all=-d,-d both=-d,-d bare=-d,-d single=d l= s=d,d f= "
    "positional=0 nonpositional=1\n"
    "[--echo,--ECHO]\n"
    "name=--echo pos= all=--echo,--ECHO both=--echo,--ECHO bare=--echo,--ECHO "
    "single=echo l=echo,ECHO s= f= positional=0 nonpositional=1\n"
    "[-v,--verbose]\n"
    "name=--verbose pos= all=-v,--verbose both=-v,--verbose bare=-v,--verbose "
    "single=verbose l=verbose s=v f= positional=0 nonpositional=1\n"
    "[--verbose{2}]\n"
    "name=--verbose pos= all=--verbose{2} both=--verbose{2} bare=--verbose "
    "single=verbose l=verbose s= f=verbose positional=0 nonpositional=1\n"
    "[-v{3},--verbose]\n"
    "name=--verbose pos= all=-v{3},--verbose both=-v{3},--verbose "
    "bare=-v,--verbose single=verbose l=verbose s=v f=v positional=0 "
    "nonpositional=1\n"
    "[!--no-color,--color]\n"
    "name=--no-color pos= all=--no-color{false},--color "
    "both=--no-color{false},--color bare=--no-color,--color single=no-color "
    "l=no-color,color s= f=no-color positional=0 nonpositional=1\n"
    "[--color,!--no-color{x}]\n"
    "name=--color pos= all=--color,--no-color{x} both=--color,--no-color{x} "
    "bare=--color,--no-color single=color l=color,no-color s= f=no-color "
    "positional=0 nonpositional=1\n"
    "[-q{false},--quiet]\n"
    "name=--quiet pos= all=-q{false},--quiet both=-q{false},--quiet "
    "bare=-q,--quiet single=quiet l=quiet s=q f=q positional=0 "
    "nonpositional=1\n"
    "setting 0: -a=1000 a=0100 --alpha_beta=1000 alpha_beta=0010 "
    "ALPHA_BETA=0000 alphabeta=0000 --alphabeta=0000 pos=1000 POS=0000 "
    "no-x=0000 --no-x=0000 x=0000\n"
    "setting 1: -a=1000 a=0100 --alpha_beta=1000 alpha_beta=0010 "
    "ALPHA_BETA=0010 alphabeta=0000 --alphabeta=0000 pos=1000 POS=1000 "
    "no-x=0000 --no-x=0000 x=0000\n"
    "setting 2: -a=1000 a=0100 --alpha_beta=1000 alpha_beta=0010 "
    "ALPHA_BETA=0010 alphabeta=0010 --alphabeta=1000 pos=1000 POS=1000 "
    "no-x=0000 --no-x=0000 x=0000\n"
    "setting 3: -a=1000 a=0100 --alpha_beta=1000 alpha_beta=0010 "
    "ALPHA_BETA=0000 alphabeta=0000 --alphabeta=0000 pos=0000 POS=0000 "
    "no-x=0011 --no-x=1000 x=0000\n"
    "setting 4: -a=1000 a=0100 --alpha_beta=1000 alpha_beta=0010 "
    "ALPHA_BETA=0010 alphabeta=0000 --alphabeta=0000 pos=0000 POS=0000 "
    "no-x=0011 --no-x=1000 x=0000\n"
    "setting 5: -a=1000 a=0100 --alpha_beta=1000 alpha_beta=0010 "
    "ALPHA_BETA=0010 alphabeta=0010 --alphabeta=1000 pos=0000 POS=0000 "
    "no-x=0011 --no-x=1000 x=0000\n";

// DEFAULTS
//   constant: what the steps of d_tests_arg_cpp_option_defaults write, as
// CLI11 writes them.
const char DEFAULTS[] =
    "default_str switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "default_str counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "default_str text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[plain]\n"
    "text count=0 empty=1 results=[] reduced=[] run=0\n"
    "count count=0 empty=1 results=[] reduced=[] run=0\n"
    "list count=0 empty=1 results=[] reduced=[] run=0\n"
    "variables: count=5 ratio=2.5 name=joe values=2 "
    "defaults=[5|2.5|joe|[1,2]]\n"
    "captured switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=1 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "captured counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "captured text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[INT] env=[] default=[3]\n"
    "function_unused switches: required=0 configurable=1 extra_args=0 "
    "trigger=0 force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "function_unused counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "function_unused text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[]\n"
    "function switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=0 inject=0 capture=0 no_override=0 case=0 "
    "underscore=0\n"
    "function counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "function text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[] env=[] default=[computed]\n"
    "always switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=1 inject=0 capture=1 no_override=0 case=0 underscore=0\n"
    "always counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "always text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[INT] env=[] default=[4]\n"
    "always_text switches: required=0 configurable=1 extra_args=0 trigger=0 "
    "force=0 for_default=1 inject=0 capture=1 no_override=0 case=0 "
    "underscore=0\n"
    "always_text counts: policy=Throw priority=Normal delimiter=0 "
    "expected=1,1,1 type_size=1,1,1 items=1,1,1\n"
    "always_text text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[TEXT] env=[] default=[word]\n"
    "within switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "within counts: policy=Throw priority=Normal delimiter=0 expected=1,1,1 "
    "type_size=1,1,1 items=1,1,1\n"
    "within text: description=[] has=0 option_text=[] group=[OPTIONS] "
    "type=[:INT in [1 - 10]] env=[] default=[7]\n"
    "THROWN ValidationError: --checked: Value 11 not in range [1 - 10]\n";

// RELATIONS
//   constant: what the steps of d_tests_arg_cpp_option_relations write, as
// CLI11 writes them.
const char RELATIONS[] =
    "first needs=[--second] excludes=[--third]\n"
    "second needs=[] excludes=[]\n"
    "third needs=[] excludes=[--first]\n"
    "ERR RequiresError 107: --first requires --second\n"
    "cmd app n=1 miss=[] subs=[] order=[--first]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=0 res=[]\n"
    "opt third n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[--first|--second]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=1 res=[2]\n"
    "opt third n=0 res=[]\n"
    "ERR RequiresError 107: --first requires --second\n"
    "cmd app n=1 miss=[] subs=[] order=[--first|--third]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=0 res=[]\n"
    "opt third n=1 res=[3]\n"
    "removed: needs=1 again=0 excludes=1\n"
    "first needs=[] excludes=[]\n"
    "third needs=[] excludes=[--first]\n"
    "ERR ExcludesError 108: --third excludes --first\n"
    "cmd app n=1 miss=[] subs=[] order=[--first|--third]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=0 res=[]\n"
    "opt third n=1 res=[3]\n"
    "third needs=[] excludes=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[--first|--third]\n"
    "opt first n=1 res=[1]\n"
    "opt second n=0 res=[]\n"
    "opt third n=1 res=[3]\n"
    "THROWN IncorrectConstruction: Option --missing is not defined\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// SEVERAL
//   constant: what the steps of d_tests_arg_cpp_option_relations_several
// write, as CLI11 writes them.
const char SEVERAL[] =
    "fourth needs=[--first,--second,--third] excludes=[]\n"
    "fifth needs=[] excludes=[--first,--second]\n"
    "first needs=[] excludes=[--fifth]\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// VALIDATORS
//   constant: what the steps of d_tests_arg_cpp_option_validators write, as
// CLI11 writes them.
const char VALIDATORS[] =
    "type=[:INT in [1 - 10]:POSITIVE]\n"
    "by name: INT in [1 - 10]\n"
    "by index: POSITIVE\n"
    "cmd app n=1 miss=[] subs=[] order=[--number]\n"
    "opt number n=1 res=[5]\n"
    "ERR ValidationError 105: --number: Value 11 not in range [1 - 10]\n"
    "cmd app n=1 miss=[] subs=[] order=[--number]\n"
    "opt number n=1 res=[11]\n"
    "cmd app n=1 miss=[] subs=[] order=[--number]\n"
    "opt number n=1 res=[11]\n"
    "ERR ValidationError 105: --number: Value 11 not in range [1 - 10]\n"
    "cmd app n=1 miss=[] subs=[] order=[--number]\n"
    "opt number n=1 res=[11]\n"
    "types=[:TWICE:EVEN|:EVEN]\n"
    "cmd app n=1 miss=[] subs=[] order=[--word|--plain]\n"
    "opt word n=1 res=[abab]\n"
    "opt plain n=1 res=[cd]\n"
    "ERR ValidationError 105: --plain: odd length\n"
    "cmd app n=1 miss=[] subs=[] order=[--word|--plain]\n"
    "opt word n=1 res=[abcabc]\n"
    "opt plain n=1 res=[cde]\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// VALIDATORS_MAPPED
//   constant: what the steps of d_tests_arg_cpp_option_validators_mapped
// write, as CLI11 writes them.
const char VALIDATORS_MAPPED[] =
    "types=[:MAPPED]\n"
    "cmd app n=1 miss=[] subs=[] order=[--mapped]\n"
    "opt mapped n=1 res=[1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--mapped]\n"
    "opt mapped n=1 res=[three]\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// EACH
//   constant: what the steps of d_tests_arg_cpp_option_each write, as CLI11
// writes them.
const char EACH[] =
    "each a\n"
    "each b\n"
    "each x\n"
    "each p\n"
    "each q\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--list|--list|--one|--joined|--joined]\n"
    "opt list n=2 res=[a|b]\n"
    "opt one n=1 res=[x]\n"
    "opt joined n=2 res=[p|q]\n"
    "opt checked n=0 res=[]\n"
    "ERR ValidationError 105: --checked: Value 7 not in range [1 - 5]\n"
    "cmd app n=1 miss=[] subs=[] order=[--checked]\n"
    "opt list n=0 res=[]\n"
    "opt one n=0 res=[]\n"
    "opt joined n=0 res=[]\n"
    "opt checked n=1 res=[7]\n";

// RESULTS
//   constant: what the steps of d_tests_arg_cpp_option_results write, as
// CLI11 writes them.
const char RESULTS[] =
    "last count=2 empty=0 results=[1|2] reduced=[2] run=1\n"
    "first count=2 empty=0 results=[3|4] reduced=[3] run=1\n"
    "all count=2 empty=0 results=[5|6] reduced=[5|6] run=1\n"
    "joined count=2 empty=0 results=[7|8] reduced=[7\\n8] run=1\n"
    "sum count=2 empty=0 results=[9|10] reduced=[19] run=1\n"
    "reversed count=2 empty=0 results=[11|12] reduced=[12] run=1\n"
    "as: last=2 first=3 all=2 joined=7\n"
    "8 sum=19 reversed=1\n"
    "added count=5 empty=0 results=[1|2|3|4.5|5] reduced=[1|2|3|4.5|5] run=0\n"
    "added=2 reals=5 single=1\n"
    "cleared count=0 empty=1 results=[] reduced=[] run=0\n"
    "given count=1 empty=0 results=[1] reduced=[1] run=1\n"
    "absent count=0 empty=1 results=[] reduced=[] run=0\n"
    "forced count=1 empty=0 results=[] reduced=[] run=1\n"
    "bound=9\n"
    "rerun count=2 empty=0 results=[1|9] reduced=[9] run=1\n";

// BINDING
//   constant: what the steps of d_tests_arg_cpp_option_binding write, as
// CLI11 writes them.
const char BINDING[] =
    "cmd app n=1 miss=[] subs=[] "
    "order=[--int|--uint|--real|--text|--char|--bool|--list|--list|--list]\n"
    "opt int n=1 res=[-7]\n"
    "opt text n=1 res=[a b]\n"
    "opt list n=3 res=[1|2|3]\n"
    "variables: int=-7 uint=7 real=25 text=a b char=z bool=1 list=3\n";

// HANDLE
//   constant: what d_tests_arg_cpp_option_handle writes: the module's own,
// as CLI11 has no Option* on nothing to read, and no C option to wrap. A
// handle on no option reads as the C library reads NULL: every switch off,
// every count 0, and every text empty but the group, which reads OPTIONS, as
// an unset group does.
const char HANDLE[] =
    "none switches: required=0 configurable=0 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "none counts: policy=Throw priority=Normal delimiter=0 expected=0,0,0 "
    "type_size=0,0,0 items=0,0,0\n"
    "none text: description=[] has=0 option_text=[] group=[OPTIONS] type=[] "
    "env=[] default=[]\n"
    "name= pos= all= both= bare= single= l= s= f= positional=0 "
    "nonpositional=0\n"
    "none: valid=0 given=0 count=0 results=0 as=0 added=BadArgument "
    "validator=0 equal=1 unequal=0\n"
    "copies: same=1 required=1 record=1 deref=1 arrow=1 other=1 "
    "matching=n,NULL\n"
    "from_c switches: required=0 configurable=1 extra_args=0 trigger=0 force=0 "
    "for_default=0 inject=0 capture=0 no_override=0 case=0 underscore=0\n"
    "from_c counts: policy=Throw priority=Normal delimiter=0 expected=2,2,2 "
    "type_size=1,1,1 items=2,2,2\n"
    "from_c text: description=[made by C] has=1 option_text=[] group=[C] "
    "type=[] env=[] default=[]\n"
    "read by C: group=C expected=2\n";

// CALLBACKS
//   constant: what the steps of d_tests_arg_cpp_option_callbacks write: the
// module's own, as CLI11's Option has no callback() -- its callback_t is
// set only by the low-level add_option ISSUES 51 leaves out -- and its
// callbacks return nothing but a bool. The messages are CLI11's: a callback
// returning false is its "Could not convert".
const char CALLBACKS[] =
    "callback void: a b\n"
    "callback status: 1\n"
    "callback bool: 1\n"
    "callback bound: variable=7 values=1\n"
    "cmd app n=1 miss=[] subs=[] "
    "order=[--counted|--counted|--judged|--truth|--bound]\n"
    "run: 1\n"
    "callback status: 3\n"
    "ERR ValidationError 105: too many values\n"
    "cmd app n=1 miss=[] subs=[] order=[--judged|--judged|--judged]\n"
    "callback bool: 1\n"
    "ERR ConversionError 104: Could not convert: --truth = x\n"
    "cmd app n=1 miss=[] subs=[] order=[--truth]\n"
    "a default given:\n"
    "callback void: d\n"
    "callback void: \n"
    "callback void: 1\n"
    "cmd app n=1 miss=[] subs=[] order=[--later]\n"
    "callback void: 2\n"
    "by hand: OK OK\n";

// DEPARTED
//   constant: what d_tests_arg_cpp_option_departures writes: the module's
// own, where the face departs from CLI11 (ISSUES 34, 45, 50 and 58):
// as<T>() gives Type() for values that do not convert, where CLI11's throws
// ConversionError, and results(T&) says so as a status; add_result()
// returns a status, where CLI11's returns the Option*; is_given() is
// CLI11's operator bool, under a name every level has; a variable is bound
// after the option is made; and a bound variable keeps its value when the
// value given it does not convert, where CLI11's leaves what its
// conversion wrote before it failed -- 0 for `x7` into an int.
const char DEPARTED[] =
    "as<int>: 0\n"
    "results(int&): ConversionError kept=7 message=Could not convert: --word = "
    "abc\n"
    "add_result: OK count=2\n"
    "given: 1 late=4\n"
    "unbound: late=4 as=5\n"
    "refused: whole=3 count=1 message=Could not convert: --whole = x7\n";

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
 * @brief Writes every setting of an option, as its readers read them, on
 *        three lines of the run's transcript.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the lines name the option by.
 * @param[in]     _option  the option.
 */
void
write_settings(
    run&               _run,
    const char*        _label,
    const cli::option& _option
)
{
    std::ostringstream switches;
    std::ostringstream counts;
    std::ostringstream text;

    switches << _label
             << " switches: required=" << _option->get_required()
             << " configurable=" << _option->get_configurable()
             << " extra_args=" << _option->get_allow_extra_args()
             << " trigger=" << _option->get_trigger_on_parse()
             << " force=" << _option->get_force_callback()
             << " for_default=" << _option->get_run_callback_for_default()
             << " inject=" << _option->get_inject_separator()
             << " capture=" << _option->get_always_capture_default()
             << " no_override=" << _option->get_disable_flag_override()
             << " case=" << _option->get_ignore_case()
             << " underscore=" << _option->get_ignore_underscore();
    counts << _label
           << " counts: policy="
           << policy_name(_option->get_multi_option_policy())
           << " priority="
           << priority_name(_option->get_callback_priority())
           << " delimiter="
           << static_cast<int>(_option->get_delimiter())
           << " expected=" << _option->get_expected()
           << "," << _option->get_expected_min()
           << "," << _option->get_expected_max()
           << " type_size=" << _option->get_type_size()
           << "," << _option->get_type_size_min()
           << "," << _option->get_type_size_max()
           << " items=" << _option->get_items_expected()
           << "," << _option->get_items_expected_min()
           << "," << _option->get_items_expected_max();
    text << _label
         << " text: description=[" << _option->get_description()
         << "] has=" << _option->has_description()
         << " option_text=[" << _option->get_option_text()
         << "] group=[" << _option->get_group()
         << "] type=[" << _option->get_type_name()
         << "] env=[" << _option->get_envname()
         << "] default=[" << _option->get_default_str()
         << "]";
    _run.note(switches.str());
    _run.note(counts.str());
    _run.note(text.str());

    return;
}

/**
 * @brief Writes every name of an option, as its readers read them, on one
 *        line of the run's transcript.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _option  the option.
 */
void
write_names(
    run&               _run,
    const cli::option& _option
)
{
    const std::vector<std::string> lnames = _option->get_lnames();
    const std::vector<std::string> snames = _option->get_snames();
    const std::vector<std::string> fnames = _option->get_fnames();
    std::ostringstream             line;

    line << "name=" << _option->get_name()
         << " pos=" << _option->get_name(true)
         << " all=" << _option->get_name(false,
                                         true)
         << " both=" << _option->get_name(true,
                                          true)
         << " bare=" << _option->get_name(false,
                                          true,
                                          true)
         << " single=" << _option->get_single_name()
         << " l=" << joined(lnames,
                            ",")
         << " s=" << joined(snames,
                            ",")
         << " f=" << joined(fnames,
                            ",")
         << " positional=" << _option->get_positional()
         << " nonpositional=" << _option->nonpositional();
    _run.note(line.str());

    return;
}

/**
 * @brief Writes an option's values -- as given, and as its policy keeps
 *        them -- with its count and whether its callback ran, on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the option by.
 * @param[in]     _option  the option.
 */
void
write_values(
    run&               _run,
    const char*        _label,
    const cli::option& _option
)
{
    const std::vector<std::string> results = _option->results();
    const std::vector<std::string> reduced = _option->reduced_results();
    std::ostringstream             line;

    line << _label
         << " count=" << _option->count()
         << " empty=" << _option->empty()
         << " results=[" << joined(results,
                                   "|")
         << "] reduced=[" << joined(reduced,
                                    "|")
         << "] run=" << _option->get_callback_run();
    _run.note(line.str());

    return;
}

/**
 * @brief The names of the options a list holds, sorted: CLI11's lists are
 *        a std::set or a std::vector of Option*, the face's a list of
 *        handles, and both are read with an iterator; a set of pointers is
 *        ordered by address, so the names are sorted.
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

    // each option of the list
    for (typename List::const_iterator at = _list.begin();
         at != _list.end();
         ++at)
    {
        names.push_back((*at)->get_name());
    }

    std::sort(names.begin(),
              names.end());

    return names;
}

/**
 * @brief Writes the names of an option's needs and excludes.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the option by.
 * @param[in]     _option  the option.
 */
void
write_relations(
    run&               _run,
    const char*        _label,
    const cli::option& _option
)
{
    _run.note(std::string(_label) +
              " needs=[" +
              joined(names_of(_option->get_needs()),
                     ",") +
              "] excludes=[" +
              joined(names_of(_option->get_excludes()),
                     ",") +
              "]");

    return;
}

// 1.1    Settings
//------------------------------------------------------------------------------

/**
 * @brief SETTINGS 1: an option as add_option makes it, every setting at its
 *        default.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_bare(
    run& _run
)
{
    write_settings(_run,
                   "bare",
                   _run.app().add_option("-n,--number"));

    return;
}

/**
 * @brief SETTINGS 2: each switch set, chained with `->`, then each cleared.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_switches(
    run& _run
)
{
    cli::option option = _run.app().add_option("-n,--number");

    option->required()
          ->configurable(false)
          ->allow_extra_args()
          ->trigger_on_parse()
          ->force_callback()
          ->run_callback_for_default()
          ->always_capture_default()
          ->disable_flag_override()
          ->ignore_case()
          ->ignore_underscore();
    option->inject_separator();
    write_settings(_run,
                   "set",
                   option);
    option->mandatory(false)
          ->configurable()
          ->allow_extra_args(false)
          ->trigger_on_parse(false)
          ->force_callback(false)
          ->run_callback_for_default(false)
          ->always_capture_default(false)
          ->disable_flag_override(false)
          ->ignore_case(false)
          ->ignore_underscore(false);
    option->inject_separator(false);
    write_settings(_run,
                   "cleared",
                   option);
    option->mandatory();
    write_settings(_run,
                   "mandatory",
                   option);

    return;
}

/**
 * @brief SETTINGS 3: each multi-option policy, by name and by its
 *        shorthand, the delimiter they set, and each callback priority.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_policies(
    run& _run
)
{
    cli::option option = _run.app().add_option("--values");

    option->take_last();
    write_settings(_run,
                   "take_last",
                   option);
    option->take_first();
    write_settings(_run,
                   "take_first",
                   option);
    option->take_all();
    write_settings(_run,
                   "take_all",
                   option);
    option->join();
    write_settings(_run,
                   "join",
                   option);
    option->join(';');
    write_settings(_run,
                   "join_semicolon",
                   option);
    option->multi_option_policy(cli::multi_option_policy::sum)
          ->delimiter(',');
    write_settings(_run,
                   "sum",
                   option);
    option->multi_option_policy(cli::multi_option_policy::reverse)
          ->delimiter();
    write_settings(_run,
                   "reverse",
                   option);
    option->multi_option_policy();
    write_settings(_run,
                   "throw",
                   option);
    option->callback_priority(cli::callback_priority::first_pre_help);
    write_settings(_run,
                   "first_pre_help",
                   option);
    option->callback_priority(cli::callback_priority::pre_requirements);
    write_settings(_run,
                   "pre_requirements",
                   option);
    option->callback_priority(cli::callback_priority::last);
    write_settings(_run,
                   "last",
                   option);
    option->callback_priority();
    write_settings(_run,
                   "normal",
                   option);

    return;
}

/**
 * @brief SETTINGS 4: the counts: expected, alone and as a range, type
 *        sizes, and the items they come to.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_counts(
    run& _run
)
{
    cli::option option = _run.app().add_option("--values");

    option->expected(3);
    write_settings(_run,
                   "expected_3",
                   option);
    option->expected(1,
                     4);
    write_settings(_run,
                   "expected_1_4",
                   option);
    option->expected(-1);
    write_settings(_run,
                   "expected_unlimited",
                   option);
    option->type_size(2);
    write_settings(_run,
                   "type_size_2",
                   option);
    option->type_size(1,
                      3);
    write_settings(_run,
                   "type_size_1_3",
                   option);
    option->expected(0,
                     1);
    write_settings(_run,
                   "expected_0_1",
                   option);

    return;
}

/**
 * @brief SETTINGS 5: the text an option carries: description, option
 *        text, group, type name, and the environment variable.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_text(
    run& _run
)
{
    cli::option option = _run.app().add_option("--values",
                                               "first description");

    write_settings(_run,
                   "described",
                   option);
    option->description("second description")
          ->option_text("TEXT ...")
          ->group("Values")
          ->type_name("VALUE")
          ->envname("VALUES_ENV");
    write_settings(_run,
                   "set",
                   option);
    option->description("")
          ->group("");
    write_settings(_run,
                   "emptied",
                   option);

    return;
}

// type_name_maker
//   struct: a type_name_fn() function, as CLI11's takes a
// std::function<std::string()>.
struct type_name_maker
{
    /**
     * @brief The type's name.
     *
     * @return "MADE".
     */
    std::string
    operator()() const
    {
        return std::string("MADE");
    }
};

/**
 * @brief SETTINGS 6: a type name given by a function, and the type name a
 *        bound variable gives.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_type_names(
    run& _run
)
{
    static int  count = 0;
    cli::option made  = _run.app().add_option("--made");
    cli::option bound = _run.app().add_option("--bound",
                                              count,
                                              "bound");

    made->type_name_fn(type_name_maker());
    write_settings(_run,
                   "made",
                   made);
    write_settings(_run,
                   "bound",
                   bound);

    return;
}

// SETTINGS_STEPS
//   constant: the steps of d_tests_arg_cpp_option_settings.
const step_fn SETTINGS_STEPS[] =
{
    &settings_bare,
    &settings_switches,
    &settings_policies,
    &settings_counts,
    &settings_text,
    &settings_type_names
};

// SETTINGS_COUNT
//   constant: how many steps SETTINGS_STEPS holds.
const std::size_t SETTINGS_COUNT =
    sizeof(SETTINGS_STEPS) / sizeof(SETTINGS_STEPS[0]);

// 1.2    Names
//------------------------------------------------------------------------------

/**
 * @brief NAMES 1: the names definition strings make, each written by every
 *        reader of names.
 *
 * @param[in,out] _run  the step's run.
 */
void
names_written(
    run& _run
)
{
    static const char* const DEFINITIONS[] =
    {
        "-a",
        "--alpha",
        "-a,--alpha",
        "--alpha,-a,--beta,-b",
        "pos",
        "pos,-p",
        "-p,pos,--place",
        " -c , --charlie ",
        "-d,-d",
        "--echo,--ECHO"
    };

    // each definition on a command of its own
    for (std::size_t index = 0u;
         index < (sizeof(DEFINITIONS) / sizeof(DEFINITIONS[0]));
         ++index)
    {
        cli::command sub = _run.app().add_subcommand(number(
            static_cast<unsigned long>(index)));

        _run.note(std::string("[") + DEFINITIONS[index] + "]");
        write_names(_run,
                    sub->add_option(DEFINITIONS[index]));
    }

    return;
}

/**
 * @brief NAMES 2: the names a flag's definition makes: values, negations
 *        and the names they leave.
 *
 * @param[in,out] _run  the step's run.
 */
void
names_flags(
    run& _run
)
{
    static const char* const DEFINITIONS[] =
    {
        "-v,--verbose",
        "--verbose{2}",
        "-v{3},--verbose",
        "!--no-color,--color",
        "--color,!--no-color{x}",
        "-q{false},--quiet"
    };

    // each definition on a command of its own
    for (std::size_t index = 0u;
         index < (sizeof(DEFINITIONS) / sizeof(DEFINITIONS[0]));
         ++index)
    {
        cli::command sub = _run.app().add_subcommand(number(
            static_cast<unsigned long>(index)));

        _run.note(std::string("[") + DEFINITIONS[index] + "]");
        write_names(_run,
                    sub->add_flag(DEFINITIONS[index]));
    }

    return;
}

// CANDIDATES
//   constant: the names names_matched checks against each option.
const char* const CANDIDATES[] =
{
    "-a",
    "a",
    "--alpha_beta",
    "alpha_beta",
    "ALPHA_BETA",
    "alphabeta",
    "--alphabeta",
    "pos",
    "POS",
    "no-x",
    "--no-x",
    "x"
};

// CANDIDATE_COUNT
//   constant: how many names CANDIDATES holds.
const std::size_t CANDIDATE_COUNT = sizeof(CANDIDATES) / sizeof(CANDIDATES[0]);

/**
 * @brief Writes how an option matches each candidate name, under a setting:
 *        check_name, check_sname, check_lname and check_fname in turn.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _setting  the setting's number.
 * @param[in]     _option   the option.
 */
void
write_matches(
    run&               _run,
    int                _setting,
    const cli::option& _option
)
{
    std::ostringstream line;

    line << "setting " << _setting << ":";

    // each candidate name
    for (std::size_t index = 0u; index < CANDIDATE_COUNT; ++index)
    {
        line << " " << CANDIDATES[index] << "="
             << _option->check_name(CANDIDATES[index])
             << _option->check_sname(CANDIDATES[index])
             << _option->check_lname(CANDIDATES[index])
             << _option->check_fname(CANDIDATES[index]);
    }

    _run.note(line.str());

    return;
}

/**
 * @brief NAMES 3: names matched -- check_name and its kin, for an option
 *        and for a flag, under each setting that changes how names
 *        compare.
 *
 * @param[in,out] _run  the step's run.
 */
void
names_matched(
    run& _run
)
{
    cli::command flags      = _run.app().add_subcommand("flags");
    cli::option  options[2] =
    {
        _run.app().add_option("-a,--alpha_beta,pos"),
        flags->add_flag("-a,--alpha_beta,!--no-x")
    };

    // each option under each setting
    for (int setting = 0; setting < 6; ++setting)
    {
        cli::option option = options[setting / 3];

        // case ignored from the second setting
        if ((setting % 3) > 0)
        {
            option->ignore_case();
        }

        // underscores ignored from the third
        if ((setting % 3) > 1)
        {
            option->ignore_underscore();
        }

        write_matches(_run,
                      setting,
                      option);
    }

    return;
}

// NAMES_STEPS
//   constant: the steps of d_tests_arg_cpp_option_names.
const step_fn NAMES_STEPS[] =
{
    &names_written,
    &names_flags,
    &names_matched
};

// NAMES_COUNT
//   constant: how many steps NAMES_STEPS holds.
const std::size_t NAMES_COUNT =
    sizeof(NAMES_STEPS) / sizeof(NAMES_STEPS[0]);

// 1.3    Defaults
//------------------------------------------------------------------------------

// default_maker
//   struct: a default_function() function, as CLI11's takes a
// std::function<std::string()>.
struct default_maker
{
    /**
     * @brief The default's text.
     *
     * @return "computed".
     */
    std::string
    operator()() const
    {
        return std::string("computed");
    }
};

/**
 * @brief DEFAULTS 1: default_str, default_val over values of several
 *        types, and what each leaves in the option and in a bound variable.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_given(
    run& _run
)
{
    static int              count  = 1;
    static double           ratio  = 0.0;
    static std::string      name;
    static std::vector<int> values;
    std::vector<int>        listed;
    cli::option             text   = _run.app().add_option("--text");
    cli::option             number = _run.app().add_option("--count",
                                                           count);
    cli::option             real   = _run.app().add_option("--ratio",
                                                           ratio);
    cli::option             named  = _run.app().add_option("--name",
                                                           name);
    cli::option             list   = _run.app().add_option("--values",
                                                           values);

    listed.push_back(1);
    listed.push_back(2);
    text->default_str("plain");
    write_settings(_run,
                   "default_str",
                   text);
    text->default_val("given");
    write_values(_run,
                 "text",
                 text);
    number->default_val(5);
    real->default_val(2.5);
    named->default_val(std::string("joe"));
    list->default_val(listed);
    write_values(_run,
                 "count",
                 number);
    write_values(_run,
                 "list",
                 list);

    std::ostringstream line;

    line << "variables: count=" << count
         << " ratio=" << ratio
         << " name=" << name
         << " values=" << values.size()
         << " defaults=[" << number->get_default_str()
         << "|" << real->get_default_str()
         << "|" << named->get_default_str()
         << "|" << list->get_default_str() << "]";
    _run.note(line.str());

    return;
}

/**
 * @brief DEFAULTS 2: defaults captured from bound variables -- by
 *        capture_default_str, and for every option a command makes once
 *        its option defaults always capture -- and from a function.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_captured(
    run& _run
)
{
    static int         before = 3;
    static int         after  = 4;
    static std::string text   = "word";
    cli::option        first  = _run.app().add_option("--before",
                                                      before);
    cli::option        made   = _run.app().add_option("--made");

    first->capture_default_str();
    made->default_function(default_maker());
    write_settings(_run,
                   "captured",
                   first);
    write_settings(_run,
                   "function_unused",
                   made);
    made->capture_default_str();
    write_settings(_run,
                   "function",
                   made);
    _run.app().option_defaults()->always_capture_default();

    cli::option second = _run.app().add_option("--after",
                                               after);
    cli::option third  = _run.app().add_option("--text",
                                               text);

    write_settings(_run,
                   "always",
                   second);
    write_settings(_run,
                   "always_text",
                   third);

    return;
}

/**
 * @brief DEFAULTS 3: a default given to an option that checks its values,
 *        within the check, then refused by it.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_refused(
    run& _run
)
{
    cli::option checked = _run.app().add_option("--checked");

    checked->check(cli::range(1,
                              10))
           ->default_val(7);
    write_settings(_run,
                   "within",
                   checked);
    checked->default_val(11);

    return;
}

// DEFAULTS_STEPS
//   constant: the steps of d_tests_arg_cpp_option_defaults.
const step_fn DEFAULTS_STEPS[] =
{
    &defaults_given,
    &defaults_captured,
    &defaults_refused
};

// DEFAULTS_COUNT
//   constant: how many steps DEFAULTS_STEPS holds.
const std::size_t DEFAULTS_COUNT =
    sizeof(DEFAULTS_STEPS) / sizeof(DEFAULTS_STEPS[0]);

// 1.4    Relations
//------------------------------------------------------------------------------

/**
 * @brief RELATIONS 1: needs and excludes by handle and by name, as the
 *        options list them and as a parse holds them.
 *
 * @param[in,out] _run  the step's run.
 */
void
relations_linked(
    run& _run
)
{
    static const char* const NEEDING[]   = { "--first", "1" };
    static const char* const BOTH[]      = { "--first", "1", "--second", "2" };
    static const char* const EXCLUDING[] = { "--first", "1", "--third", "3" };
    cli::option first  = _run.watch("first",
                                    _run.app().add_option("--first"));
    cli::option second = _run.watch("second",
                                    _run.app().add_option("--second"));
    cli::option third  = _run.watch("third",
                                    _run.app().add_option("--third"));

    first->needs(second)
         ->excludes("--third");
    write_relations(_run,
                    "first",
                    first);
    write_relations(_run,
                    "second",
                    second);
    write_relations(_run,
                    "third",
                    third);
    _run.parse(NEEDING,
               2u);
    _run.parse(BOTH,
               4u);
    _run.parse(EXCLUDING,
               4u);

    return;
}

/**
 * @brief RELATIONS 2: relations removed, each side of an exclusion apart,
 *        and a removal that finds nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
relations_removed(
    run& _run
)
{
    static const char* const BOTH[] = { "--first", "1", "--third", "3" };
    cli::option first  = _run.watch("first",
                                    _run.app().add_option("--first"));
    cli::option second = _run.watch("second",
                                    _run.app().add_option("--second"));
    cli::option third  = _run.watch("third",
                                    _run.app().add_option("--third"));

    first->needs("--second")
         ->excludes(third);

    const bool         needs    = first->remove_needs(second);
    const bool         again    = first->remove_needs(second);
    const bool         excludes = first->remove_excludes(third);
    std::ostringstream line;

    line << "removed: needs=" << needs
         << " again=" << again
         << " excludes=" << excludes;
    _run.note(line.str());
    write_relations(_run,
                    "first",
                    first);
    write_relations(_run,
                    "third",
                    third);
    _run.parse(BOTH,
               4u);
    third->remove_excludes(first);
    write_relations(_run,
                    "third",
                    third);
    _run.parse(BOTH,
               4u);

    return;
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief SEVERAL 1: several relations at once, by handle and by name, as
 *        C++11 lets a call take them.
 *
 * @param[in,out] _run  the step's run.
 */
void
relations_several(
    run& _run
)
{
    cli::option first  = _run.app().add_option("--first");
    cli::option second = _run.app().add_option("--second");
    cli::option third  = _run.app().add_option("--third");
    cli::option fourth = _run.app().add_option("--fourth");
    cli::option fifth  = _run.app().add_option("--fifth");

    (void)third;
    fourth->needs(first,
                  "--second",
                  std::string("--third"));
    fifth->excludes(first,
                    second);
    write_relations(_run,
                    "fourth",
                    fourth);
    write_relations(_run,
                    "fifth",
                    fifth);
    write_relations(_run,
                    "first",
                    first);

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief RELATIONS 3: a need named that no option has.
 *
 * @param[in,out] _run  the step's run.
 */
void
relations_unknown(
    run& _run
)
{
    _run.app().add_option("--first")
              ->needs("--missing");

    return;
}

// RELATIONS_STEPS
//   constant: the steps of d_tests_arg_cpp_option_relations.
const step_fn RELATIONS_STEPS[] =
{
    &relations_linked,
    &relations_removed,
    &relations_unknown
};

// RELATIONS_COUNT
//   constant: how many steps RELATIONS_STEPS holds.
const std::size_t RELATIONS_COUNT =
    sizeof(RELATIONS_STEPS) / sizeof(RELATIONS_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// SEVERAL_STEPS
//   constant: the steps of d_tests_arg_cpp_option_relations_several.
const step_fn SEVERAL_STEPS[] =
{
    &relations_several
};

// SEVERAL_COUNT
//   constant: how many steps SEVERAL_STEPS holds.
const std::size_t SEVERAL_COUNT =
    sizeof(SEVERAL_STEPS) / sizeof(SEVERAL_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.5    Validators
//------------------------------------------------------------------------------

// even_length
//   struct: a check() function, as CLI11's takes a
// std::function<std::string(const std::string&)>: refuses a value of odd
// length.
struct even_length
{
    /**
     * @brief Judges a value.
     *
     * @param[in] _value  the value.
     * @return the empty string, or why the value is refused.
     */
    std::string
    operator()(
        const std::string& _value
    ) const
    {
        return ((_value.size() % 2u) == 0u)
               ? std::string()
               : std::string("odd length");
    }
};

// doubled
//   struct: a transform() function, as CLI11's takes a
// std::function<std::string(std::string)>: the value twice.
struct doubled
{
    /**
     * @brief Transforms a value.
     *
     * @param[in] _value  the value.
     * @return the value written twice.
     */
    std::string
    operator()(
        std::string _value
    ) const
    {
        return _value + _value;
    }
};

/**
 * @brief VALIDATORS 1: validators given to options, named and not, found by
 *        name and by index, turned off and on, and the type names they
 *        leave.
 *
 * @param[in,out] _run  the step's run.
 */
void
validators_held(
    run& _run
)
{
    static const char* const INSIDE[]  = { "--number", "5" };
    static const char* const OUTSIDE[] = { "--number", "11" };
    cli::option number = _run.watch("number",
                                    _run.app().add_option("--number"));

    number->check(cli::range(1,
                             10),
                  "range")
          ->check(cli::positive_number());
    _run.note(std::string("type=[") + number->get_type_name() + "]");
    _run.note(std::string("by name: ") +
              number->get_validator("range")->get_description());
    _run.note(std::string("by index: ") +
              number->get_validator(1)->get_description());
    _run.parse(INSIDE,
               2u);
    _run.parse(OUTSIDE,
               2u);
    number->get_validator("range")->active(false);
    _run.parse(OUTSIDE,
               2u);
    number->get_validator("range")->active();
    _run.parse(OUTSIDE,
               2u);

    return;
}

/**
 * @brief VALIDATORS 2: transformations and checks given as validators and
 *        as functions, and the order they run in.
 *
 * @param[in,out] _run  the step's run.
 */
void
validators_functions(
    run& _run
)
{
    static const char* const EVEN[] = { "--word", "ab", "--plain", "cd" };
    static const char* const ODD[]  = { "--word", "abc", "--plain", "cde" };
    cli::option              word   = _run.watch(
                                          "word",
                                          _run.app().add_option("--word"));
    cli::option              plain  = _run.watch(
                                          "plain",
                                          _run.app().add_option("--plain"));

    word->check(even_length(),
                "EVEN",
                "even")
        ->transform(doubled(),
                    "TWICE");
    plain->check(even_length(),
                 "EVEN");
    _run.note(std::string("types=[") + word->get_type_name() + "|" +
              plain->get_type_name() + "]");
    _run.parse(EVEN,
               4u);
    _run.parse(ODD,
               4u);

    return;
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief VALIDATORS_MAPPED 1: a mapping given to transform(), with a
 *        description and a name, for a value it maps and one it does not.
 *
 * @param[in,out] _run  the step's run.
 */
void
validators_mapped(
    run& _run
)
{
    static const char* const           MAPPED[]  = { "--mapped", "one" };
    static const char* const           UNKNOWN[] = { "--mapped", "three" };
    std::map<std::string, std::string> mapping;

    mapping["one"] = "1";
    mapping["two"] = "2";

    cli::option mapped = _run.watch("mapped",
                                    _run.app().add_option("--mapped"));

    mapped->transform(cli::transformer(mapping),
                      "MAPPED",
                      "map");
    _run.note(std::string("types=[") + mapped->get_type_name() + "]");
    _run.parse(MAPPED,
               2u);
    _run.parse(UNKNOWN,
               2u);

    return;
}

// VALIDATORS_MAPPED_STEPS
//   constant: the steps of d_tests_arg_cpp_option_validators_mapped.
const step_fn VALIDATORS_MAPPED_STEPS[] =
{
    &validators_mapped
};

// VALIDATORS_MAPPED_COUNT
//   constant: how many steps VALIDATORS_MAPPED_STEPS holds.
const std::size_t VALIDATORS_MAPPED_COUNT =
    sizeof(VALIDATORS_MAPPED_STEPS) / sizeof(VALIDATORS_MAPPED_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// VALIDATORS_STEPS
//   constant: the steps of d_tests_arg_cpp_option_validators.
const step_fn VALIDATORS_STEPS[] =
{
    &validators_held,
    &validators_functions
};

// VALIDATORS_COUNT
//   constant: how many steps VALIDATORS_STEPS holds.
const std::size_t VALIDATORS_COUNT =
    sizeof(VALIDATORS_STEPS) / sizeof(VALIDATORS_STEPS[0]);

// 1.6    Each
//------------------------------------------------------------------------------

// each_recorder
//   struct: an each() function, as CLI11's takes a
// std::function<void(const std::string&)>: writes each value it is given
// into the run.
struct each_recorder
{
    run* m_run;

    /**
     * @brief Writes a value into the run.
     *
     * @param[in] _value  the value.
     */
    void
    operator()(
        const std::string& _value
    ) const
    {
        m_run->note(std::string("each ") + escape(_value));

        return;
    }
};

/**
 * @brief EACH 1: each() over the values an option is given, over a list,
 *        a single value and values joined, and after a check that refuses.
 *
 * @param[in,out] _run  the step's run.
 */
void
each_values(
    run& _run
)
{
    static const char* const GIVEN[]   = { "--list", "a", "b", "--one", "x",
                                           "--joined", "p", "--joined", "q" };
    static const char* const REFUSED[] = { "--checked", "7" };
    const each_recorder recorder = { &_run };
    cli::option         list     = _run.watch("list",
                                              _run.app().add_option("--list"));
    cli::option         one      = _run.watch("one",
                                              _run.app().add_option("--one"));
    cli::option         joined   = _run.watch("joined",
                                              _run.app().add_option(
                                                  "--joined"));
    cli::option         checked  = _run.watch("checked",
                                              _run.app().add_option(
                                                  "--checked"));

    list->expected(1,
                   3)
        ->each(recorder);
    one->each(recorder);
    joined->join(',')
          ->each(recorder);
    checked->check(cli::range(1,
                              5))
           ->each(recorder);
    _run.parse(GIVEN,
               9u);
    _run.parse(REFUSED,
               2u);

    return;
}

// EACH_STEPS
//   constant: the steps of d_tests_arg_cpp_option_each.
const step_fn EACH_STEPS[] =
{
    &each_values
};

// EACH_COUNT
//   constant: how many steps EACH_STEPS holds.
const std::size_t EACH_COUNT =
    sizeof(EACH_STEPS) / sizeof(EACH_STEPS[0]);

// 1.7    Results
//------------------------------------------------------------------------------

/**
 * @brief RESULTS 1: the values a parse gives, as given and as each policy
 *        keeps them, and converted.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_policies(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--last", "1", "--last", "2",
        "--first", "3", "--first", "4",
        "--all", "5", "--all", "6",
        "--joined", "7", "--joined", "8",
        "--sum", "9", "--sum", "10",
        "--reversed", "11", "--reversed", "12"
    };
    cli::option last     = _run.app().add_option("--last");
    cli::option first    = _run.app().add_option("--first");
    cli::option all      = _run.app().add_option("--all");
    cli::option joined   = _run.app().add_option("--joined");
    cli::option summed   = _run.app().add_option("--sum");
    cli::option reversed = _run.app().add_option("--reversed");

    last->take_last();
    first->take_first();
    all->take_all();
    joined->join();
    summed->multi_option_policy(cli::multi_option_policy::sum);
    reversed->multi_option_policy(cli::multi_option_policy::reverse);
    (void)_run.parse_quiet(GIVEN,
                           24u);
    write_values(_run,
                 "last",
                 last);
    write_values(_run,
                 "first",
                 first);
    write_values(_run,
                 "all",
                 all);
    write_values(_run,
                 "joined",
                 joined);
    write_values(_run,
                 "sum",
                 summed);
    write_values(_run,
                 "reversed",
                 reversed);

    std::ostringstream line;

    line << "as: last=" << last->as<int>()
         << " first=" << first->as<std::string>()
         << " all=" << all->as<std::vector<std::string> >().size()
         << " joined=" << joined->as<std::string>()
         << " sum=" << summed->as<long>()
         << " reversed=" << reversed->as<std::vector<int> >().size();
    _run.note(line.str());

    return;
}

/**
 * @brief RESULTS 2: values added by hand -- one, one split at its
 *        delimiter with the count added, several -- converted into
 *        variables, then cleared.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_added(
    run& _run
)
{
    std::vector<std::string> more;
    int                      added  = 0;
    int                      single = 0;
    std::vector<double>      reals;
    cli::option              option = _run.app().add_option("--values");

    more.push_back("4.5");
    more.push_back("5");
    option->delimiter(',')
          ->expected(1,
                     10);
    option->add_result("1");
    option->add_result("2,3",
                       added);
    option->add_result(more);
    write_values(_run,
                 "added",
                 option);
    option->results(reals);
    option->take_first();
    option->results(single);

    std::ostringstream line;

    line << "added=" << added
         << " reals=" << reals.size()
         << " single=" << single;
    _run.note(line.str());
    option->clear();
    write_values(_run,
                 "cleared",
                 option);

    return;
}

/**
 * @brief RESULTS 3: an option given, one absent, one whose callback is
 *        forced, and a callback run by hand after a value is added, which
 *        the option, taking the last, keeps.
 *
 * @param[in,out] _run  the step's run.
 */
void
results_given(
    run& _run
)
{
    static const char* const GIVEN[] = { "--given", "1" };
    static int               bound   = 0;
    cli::option              given   = _run.app().add_option("--given",
                                                             bound);
    cli::option              absent  = _run.app().add_option("--absent");
    cli::option              forced  = _run.app().add_option("--forced");

    given->take_last();
    forced->force_callback();
    (void)_run.parse_quiet(GIVEN,
                           2u);
    write_values(_run,
                 "given",
                 given);
    write_values(_run,
                 "absent",
                 absent);
    write_values(_run,
                 "forced",
                 forced);
    given->add_result("9");
    given->run_callback();

    std::ostringstream line;

    line << "bound=" << bound;
    _run.note(line.str());
    write_values(_run,
                 "rerun",
                 given);

    return;
}

// RESULTS_STEPS
//   constant: the steps of d_tests_arg_cpp_option_results.
const step_fn RESULTS_STEPS[] =
{
    &results_policies,
    &results_added,
    &results_given
};

// RESULTS_COUNT
//   constant: how many steps RESULTS_STEPS holds.
const std::size_t RESULTS_COUNT =
    sizeof(RESULTS_STEPS) / sizeof(RESULTS_STEPS[0]);

// 1.8    Binding
//------------------------------------------------------------------------------

/**
 * @brief BINDING 1: variables of several types bound to options, given
 *        values.
 *
 * @param[in,out] _run  the step's run.
 */
void
binding_types(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--int", "-7", "--uint", "7", "--real", "2.5e1", "--text", "a b",
        "--char", "z", "--bool", "yes", "--list", "1", "2", "3"
    };
    static int               whole  = 1;
    static unsigned int      count  = 1u;
    static double            real   = 1.0;
    static std::string       text   = "unset";
    static char              letter = 'a';
    static bool              truth  = false;
    static std::vector<int>  list;

    (void)_run.watch("int",
                     _run.app().add_option("--int",
                                           whole));
    (void)_run.app().add_option("--uint",
                                count);
    (void)_run.app().add_option("--real",
                                real);
    (void)_run.watch("text",
                     _run.app().add_option("--text",
                                           text));
    (void)_run.app().add_option("--char",
                                letter);
    (void)_run.app().add_option("--bool",
                                truth);
    (void)_run.watch("list",
                     _run.app().add_option("--list",
                                           list));
    _run.parse(GIVEN,
               16u);

    std::ostringstream line;

    line << "variables: int=" << whole
         << " uint=" << count
         << " real=" << real
         << " text=" << text
         << " char=" << letter
         << " bool=" << truth
         << " list=" << list.size();
    _run.note(line.str());

    return;
}

// BINDING_STEPS
//   constant: the steps of d_tests_arg_cpp_option_binding.
const step_fn BINDING_STEPS[] =
{
    &binding_types
};

// BINDING_COUNT
//   constant: how many steps BINDING_STEPS holds.
const std::size_t BINDING_COUNT =
    sizeof(BINDING_STEPS) / sizeof(BINDING_STEPS[0]);

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: the handle on no option and over a C option,
// callbacks over the values, and the departures. Each is a step written
// here, through the whole face.

namespace
{

/**
 * @brief HANDLE 1: what a handle on no option reads, after every setting
 *        was given to it.
 *
 * @param[in,out] _run  the step's run.
 */
void
handle_none(
    run& _run
)
{
    cli::option none;

    none.description("x")
        ->group("G")
        .required()
        .take_last()
        .expected(2)
        .check(cli::range(1,
                          2))
        .default_val(3);
    write_settings(_run,
                   "none",
                   none);
    write_names(_run,
                none);

    const cli::status  added = none.add_result(cli::text("1"));
    std::ostringstream line;

    line << "none: valid=" << none.valid()
         << " given=" << none.is_given()
         << " count=" << none.count()
         << " results=" << none.results().size()
         << " as=" << none.as<int>()
         << " added=" << added.get_name()
         << " validator=" << none.get_validator().valid()
         << " equal=" << (none == cli::option())
         << " unequal=" << (none != cli::option());
    _run.note(line.str());

    return;
}

/**
 * @brief HANDLE 2: what handles on one option read: copies, comparisons,
 *        the C option they hold, an option C made, read and set through a
 *        handle, and the name two options share.
 *
 * @param[in,out] _run  the step's run.
 */
void
handle_records(
    run& _run
)
{
    cli::option       made   = _run.app().add_option("-n,--number");
    cli::option       copy   = made;
    cli::option       from_c = cli::option(
        d_arg_command_add_option(&_run.app().command,
                                 "--from-c",
                                 "made by C"));
    const char* const shared = made.matching_name(copy);
    const char* const none   = made.matching_name(from_c);

    copy.required();
    from_c->group("C")
          ->expected(2);

    std::ostringstream line;

    line << "copies: same=" << (copy == made)
         << " required=" << made.get_required()
         << " record=" << (made.get() == copy.get())
         << " deref=" << (*made).get_required()
         << " arrow=" << made->get_required()
         << " other=" << (made != from_c)
         << " matching=" << ((shared != D_NULLPTR) ? shared : "NULL")
         << "," << ((none != D_NULLPTR) ? none : "NULL");
    _run.note(line.str());
    write_settings(_run,
                   "from_c",
                   from_c);
    _run.note(std::string("read by C: group=") +
              d_arg_option_group(from_c.get()) +
              " expected=" +
              number(static_cast<long>(from_c.get()->expected_min)));

    return;
}

// the handle is one pointer, as CLI11's Option* is
D_STATIC_ASSERT( (sizeof(cli::option) == sizeof(::d_arg_option*)),
                 "an option handle is one pointer" );

// HANDLE_STEPS
//   constant: the steps of d_tests_arg_cpp_option_handle.
const step_fn HANDLE_STEPS[] =
{
    &handle_none,
    &handle_records
};

// HANDLE_COUNT
//   constant: how many steps HANDLE_STEPS holds.
const std::size_t HANDLE_COUNT =
    sizeof(HANDLE_STEPS) / sizeof(HANDLE_STEPS[0]);

// values_counter
//   struct: an option's callback returning nothing, taking the values as a
// text_list: writes them.
struct values_counter
{
    run* m_run;

    /**
     * @brief Writes the values given.
     *
     * @param[in] _values  the values.
     */
    void
    operator()(
        const cli::text_list& _values
    ) const
    {
        std::string line = "callback void:";

        // each value
        for (std::size_t index = 0u; index < _values.size(); ++index)
        {
            line += " ";
            line += escape(_values[index]);
        }

        m_run->note(line);

        return;
    }
};

// values_judge
//   struct: an option's callback returning a status, taking the values as a
// std::vector<std::string>: refuses more than two.
struct values_judge
{
    run*         m_run;
    cli::option* m_option;

    /**
     * @brief Judges the values given.
     *
     * @param[in] _values  the values.
     * @return ok, or the failure the option records.
     */
    cli::status
    operator()(
        const std::vector<std::string>& _values
    ) const
    {
        m_run->note(std::string("callback status: ") +
                    number(static_cast<unsigned long>(_values.size())));

        return (_values.size() > 2u)
               ? m_option->fail(cli::status_code::validation_error,
                                "too many values")
               : cli::status();
    }
};

// values_truth
//   struct: an option's callback returning a bool, as CLI11's callback_t:
// refuses a value "x", false being CLI11's "Could not convert".
struct values_truth
{
    run* m_run;

    /**
     * @brief Judges the values given.
     *
     * @param[in] _values  the values.
     * @return whether none is "x".
     */
    bool
    operator()(
        const cli::text_list& _values
    ) const
    {
        m_run->note(std::string("callback bool: ") +
                    number(static_cast<unsigned long>(_values.size())));

        // each value
        for (std::size_t index = 0u; index < _values.size(); ++index)
        {
            // the one value refused
            if (_values[index] == "x")
            {
                return false;
            }
        }

        return true;
    }
};

// bound_reader
//   struct: an option's callback reading the variable bound to the option,
// which is written before the callback runs.
struct bound_reader
{
    run*       m_run;
    const int* m_variable;

    /**
     * @brief Writes the variable.
     *
     * @param[in] _values  the values.
     */
    void
    operator()(
        const cli::text_list& _values
    ) const
    {
        m_run->note(std::string("callback bound: variable=") +
                    number(static_cast<long>(*m_variable)) +
                    " values=" +
                    number(static_cast<unsigned long>(_values.size())));

        return;
    }
};

/**
 * @brief CALLBACKS 1: callbacks of each result kind over the values, a
 *        variable bound beside one, and the parses that call them.
 *
 * @param[in,out] _run  the step's run.
 */
void
callbacks_kinds(
    run& _run
)
{
    static const char* const GIVEN[] =
    {
        "--counted", "a", "b", "--judged", "1", "--truth", "y",
        "--bound", "7"
    };
    static const char* const TOO_MANY[] = { "--judged", "1", "2", "3" };
    static const char* const REFUSED[]  = { "--truth", "x" };
    static int               bound      = 0;
    cli::option              counted    = _run.app().add_option("--counted");
    cli::option              judged     = _run.app().add_option("--judged");
    cli::option              truth      = _run.app().add_option("--truth");
    cli::option              read       = _run.app().add_option("--bound",
                                                                bound);
    const values_counter     counter    = { &_run };
    const values_judge       judge      = { &_run, &judged };
    const values_truth       truths     = { &_run };
    const bound_reader       reader     = { &_run, &bound };

    counted.expected(1,
                     5)
           .callback(counter);
    judged.expected(1,
                    5)
          .callback(judge);
    truth.callback(truths);
    read.callback(reader);
    _run.parse(GIVEN,
               9u);
    _run.note(std::string("run: ") +
              number(static_cast<long>(counted.get_callback_run())));
    _run.parse(TOO_MANY,
               4u);
    _run.parse(REFUSED,
               2u);

    return;
}

/**
 * @brief CALLBACKS 2: a callback forced to run with no values, one run
 *        for a default as default_val gives it, one run as the option is
 *        parsed, and one run by hand.
 *
 * @param[in,out] _run  the step's run.
 */
void
callbacks_when(
    run& _run
)
{
    static const char* const LATER[] = { "--later", "1" };
    const values_counter     counter = { &_run };
    cli::option              forced  = _run.app().add_option("--forced");
    cli::option              by_def  = _run.app().add_option("--default");
    cli::option              at_once = _run.app().add_option("--at-once");
    cli::option              later   = _run.app().add_option("--later");

    forced.force_callback()
          .callback(counter);
    _run.note("a default given:");
    by_def.run_callback_for_default()
          .callback(counter)
          .default_val("d");
    at_once.trigger_on_parse()
           .callback(counter);
    later.take_last()
         .callback(counter);
    _run.parse(LATER,
               2u);

    const cli::status added = later.add_result(cli::text("2"));
    const cli::status ran   = later.run_callback();

    _run.note(std::string("by hand: ") + added.get_name() + " " +
              ran.get_name());

    return;
}

// CALLBACKS_STEPS
//   constant: the steps of d_tests_arg_cpp_option_callbacks.
const step_fn CALLBACKS_STEPS[] =
{
    &callbacks_kinds,
    &callbacks_when
};

// CALLBACKS_COUNT
//   constant: how many steps CALLBACKS_STEPS holds.
const std::size_t CALLBACKS_COUNT =
    sizeof(CALLBACKS_STEPS) / sizeof(CALLBACKS_STEPS[0]);

/**
 * @brief DEPARTED 1: values that do not convert, read with as<T>() and
 *        results(T&); add_result's status; is_given; a variable bound
 *        after the option was made, then unbound; a bound variable given a
 *        value that does not convert.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_values(
    run& _run
)
{
    static const char* const GIVEN[] = { "--word", "abc", "--late", "4" };
    static const char* const AGAIN[] = { "--word", "def", "--late", "5" };
    static const char* const BAD[]   = { "--whole", "x7" };
    int                      kept    = 7;
    int                      late    = 0;
    int                      whole   = 3;
    cli::option              word    = _run.app().add_option("--word");
    cli::option              bound   = _run.app().add_option("--late");
    cli::option              refused = _run.app().add_option("--whole",
                                                             whole);

    bound.bind(late);
    (void)_run.parse_quiet(GIVEN,
                           4u);

    const cli::status converted = word.results(kept);

    _run.note(std::string("as<int>: ") +
              number(static_cast<long>(word.as<int>())));
    _run.note(std::string("results(int&): ") + converted.get_name() +
              " kept=" + number(static_cast<long>(kept)) +
              " message=" + escape(_run.app().get_error().what()));

    const cli::status added = word.add_result(cli::text("ghi"));

    _run.note(std::string("add_result: ") + added.get_name() +
              " count=" + number(static_cast<unsigned long>(word.count())));
    _run.note(std::string("given: ") +
              number(static_cast<long>(word.is_given())) +
              " late=" + number(static_cast<long>(late)));
    bound.unbind();
    (void)_run.parse_quiet(AGAIN,
                           4u);
    _run.note(std::string("unbound: late=") +
              number(static_cast<long>(late)) +
              " as=" + number(static_cast<long>(bound.as<int>())));
    (void)_run.parse_quiet(BAD,
                           2u);
    _run.note(std::string("refused: whole=") +
              number(static_cast<long>(whole)) +
              " count=" +
              number(static_cast<unsigned long>(refused.count())) +
              " message=" + escape(_run.app().get_error().what()));

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_option_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_values
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

}  // namespace

//==============================================================================
// 3.  THE HANDLE
//==============================================================================

/*
d_tests_arg_cpp_option_handle
  What a cli::option is: one pointer, on an option or on none.
  Tests the following:
  - a handle on no option ignores every setting, chained with `.` and `->`,
    and reads as an option with nothing set: no value, as<T>() its Type(),
    add_result refused, no validator
  - copies of a handle are the same option; == and != compare the options;
    get() is the C option, `*` and `->` reach the handle
  - a handle over an option C made sets and reads it as C does
  - matching_name of two handles on one option, and NULL for two options
    that share no name
*/
bool
d_tests_arg_cpp_option_handle()
{
    return steps_hold("HANDLE",
                      HANDLE_STEPS,
                      HANDLE_COUNT,
                      HANDLE);
}

//==============================================================================
// 4.  SETTINGS
//==============================================================================

/*
d_tests_arg_cpp_option_settings
  Every setting of CLI11's Option through the handle, read back through
every reader, as CLI11's Option reads.
  Tests the following:
  - an option as add_option makes it
  - each switch set and cleared, chained with `->`; mandatory as required
  - each multi-option policy by name and by its shorthand, the delimiters
    join and the policy's setter leave, each callback priority
  - expected alone and as a range, unlimited; type sizes; the items they
    come to
  - description, option text, group, type name, environment variable, and
    a description emptied
  - a type name given by a function, and the one a bound variable gives
*/
bool
d_tests_arg_cpp_option_settings()
{
    return steps_hold("SETTINGS",
                      SETTINGS_STEPS,
                      SETTINGS_COUNT,
                      SETTINGS);
}

/*
d_tests_arg_cpp_option_names
  What the readers of names give, and how names match, as CLI11's Option
gives them.
  Tests the following:
  - get_name in each of its forms, get_single_name, the lists of long,
    short and flag names, positional and nonpositional, for options of every
    kind of name a definition can give
  - the same for flags, with values and negations
  - check_name, check_sname, check_lname and check_fname, for an option and
    for a flag, for names given with and without dashes, under ignore_case
    and ignore_underscore
*/
bool
d_tests_arg_cpp_option_names()
{
    return steps_hold("NAMES",
                      NAMES_STEPS,
                      NAMES_COUNT,
                      NAMES);
}

//==============================================================================
// 5.  DEFINITION
//==============================================================================

/*
d_tests_arg_cpp_option_defaults
  Defaults, given and captured, as CLI11's Option keeps them.
  Tests the following:
  - default_str; default_val over text, a number, a real, a std::string and
    a vector, and what each leaves in the option's values and in a bound
    variable
  - capture_default_str from a bound variable; default_function, unused
    until a capture; the command's option defaults capturing every option's
  - a default within a check, and one the check refuses, which is the
    definition's failure
*/
bool
d_tests_arg_cpp_option_defaults()
{
    return steps_hold("DEFAULTS",
                      DEFAULTS_STEPS,
                      DEFAULTS_COUNT,
                      DEFAULTS);
}

/*
d_tests_arg_cpp_option_relations
  Needs and excludes, as CLI11's Option links them.
  Tests the following:
  - needs and excludes by handle and by name, listed by both options of an
    exclusion and by the needing one alone, and held by a parse
  - removal of each, one side of an exclusion at a time, and of what is not
    there
  - a need named that no option has
*/
bool
d_tests_arg_cpp_option_relations()
{
    return steps_hold("RELATIONS",
                      RELATIONS_STEPS,
                      RELATIONS_COUNT,
                      RELATIONS);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_option_relations_several
  Several relations given at once, as CLI11's variadic needs and excludes
take them, from C++11.
  Tests the following:
  - needs of a handle, a C string and a std::string in one call
  - excludes of two handles in one call, listed by both sides
*/
bool
d_tests_arg_cpp_option_relations_several()
{
    return steps_hold("SEVERAL",
                      SEVERAL_STEPS,
                      SEVERAL_COUNT,
                      SEVERAL);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_option_validators
  Validators an option holds, as CLI11's Option holds them.
  Tests the following:
  - check() with a validator, named and not; get_validator by name and by
    index; a validator turned off and on through its handle; the type name
    the validators leave
  - check() and transform() given functions, with a description and a name;
    a transformation running before the checks given before it
*/
bool
d_tests_arg_cpp_option_validators()
{
    return steps_hold("VALIDATORS",
                      VALIDATORS_STEPS,
                      VALIDATORS_COUNT,
                      VALIDATORS);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_option_validators_mapped
  A mapping an option transforms its values through, as CLI11's Transformer
given to an Option.
  Tests the following:
  - a mapping given to transform() with a description and a name: the type
    name it leaves, a value it maps, and one it passes as it came
*/
bool
d_tests_arg_cpp_option_validators_mapped()
{
    return steps_hold("VALIDATORS_MAPPED",
                      VALIDATORS_MAPPED_STEPS,
                      VALIDATORS_MAPPED_COUNT,
                      VALIDATORS_MAPPED);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

/*
d_tests_arg_cpp_option_each
  each(), as CLI11's Option runs it.
  Tests the following:
  - each value of a list, of a single value, of values joined
  - each() after a check that refuses the value: not called
*/
bool
d_tests_arg_cpp_option_each()
{
    return steps_hold("EACH",
                      EACH_STEPS,
                      EACH_COUNT,
                      EACH);
}

//==============================================================================
// 6.  VALUES
//==============================================================================

/*
d_tests_arg_cpp_option_results
  The values an option holds, as CLI11's Option gives them.
  Tests the following:
  - results() and reduced_results() under each multi-option policy, count()
    and empty(), and as<T>() over several types
  - add_result of one value, of one split at the delimiter with the count
    added, of a vector; results(T&) into a vector and, under take_first,
    into a number; clear()
  - get_callback_run; a callback forced with no value; run_callback by hand
    after add_result, writing a bound variable
*/
bool
d_tests_arg_cpp_option_results()
{
    return steps_hold("RESULTS",
                      RESULTS_STEPS,
                      RESULTS_COUNT,
                      RESULTS);
}

/*
d_tests_arg_cpp_option_callbacks
  An option's callback over its values -- the face's callback() -- in each
of its result kinds.
  Tests the following:
  - a callback returning nothing over a text_list; one returning a status
    over a std::vector<std::string>, failing through the option's fail();
    one returning a bool, false refusing as CLI11's "Could not convert"
  - a bound variable written before its callback runs
  - callbacks forced with no value, run for a default, run as the option is
    parsed, and run by hand after add_result
*/
bool
d_tests_arg_cpp_option_callbacks()
{
    return steps_hold("CALLBACKS",
                      CALLBACKS_STEPS,
                      CALLBACKS_COUNT,
                      CALLBACKS);
}

/*
d_tests_arg_cpp_option_binding
  Variables bound to options, as CLI11's add_option(name, variable) binds
them.
  Tests the following:
  - an int, an unsigned int, a double, a std::string, a char, a bool and a
    vector, each given its value
*/
bool
d_tests_arg_cpp_option_binding()
{
    return steps_hold("BINDING",
                      BINDING_STEPS,
                      BINDING_COUNT,
                      BINDING);
}

//==============================================================================
// 7.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_option_departures
  Where the face's option departs from CLI11's Option* (ISSUES 45, 50, 58).
  Tests the following:
  - as<T>() over values that do not convert gives Type(); results(T&) gives
    conversion_error, the message recorded, the variable untouched
  - add_result returns its status
  - is_given, CLI11's operator bool, at every level
  - bind() after the option was made, and unbind()
  - a bound variable keeps its value when the value given it does not
    convert (ISSUES 34)
*/
bool
d_tests_arg_cpp_option_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

//==============================================================================
// 8.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_option
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_option[] =
{
    { "cpp_option_handle",
      d_tests_arg_cpp_option_handle },
    { "cpp_option_settings",
      d_tests_arg_cpp_option_settings },
    { "cpp_option_names",
      d_tests_arg_cpp_option_names },
    { "cpp_option_defaults",
      d_tests_arg_cpp_option_defaults },
    { "cpp_option_relations",
      d_tests_arg_cpp_option_relations },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_option_relations_several",
      d_tests_arg_cpp_option_relations_several },
#endif
    { "cpp_option_validators",
      d_tests_arg_cpp_option_validators },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_option_validators_mapped",
      d_tests_arg_cpp_option_validators_mapped },
#endif
    { "cpp_option_each",
      d_tests_arg_cpp_option_each },
    { "cpp_option_results",
      d_tests_arg_cpp_option_results },
    { "cpp_option_callbacks",
      d_tests_arg_cpp_option_callbacks },
    { "cpp_option_binding",
      d_tests_arg_cpp_option_binding },
    { "cpp_option_departures",
      d_tests_arg_cpp_option_departures }
};

// d_tests_arg_cpp_option_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_option_count =
    sizeof(d_tests_arg_cpp_option) / sizeof(d_tests_arg_cpp_option[0]);
