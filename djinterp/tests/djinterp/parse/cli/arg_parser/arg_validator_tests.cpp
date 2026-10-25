/*******************************************************************************
* djinterp [parse]                                       arg_validator_tests.cpp
*
* The argument parser's C++ validator suite's sections.
*   The sections held to CLI11 run what the CASES section defines:
* validators made by the face's functions of CLI11's names, over the types
* and containers they take; described, named, set and combined; run over
* values -- each value's line written by transcribe(), which reads integers
* under CLI11's rule for leading zeros, as the C suites do -- and given to
* options in parses. tools/cppref/cppref.py compiles that section against
* CLI11 2.7.2, through the face's names for CLI11's (cli11_shim.hpp), and
* the transcripts below are what CLI11 wrote; so the CASES section is
* written in the part of the face CLI11 shares -- `->` on every handle, no
* status taken from a call -- and C++98 unless a guard says otherwise. The
* sections that are the module's own are written apart, after it, each
* saying why CLI11 cannot write its transcript.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_validator_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.21
*******************************************************************************/
#include "./arg_validator_tests.hpp"  // corresponding header
// std
#include <cstddef>   // std::size_t
#include <istream>   // std::istream
#include <map>       // std::map
#include <ostream>   // std::ostream
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <string>    // std::string
#include <utility>   // std::pair, std::make_pair, std::move
#include <vector>    // std::vector
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
using cli::testing::file_entry;
using cli::testing::escape;
using cli::testing::transcribe;
using cli::testing::scratch_directory;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// DEFAULTS
//   constant: what the steps of d_tests_arg_cpp_validator_defaults write, as
// CLI11 writes them.
const char DEFAULTS[] =
    "desc <INT in [1 - 10]>\n"
    "[5] -> pass <5>\n"
    "[1] -> pass <1>\n"
    "[10] -> pass <10>\n"
    "[0] -> fail <Value 0 not in range [1 - 10]>\n"
    "[11] -> fail <Value 11 not in range [1 - 10]>\n"
    "[-3] -> fail <Value -3 not in range [1 - 10]>\n"
    "[2.5] -> fail <Value 2.5 not in range [1 - 10]>\n"
    "[x] -> fail <Value x not in range [1 - 10]>\n"
    "[] -> fail <Value  not in range [1 - 10]>\n"
    "desc <FLOAT in [0.5 - 2.5]>\n"
    "[5] -> fail <Value 5 not in range [0.5 - 2.5]>\n"
    "[1] -> pass <1>\n"
    "[10] -> fail <Value 10 not in range [0.5 - 2.5]>\n"
    "[0] -> fail <Value 0 not in range [0.5 - 2.5]>\n"
    "[11] -> fail <Value 11 not in range [0.5 - 2.5]>\n"
    "[-3] -> fail <Value -3 not in range [0.5 - 2.5]>\n"
    "[2.5] -> pass <2.5>\n"
    "[x] -> fail <Value x not in range [0.5 - 2.5]>\n"
    "[] -> fail <Value  not in range [0.5 - 2.5]>\n"
    "desc <UINT in [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[1] -> pass <1>\n"
    "[10] -> pass <10>\n"
    "[0] -> pass <0>\n"
    "[11] -> fail <Value 11 not in range [0 - 10]>\n"
    "[-3] -> fail <Value -3 not in range [0 - 10]>\n"
    "[2.5] -> fail <Value 2.5 not in range [0 - 10]>\n"
    "[x] -> fail <Value x not in range [0 - 10]>\n"
    "[] -> fail <Value  not in range [0 - 10]>\n"
    "desc <from -5 to 5>\n"
    "[5] -> pass <5>\n"
    "[1] -> pass <1>\n"
    "[10] -> fail <Value 10 not in range [-5 - 5]>\n"
    "[0] -> pass <0>\n"
    "[11] -> fail <Value 11 not in range [-5 - 5]>\n"
    "[-3] -> pass <-3>\n"
    "[2.5] -> fail <Value 2.5 not in range [-5 - 5]>\n"
    "[x] -> fail <Value x not in range [-5 - 5]>\n"
    "[] -> fail <Value  not in range [-5 - 5]>\n"
    "desc <POSITIVE>\n"
    "[1] -> pass <1>\n"
    "[0] -> fail <Value 0 not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[-1] -> fail <Value -1 not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[0.5] -> pass <0.5>\n"
    "[-0.5] -> fail <Value -0.5 not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[1e3] -> pass <1e3>\n"
    "[x] -> fail <Value x not in range [4.94066e-324 - 1.79769e+308]>\n"
    "[] -> fail <Value  not in range [4.94066e-324 - 1.79769e+308]>\n"
    "desc <NONNEGATIVE>\n"
    "[1] -> pass <1>\n"
    "[0] -> pass <0>\n"
    "[-1] -> fail <Value -1 not in range [0 - 1.79769e+308]>\n"
    "[0.5] -> pass <0.5>\n"
    "[-0.5] -> fail <Value -0.5 not in range [0 - 1.79769e+308]>\n"
    "[1e3] -> pass <1e3>\n"
    "[x] -> fail <Value x not in range [0 - 1.79769e+308]>\n"
    "[] -> fail <Value  not in range [0 - 1.79769e+308]>\n"
    "desc <>\n"
    "[plain] -> pass <plain>\n"
    "[\"a\\tb\"] -> pass <a\\tb>\n"
    "['a\\tb'] -> pass <a\\tb>\n"
    "[a\\nb] -> pass <a\\nb>\n"
    "[\"q\\x41\"] -> fail <unrecognized escape sequence \\x in q\\x41>\n"
    "[B\"(\\x41\\x42)\"] -> pass <AB>\n"
    "[\"bad\\q\"] -> fail <unrecognized escape sequence \\q in bad\\q>\n";

// SETTINGS
//   constant: what the steps of d_tests_arg_cpp_validator_settings write, as
// CLI11 writes them.
const char SETTINGS[] =
    "made: name=[loud] description=[LOUD] active=1 modifying=1 index=-1\n"
    "desc <LOUD>\n"
    "[abc] -> pass <ABC>\n"
    "[Mixed Case] -> pass <MIXED CASE>\n"
    "[a1] -> fail <no digits allowed in a1>\n"
    "[] -> pass <>\n"
    "renamed: name=[louder] description=[LOUDER] active=1 modifying=1 "
    "index=-1\n"
    "inactive: name=[louder] description=[] active=0 modifying=1 index=-1\n"
    "desc <>\n"
    "[abc] -> pass <abc>\n"
    "[Mixed Case] -> pass <Mixed Case>\n"
    "[a1] -> pass <a1>\n"
    "[] -> pass <>\n"
    "non-modifying: name=[louder] description=[LOUDER] active=1 modifying=0 "
    "index=-1\n"
    "desc <LOUDER>\n"
    "[abc] -> pass <abc>\n"
    "[Mixed Case] -> pass <Mixed Case>\n"
    "[a1] -> fail <no digits allowed in a1>\n"
    "[] -> pass <>\n"
    "indexed: name=[louder] description=[LOUDER] active=1 modifying=1 index=2\n"
    "default: name=[] description=[] active=1 modifying=1 index=-1\n"
    "desc <>\n"
    "[anything] -> pass <anything>\n"
    "[] -> pass <>\n"
    "described: name=[] description=[described only] active=1 modifying=1 "
    "index=-1\n"
    "desc <described only>\n"
    "[anything] -> pass <anything>\n"
    "[] -> pass <>\n"
    "first=[] given=[ABC] second=[] kept=[xyz] third=[no digits allowed in "
    "a1]\n"
    "fourth=[no digits allowed in AB1] changed=[AB1]\n";

// COMPOSED
//   constant: what the steps of d_tests_arg_cpp_validator_composed write, as
// CLI11 writes them.
const char COMPOSED[] =
    "either: name=[] description=[(INT in [1 - 5]) OR (INT in [10 - 15])] "
    "active=1 modifying=1 index=-1\n"
    "desc <(INT in [1 - 5]) OR (INT in [10 - 15])>\n"
    "[3] -> pass <3>\n"
    "[12] -> pass <12>\n"
    "[7] -> fail <(Value 7 not in range [1 - 5]) OR (Value 7 not in range [10 "
    "- 15])>\n"
    "[0] -> fail <(Value 0 not in range [1 - 5]) OR (Value 0 not in range [10 "
    "- 15])>\n"
    "[x] -> fail <(Value x not in range [1 - 5]) OR (Value x not in range [10 "
    "- 15])>\n"
    "both: name=[] description=[(INT in [1 - 10]) AND (POSITIVE)] active=1 "
    "modifying=1 index=-1\n"
    "desc <(INT in [1 - 10]) AND (POSITIVE)>\n"
    "[3] -> pass <3>\n"
    "[12] -> fail <Value 12 not in range [1 - 10]>\n"
    "[7] -> pass <7>\n"
    "[0] -> fail <(Value 0 not in range [1 - 10]) AND (Value 0 not in range "
    "[4.94066e-324 - 1.79769e+308])>\n"
    "[x] -> fail <(Value x not in range [1 - 10]) AND (Value x not in range "
    "[4.94066e-324 - 1.79769e+308])>\n"
    "outside: name=[] description=[NOT INT in [1 - 5]] active=1 modifying=1 "
    "index=-1\n"
    "desc <NOT INT in [1 - 5]>\n"
    "[3] -> fail <check INT in [1 - 5] succeeded improperly>\n"
    "[12] -> pass <12>\n"
    "[7] -> pass <7>\n"
    "[0] -> pass <0>\n"
    "[x] -> pass <x>\n"
    "neither: name=[] description=[NOT (INT in [1 - 5]) OR (INT in [10 - 15])] "
    "active=1 modifying=1 index=-1\n"
    "desc <NOT (INT in [1 - 5]) OR (INT in [10 - 15])>\n"
    "[3] -> fail <check (INT in [1 - 5]) OR (INT in [10 - 15]) succeeded "
    "improperly>\n"
    "[12] -> fail <check (INT in [1 - 5]) OR (INT in [10 - 15]) succeeded "
    "improperly>\n"
    "[7] -> pass <7>\n"
    "[0] -> pass <0>\n"
    "[x] -> pass <x>\n"
    "loud & marked: name=[] description=[(LOUD) AND (MARKED)] active=1 "
    "modifying=1 index=-1\n"
    "desc <(LOUD) AND (MARKED)>\n"
    "[ab] -> pass <AB!>\n"
    "[abc] -> pass <ABC!>\n"
    "[a1] -> fail <no digits allowed in a1>\n"
    "marked & loud: name=[] description=[(MARKED) AND (LOUD)] active=1 "
    "modifying=1 index=-1\n"
    "desc <(MARKED) AND (LOUD)>\n"
    "[ab] -> pass <AB!>\n"
    "[abc] -> pass <ABC!>\n"
    "[a1] -> fail <no digits allowed in a1!>\n"
    "loud | even: name=[] description=[(LOUD) OR (EVEN)] active=1 modifying=1 "
    "index=-1\n"
    "desc <(LOUD) OR (EVEN)>\n"
    "[ab] -> pass <AB>\n"
    "[abc] -> pass <ABC>\n"
    "[a1] -> pass <a1>\n"
    "!even: name=[] description=[NOT EVEN] active=1 modifying=1 index=-1\n"
    "desc <NOT EVEN>\n"
    "[ab] -> fail <check EVEN succeeded improperly>\n"
    "[abc] -> pass <abc>\n"
    "[a1] -> fail <check EVEN succeeded improperly>\n";

// IN_A_PARSE
//   constant: what the steps of d_tests_arg_cpp_validator_in_a_parse write,
// as CLI11 writes them.
const char IN_A_PARSE[] =
    "cmd app n=1 miss=[] subs=[] order=[--number|--word]\n"
    "opt number n=1 res=[5]\n"
    "opt word n=1 res=[ABC]\n"
    "ERR ValidationError 105: --number: Value 11 not in range [1 - 10]\n"
    "cmd app n=1 miss=[] subs=[] order=[--number]\n"
    "opt number n=1 res=[11]\n"
    "opt word n=0 res=[]\n"
    "ERR ValidationError 105: --word: no digits allowed in a1\n"
    "cmd app n=1 miss=[] subs=[] order=[--word]\n"
    "opt number n=0 res=[]\n"
    "opt word n=1 res=[a1]\n"
    "cmd app n=1 miss=[] subs=[] order=[--list|--list]\n"
    "opt list n=2 res=[9|3]\n"
    "opt word n=0 res=[]\n"
    "ERR ValidationError 105: --list: Value 9 not in range [1 - 5]\n"
    "cmd app n=1 miss=[] subs=[] order=[--list|--list]\n"
    "opt list n=2 res=[1|9]\n"
    "opt word n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[--word]\n"
    "opt list n=0 res=[]\n"
    "opt word n=1 res=[abc]\n"
    "cmd app n=1 miss=[] subs=[] order=[--word]\n"
    "opt list n=0 res=[]\n"
    "opt word n=1 res=[ABC]\n"
    "cmd app n=1 miss=[] subs=[] order=[--list|--list]\n"
    "opt list n=2 res=[1|9]\n"
    "opt word n=0 res=[]\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// EXTRA
//   constant: what the steps of d_tests_arg_cpp_validator_extra write, as
// CLI11 writes them.
const char EXTRA[] =
    "desc <INT>\n"
    "[12] -> pass <12>\n"
    "[-3] -> pass <-3>\n"
    "[1.5] -> fail <Failed parsing 1.5 as a INT>\n"
    "[300] -> pass <300>\n"
    "[1e2] -> fail <Failed parsing 1e2 as a INT>\n"
    "[x] -> fail <Failed parsing x as a INT>\n"
    "[] -> fail <Failed parsing  as a INT>\n"
    "desc <FLOAT>\n"
    "[12] -> pass <12>\n"
    "[-3] -> pass <-3>\n"
    "[1.5] -> pass <1.5>\n"
    "[300] -> pass <300>\n"
    "[1e2] -> pass <1e2>\n"
    "[x] -> fail <Failed parsing x as a FLOAT>\n"
    "[] -> fail <Failed parsing  as a FLOAT>\n"
    "desc <UINT>\n"
    "[12] -> pass <12>\n"
    "[-3] -> fail <Failed parsing -3 as a UINT>\n"
    "[1.5] -> fail <Failed parsing 1.5 as a UINT>\n"
    "[300] -> fail <Failed parsing 300 as a UINT>\n"
    "[1e2] -> fail <Failed parsing 1e2 as a UINT>\n"
    "[x] -> fail <Failed parsing x as a UINT>\n"
    "[] -> fail <Failed parsing  as a UINT>\n"
    "desc <TEXT>\n"
    "[12] -> pass <12>\n"
    "[-3] -> pass <-3>\n"
    "[1.5] -> pass <1.5>\n"
    "[300] -> pass <300>\n"
    "[1e2] -> pass <1e2>\n"
    "[x] -> pass <x>\n"
    "[] -> pass <>\n"
    "desc <TEXT>\n"
    "[3,4] -> pass <3,4>\n"
    "[3;4] -> fail <Failed parsing 3;4 as a TEXT>\n"
    "[x] -> fail <Failed parsing x as a TEXT>\n"
    "desc <NUMBER>\n"
    "[12] -> pass <12>\n"
    "[-3] -> pass <-3>\n"
    "[1.5] -> pass <1.5>\n"
    "[300] -> pass <300>\n"
    "[1e2] -> pass <1e2>\n"
    "[x] -> fail <Failed parsing x as a FLOAT>\n"
    "[] -> fail <Failed parsing  as a FLOAT>\n"
    "desc <INT bounded to [1 - 10]>\n"
    "[0] -> pass <1>\n"
    "[5] -> pass <5>\n"
    "[11] -> pass <10>\n"
    "[-7] -> pass <1>\n"
    "[2.75] -> fail <Value 2.75 could not be converted>\n"
    "[x] -> fail <Value x could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "desc <UINT bounded to [0 - 10]>\n"
    "[0] -> pass <0>\n"
    "[5] -> pass <5>\n"
    "[11] -> pass <10>\n"
    "[-7] -> fail <Value -7 could not be converted>\n"
    "[2.75] -> fail <Value 2.75 could not be converted>\n"
    "[x] -> fail <Value x could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "desc <INT bounded to [-5 - 5]>\n"
    "[0] -> pass <0>\n"
    "[5] -> pass <5>\n"
    "[11] -> pass <5>\n"
    "[-7] -> pass <-5>\n"
    "[2.75] -> fail <Value 2.75 could not be converted>\n"
    "[x] -> fail <Value x could not be converted>\n"
    "[] -> fail <Value  could not be converted>\n"
    "desc <FLOAT bounded to [0.5 - 2.5]>\n"
    "[0.25] -> pass <0.5>\n"
    "[1.5] -> pass <1.5>\n"
    "[3] -> pass <2.5>\n"
    "[x] -> fail <Value x could not be converted>\n"
    "desc <IPV4>\n"
    "[1.2.3.4] -> pass <1.2.3.4>\n"
    "[0.0.0.0] -> pass <0.0.0.0>\n"
    "[255.255.255.255] -> pass <255.255.255.255>\n"
    "[256.1.1.1] -> fail <Each IP number must be between 0 and 255 256>\n"
    "[1.2.3] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "[1.2.3.4.5] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "[a.b.c.d] -> fail <Failed parsing number (a)>\n"
    "[1..2.3] -> fail <Failed parsing number ()>\n"
    "[-1.2.3.4] -> fail <Each IP number must be between 0 and 255 -1>\n"
    "[] -> fail <Invalid IPV4 address: must have 3 separators>\n"
    "cmd app n=1 miss=[] subs=[] order=[--count|--size|--choice]\n"
    "opt count n=1 res=[10]\n"
    "opt size n=1 res=[2048]\n"
    "opt choice n=1 res=[b]\n"
    "ERR ValidationError 105: --choice: d not in {a,b}\n"
    "cmd app n=1 miss=[] subs=[] order=[--choice]\n"
    "opt count n=0 res=[]\n"
    "opt size n=0 res=[]\n"
    "opt choice n=1 res=[d]\n";

// MEMBER
//   constant: what the steps of d_tests_arg_cpp_validator_member write, as
// CLI11 writes them.
const char MEMBER[] =
    "desc <{alpha,beta}>\n"
    "[alpha] -> pass <alpha>\n"
    "[beta] -> pass <beta>\n"
    "[gamma] -> fail <gamma not in {alpha,beta}>\n"
    "[Alpha] -> fail <Alpha not in {alpha,beta}>\n"
    "[] -> fail < not in {alpha,beta}>\n"
    "desc <{1,2,10}>\n"
    "[1] -> pass <1>\n"
    "[10] -> pass <10>\n"
    "[01] -> pass <01>\n"
    "[3] -> fail <3 not in {1,2,10}>\n"
    "[x] -> status <x>\n"
    "desc <{one,two}>\n"
    "[one] -> pass <one>\n"
    "[1] -> fail <1 not in {one,two}>\n"
    "[three] -> fail <three not in {one,two}>\n"
    "desc <{Alpha,Beta_Value,Gamma Ray}>\n"
    "[alpha] -> pass <Alpha>\n"
    "[ALPHA] -> pass <Alpha>\n"
    "[betavalue] -> fail <betavalue not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[beta_value] -> pass <Beta_Value>\n"
    "[Gamma Ray] -> pass <Gamma Ray>\n"
    "[gammaray] -> fail <gammaray not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[GAMMARAY] -> fail <GAMMARAY not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[-Alpha] -> fail <-Alpha not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[delta] -> fail <delta not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "desc <{Alpha,Beta_Value,Gamma Ray}>\n"
    "[alpha] -> pass <Alpha>\n"
    "[ALPHA] -> pass <Alpha>\n"
    "[betavalue] -> pass <Beta_Value>\n"
    "[beta_value] -> pass <Beta_Value>\n"
    "[Gamma Ray] -> pass <Gamma Ray>\n"
    "[gammaray] -> fail <gammaray not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[GAMMARAY] -> fail <GAMMARAY not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[-Alpha] -> fail <-Alpha not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[delta] -> fail <delta not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "desc <{Alpha,Beta_Value,Gamma Ray}>\n"
    "[alpha] -> fail <alpha not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[ALPHA] -> fail <ALPHA not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[betavalue] -> fail <betavalue not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[beta_value] -> fail <beta_value not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[Gamma Ray] -> pass <Gamma Ray>\n"
    "[gammaray] -> fail <gammaray not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[GAMMARAY] -> fail <GAMMARAY not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[-Alpha] -> fail <-Alpha not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[delta] -> fail <delta not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "desc <{Alpha,Beta_Value,Gamma Ray}>\n"
    "[alpha] -> pass <Alpha>\n"
    "[ALPHA] -> pass <Alpha>\n"
    "[betavalue] -> pass <Beta_Value>\n"
    "[beta_value] -> pass <Beta_Value>\n"
    "[Gamma Ray] -> pass <Gamma Ray>\n"
    "[gammaray] -> pass <Gamma Ray>\n"
    "[GAMMARAY] -> pass <Gamma Ray>\n"
    "[-Alpha] -> fail <-Alpha not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[delta] -> fail <delta not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "desc <{Alpha,Beta_Value,Gamma Ray}>\n"
    "[alpha] -> fail <alpha not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[ALPHA] -> fail <ALPHA not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[betavalue] -> fail <betavalue not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[beta_value] -> fail <beta_value not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[Gamma Ray] -> pass <Gamma Ray>\n"
    "[gammaray] -> fail <gammaray not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[GAMMARAY] -> fail <GAMMARAY not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "[-Alpha] -> pass <Alpha>\n"
    "[delta] -> fail <delta not in {Alpha,Beta_Value,Gamma Ray}>\n"
    "desc <{1,2,10}>\n"
    "[2] -> pass <2>\n"
    "[02] -> pass <02>\n"
    "[2.0] -> status <2.0>\n"
    "[3] -> fail <3 not in {1,2,10}>\n"
    "[x] -> status <x>\n"
    "desc <{0.5,1.5}>\n"
    "[0.5] -> pass <0.5>\n"
    "[0.50] -> pass <0.50>\n"
    "[5e-1] -> pass <5e-1>\n"
    "[1] -> fail <1 not in {0.5,1.5}>\n"
    "[x] -> status <x>\n";

// TRANSFORM
//   constant: what the steps of d_tests_arg_cpp_validator_transform write,
// as CLI11 writes them.
const char TRANSFORM[] =
    "desc <{one->1,two->2}>\n"
    "[one] -> pass <1>\n"
    "[two] -> pass <2>\n"
    "[1] -> pass <1>\n"
    "[three] -> pass <three>\n"
    "[] -> pass <>\n"
    "desc <value in {one->1,two->2} OR {1,2}>\n"
    "[one] -> pass <1>\n"
    "[two] -> pass <2>\n"
    "[1] -> pass <1>\n"
    "[three] -> fail <Check three value in {one->1,two->2} OR {1,2} FAILED>\n"
    "[] -> fail <Check  value in {one->1,two->2} OR {1,2} FAILED>\n"
    "desc <{low->0,high->9}>\n"
    "[low] -> pass <0>\n"
    "[high] -> pass <9>\n"
    "[9] -> pass <9>\n"
    "[mid] -> pass <mid>\n"
    "desc <value in {low->0,high->9} OR {0,9}>\n"
    "[low] -> pass <0>\n"
    "[high] -> pass <9>\n"
    "[9] -> pass <9>\n"
    "[mid] -> fail <Check mid value in {low->0,high->9} OR {0,9} FAILED>\n"
    "desc <{one->1,ten->10}>\n"
    "[ten] -> pass <10>\n"
    "[10] -> pass <10>\n"
    "[eleven] -> pass <eleven>\n"
    "desc <value in {one->1,ten->10} OR {1,10}>\n"
    "[ten] -> pass <10>\n"
    "[10] -> pass <10>\n"
    "[eleven] -> fail <Check eleven value in {one->1,ten->10} OR {1,10} "
    "FAILED>\n"
    "desc <{one->1,two->2}>\n"
    "[ONE] -> pass <1>\n"
    "[One] -> pass <1>\n"
    "[O_NE] -> pass <O_NE>\n"
    "[o ne] -> pass <o ne>\n"
    "[1] -> pass <1>\n"
    "[x] -> pass <x>\n"
    "desc <value in {one->1,two->2} OR {1,2}>\n"
    "[ONE] -> pass <1>\n"
    "[One] -> pass <1>\n"
    "[O_NE] -> pass <1>\n"
    "[o ne] -> fail <Check o ne value in {one->1,two->2} OR {1,2} FAILED>\n"
    "[1] -> pass <1>\n"
    "[x] -> fail <Check x value in {one->1,two->2} OR {1,2} FAILED>\n"
    "desc <value in {one->1,two->2} OR {1,2}>\n"
    "[ONE] -> pass <1>\n"
    "[One] -> pass <1>\n"
    "[O_NE] -> pass <1>\n"
    "[o ne] -> pass <1>\n"
    "[1] -> pass <1>\n"
    "[x] -> fail <Check x value in {one->1,two->2} OR {1,2} FAILED>\n";

// UNITS
//   constant: what the steps of d_tests_arg_cpp_validator_units write, as
// CLI11 writes them.
const char UNITS[] =
    "desc <INT [UNIT]>\n"
    "[10] -> pass <10>\n"
    "[10kb] -> pass <10240>\n"
    "[10 KB] -> pass <10240>\n"
    "[10KB] -> pass <10240>\n"
    "[2mb] -> pass <2097152>\n"
    "[4096mb] -> status <4096 multiplied by mb factor would cause number "
    "overflow. Use smaller value.>\n"
    "[1.5kb] -> status <Value 1.5 could not be converted to INT>\n"
    "[kb] -> pass <1024>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[x] -> status <x unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[] -> status <Input is empty>\n"
    "desc <INT SIZE>\n"
    "[10] -> status <Missing mandatory unit>\n"
    "[10kb] -> pass <10240>\n"
    "[10 KB] -> status <KB unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[10KB] -> status <KB unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[2mb] -> pass <2097152>\n"
    "[4096mb] -> status <4096 multiplied by mb factor would cause number "
    "overflow. Use smaller value.>\n"
    "[1.5kb] -> status <Value 1.5 could not be converted to INT>\n"
    "[kb] -> pass <1024>\n"
    "[10x] -> status <x unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[x] -> status <x unit not recognized. Allowed values: {b,kb,mb}>\n"
    "[] -> status <Input is empty>\n"
    "desc <FLOAT [UNIT]>\n"
    "[1.5km] -> pass <1500>\n"
    "[2 m] -> pass <2>\n"
    "[3] -> pass <3>\n"
    "[x] -> status <x unit not recognized. Allowed values: {km,m}>\n"
    "desc <SIZE [b, kb(=1000b), kib(=1024b), ...]>\n"
    "[1k] -> pass <1000>\n"
    "[1kb] -> pass <1000>\n"
    "[1KiB] -> pass <1024>\n"
    "[2 MB] -> pass <2000000>\n"
    "[1.5k] -> status <Value 1.5 could not be converted to UINT>\n"
    "[1ki] -> pass <1024>\n"
    "[10] -> pass <10>\n"
    "[x] -> status <x unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n"
    "desc <SIZE [b, kb(=1024b), ...]>\n"
    "[1k] -> pass <1024>\n"
    "[1kb] -> pass <1024>\n"
    "[1KiB] -> pass <1024>\n"
    "[2 MB] -> pass <2097152>\n"
    "[1.5k] -> status <Value 1.5 could not be converted to UINT>\n"
    "[1ki] -> pass <1024>\n"
    "[10] -> pass <10>\n"
    "[x] -> status <x unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}>\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// PATHS
//   constant: what the steps of d_tests_arg_cpp_validator_paths write, as
// CLI11 writes them on POSIX.
const char PATHS[] =
    "desc <FILE>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[folder] -> fail <File is actually a directory: folder>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n"
    "desc <DIR>\n"
    "[data.txt] -> fail <Directory is actually a file: data.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Directory does not exist: missing.txt>\n"
    "desc <PATH(existing)>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <PATH(non-existing)>\n"
    "[data.txt] -> fail <Path already exists: data.txt>\n"
    "[folder] -> fail <Path already exists: folder>\n"
    "[missing.txt] -> pass <missing.txt>\n"
    "desc <FILE>\n"
    "[inner.txt] -> pass <folder/inner.txt>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n"
    "desc <FILE>\n"
    "[inner.txt] -> pass <folder/inner.txt>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[missing.txt] -> pass <missing.txt>\n"
    "desc <Path with read permission>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> pass <empty.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <Path with write permission>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> pass <empty.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <Path with exec permission>\n"
    "[data.txt] -> fail <Path does not have required permissions: data.txt>\n"
    "[empty.txt] -> fail <Path does not have required permissions: empty.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <File size at least 1 bytes>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> fail <File size 0 bytes is less than minimum 1 bytes>\n"
    "[folder] -> fail <Error getting file size: Is a directory>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// PATHS_WINDOWS
//   constant: PATHS on Windows, which grants execution to everything that
// exists, as CLI11 reads Microsoft's std::filesystem (ISSUES 26).
const char PATHS_WINDOWS[] =
    "desc <FILE>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[folder] -> fail <File is actually a directory: folder>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n"
    "desc <DIR>\n"
    "[data.txt] -> fail <Directory is actually a file: data.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Directory does not exist: missing.txt>\n"
    "desc <PATH(existing)>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <PATH(non-existing)>\n"
    "[data.txt] -> fail <Path already exists: data.txt>\n"
    "[folder] -> fail <Path already exists: folder>\n"
    "[missing.txt] -> pass <missing.txt>\n"
    "desc <FILE>\n"
    "[inner.txt] -> pass <folder/inner.txt>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n"
    "desc <FILE>\n"
    "[inner.txt] -> pass <folder/inner.txt>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[missing.txt] -> pass <missing.txt>\n"
    "desc <Path with read permission>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> pass <empty.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <Path with write permission>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> pass <empty.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <Path with exec permission>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> pass <empty.txt>\n"
    "[folder] -> pass <folder>\n"
    "[missing.txt] -> fail <Path does not exist: missing.txt>\n"
    "desc <File size at least 1 bytes>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> fail <File size 0 bytes is less than minimum 1 bytes>\n"
    "[folder] -> fail <Error getting file size: Is a directory>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";

// PATH_SIZES
//   constant: what the steps of d_tests_arg_cpp_validator_path_sizes write,
// as CLI11 writes them; where no 64-bit integer is declared, the absence of
// file_size, the module's own.
#if defined(UINT64_MAX)
const char PATH_SIZES[] =
    "desc <File size between 1 and 10 bytes>\n"
    "[data.txt] -> pass <data.txt>\n"
    "[empty.txt] -> fail <File size 0 bytes is less than minimum 1 bytes>\n"
    "[folder] -> fail <Error getting file size: Is a directory>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n"
    "desc <File size at least 6 bytes>\n"
    "[data.txt] -> fail <File size 5 bytes is less than minimum 6 bytes>\n"
    "[empty.txt] -> fail <File size 0 bytes is less than minimum 6 bytes>\n"
    "[folder] -> fail <Error getting file size: Is a directory>\n"
    "[missing.txt] -> fail <File does not exist: missing.txt>\n";
#else
const char PATH_SIZES[] =
    "file_size: no 64-bit integer is declared here\n";
#endif  // UINT64_MAX

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// CPP11
//   constant: what the steps of d_tests_arg_cpp_validator_cpp11 write, as
// CLI11 writes them.
const char CPP11[] =
    "desc <ENUM in [1 - 2]>\n"
    "[1] -> pass <1>\n"
    "[2] -> pass <2>\n"
    "[5] -> fail <Value 5 not in range [1 - 2]>\n"
    "[x] -> fail <Value x not in range [1 - 2]>\n"
    "desc <ENUM in [1 - 2]>\n"
    "[1] -> pass <1>\n"
    "[2] -> pass <2>\n"
    "[5] -> fail <Value 5 not in range [1 - 2]>\n"
    "[x] -> fail <Value x not in range [1 - 2]>\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// CPP11_EXTRA
//   constant: what the steps of d_tests_arg_cpp_validator_cpp11_extra write,
// as CLI11 writes them.
const char CPP11_EXTRA[] =
    "desc <ENUM bounded to [1 - 2]>\n"
    "[1] -> pass <1>\n"
    "[2] -> pass <2>\n"
    "[5] -> pass <2>\n"
    "[x] -> fail <Value x could not be converted>\n"
    "desc <ENUM bounded to [1 - 2]>\n"
    "[1] -> pass <1>\n"
    "[2] -> pass <2>\n"
    "[5] -> pass <2>\n"
    "[x] -> fail <Value x could not be converted>\n"
    "desc <{red,green}>\n"
    "[red] -> pass <red>\n"
    "[Red] -> fail <Red not in {red,green}>\n"
    "[blue] -> fail <blue not in {red,green}>\n"
    "desc <{Red,Green}>\n"
    "[red] -> pass <Red>\n"
    "[Red] -> pass <Red>\n"
    "[blue] -> fail <blue not in {Red,Green}>\n"
    "desc <{1,2}>\n"
    "[1] -> pass <1>\n"
    "[3] -> fail <3 not in {1,2}>\n"
    "[x] -> status <x>\n"
    "desc <{one->1}>\n"
    "[one] -> pass <1>\n"
    "[1] -> pass <1>\n"
    "[two] -> pass <two>\n"
    "desc <value in {one->1} OR {1}>\n"
    "[one] -> pass <1>\n"
    "[1] -> pass <1>\n"
    "[two] -> fail <Check two value in {one->1} OR {1} FAILED>\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// VALUE
//   constant: what the steps of d_tests_arg_cpp_validator_value write: the
// module's own, as CLI11's Validator is an object of its own, not a C
// validator to swap with, hand over, or run as C runs one.
const char VALUE[] =
    "first: name=[first] description=[INT in [1 - 10]] active=1 modifying=1 "
    "index=-1\n"
    "second: name=[second] description=[second's own] active=1 modifying=1 "
    "index=-1\n"
    "third: name=[first] description=[INT in [1 - 10]] active=1 modifying=1 "
    "index=-1\n"
    "desc <INT in [1 - 10]>\n"
    "[5] -> pass <5>\n"
    "[11] -> fail <Value 11 not in range [1 - 10]>\n"
    "first: name=[second] description=[POSITIVE] active=1 modifying=1 "
    "index=-1\n"
    "second: name=[first] description=[INT in [1 - 10]] active=1 modifying=1 "
    "index=-1\n"
    "detached: name=[] description=[] active=1 modifying=1 index=-1\n"
    "raw: name=[second] status=OK passed=0 reason=[Value -1 not in range "
    "[4.94066e-324 - 1.79769e+308]]\n"
    "kept: name=[added] description=[ADDED] active=1 modifying=1 index=-1\n"
    "desc <ADDED>\n"
    "[ab] -> pass <ab+>\n"
    "desc <ADDED>\n"
    "[ab] -> pass <ab+>\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// VALUE_EXTRA
//   constant: what the steps of d_tests_arg_cpp_validator_value_extra write:
// the module's own, as CLI11's IsMember takes no C array.
const char VALUE_EXTRA[] =
    "desc <{red,green}>\n"
    "[red] -> pass <red>\n"
    "[RED] -> fail <RED not in {red,green}>\n"
    "[1] -> fail <1 not in {red,green}>\n"
    "[3] -> fail <3 not in {red,green}>\n"
    "desc <{red,green}>\n"
    "[red] -> pass <red>\n"
    "[RED] -> pass <red>\n"
    "[1] -> fail <1 not in {red,green}>\n"
    "[3] -> fail <3 not in {red,green}>\n"
    "desc <{1,2}>\n"
    "[red] -> status <red>\n"
    "[RED] -> status <RED>\n"
    "[1] -> pass <1>\n"
    "[3] -> fail <3 not in {1,2}>\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// HANDLES
//   constant: what the steps of d_tests_arg_cpp_validator_handles write: the
// module's own, as a CLI11 option gives a Validator*, not a handle to test,
// copy into a validator of its own, or set through in a chain.
const char HANDLES[] =
    "ranged: valid=1 name=[ranged] description=[INT in [1 - 10]] active=1 "
    "modifying=0 index=-1 status=OK\n"
    "second: valid=1 name=[] description=[POSITIVE] active=1 modifying=0 "
    "index=-1 status=OK\n"
    "first: valid=1 name=[] description=[POSITIVE] active=1 modifying=0 "
    "index=-1 status=OK\n"
    "set: valid=1 name=[ranged] description=[] active=0 modifying=0 index=0 "
    "status=OK\n"
    "same=1\n"
    "copied: name=[ranged] description=[changed] active=1 modifying=0 index=0\n"
    "desc <changed>\n"
    "[5] -> pass <5>\n"
    "[11] -> fail <Value 11 not in range [1 - 10]>\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// MOVED
//   constant: what the steps of d_tests_arg_cpp_validator_moved write: the
// module's own, as what a moved CLI11 Validator holds is unspecified, where
// a moved validator here is the default validator.
const char MOVED[] =
    "first, moved from: name=[] description=[] active=1 modifying=1 index=-1\n"
    "second: name=[first] description=[INT in [1 - 10]] active=1 modifying=1 "
    "index=-1\n"
    "second, moved from: name=[] description=[] active=1 modifying=1 index=-1\n"
    "third: name=[first] description=[INT in [1 - 10]] active=1 modifying=1 "
    "index=-1\n"
    "small, moved from: name=[] description=[] active=1 modifying=1 index=-1\n"
    "either: name=[] description=[(INT in [1 - 5]) OR (INT in [10 - 15])] "
    "active=1 modifying=1 index=-1\n"
    "desc <(INT in [1 - 5]) OR (INT in [10 - 15])>\n"
    "[3] -> pass <3>\n"
    "[12] -> pass <12>\n"
    "[7] -> fail <(Value 7 not in range [1 - 5]) OR (Value 7 not in range [10 "
    "- 15])>\n"
    "third, moved into the option: name=[] description=[] active=1 modifying=1 "
    "index=-1\n"
    "held: INT in [1 - 10]\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// FILTERS
//   constant: what the steps of d_tests_arg_cpp_validator_filters write: the
// module's own, as CLI11's filters are std::function objects, not values
// with flags to read or combine with `|`.
const char FILTERS[] =
    "none: flags=0 function=0\n"
    "case: flags=1 function=0\n"
    "case, called: flags=1 function=0\n"
    "underscore: flags=2 function=0\n"
    "space: flags=4 function=0\n"
    "case | underscore: flags=3 function=0\n"
    "function: flags=0 function=1\n"
    "function | space: flags=4 function=1\n"
    "left, swapped: flags=0 function=1\n"
    "right, swapped: flags=1 function=0\n"
    "desc <{Alpha,Beta_Value}>\n"
    "[ALPHA] -> pass <Alpha>\n"
    "[betavalue] -> pass <Beta_Value>\n"
    "[-+Alpha] -> fail <-+Alpha not in {Alpha,Beta_Value}>\n"
    "[+-Alpha] -> fail <+-Alpha not in {Alpha,Beta_Value}>\n"
    "[x] -> fail <x not in {Alpha,Beta_Value}>\n"
    "desc <{Alpha,Beta_Value}>\n"
    "[ALPHA] -> fail <ALPHA not in {Alpha,Beta_Value}>\n"
    "[betavalue] -> fail <betavalue not in {Alpha,Beta_Value}>\n"
    "[-+Alpha] -> pass <Alpha>\n"
    "[+-Alpha] -> fail <+-Alpha not in {Alpha,Beta_Value}>\n"
    "[x] -> fail <x not in {Alpha,Beta_Value}>\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// DEPARTED
//   constant: what the steps of d_tests_arg_cpp_validator_departures write:
// the module's own, where the face departs from CLI11. CLI11's
// get_validator throws OptionNotFound for a validator an option does not
// have -- "Validator nope Not Found not found", "Validator index is not
// valid not found" -- where the face gives a NULL handle (ISSUES 50); a
// transformation's function that changes a value and then rejects it
// leaves the change in CLI11's option -- `AB1` for `ab1` -- where here the
// result stays as it was given (ISSUES 37), though the value operator() is
// called on keeps it in both, as SETTINGS shows; and CLI11 streams a
// range's bounds over an 8-bit integer as characters, `INT in [` with a
// NUL and a newline for one from 0 to 10 and `UINT in [A - d]` for one from
// 65 to 100, where here they are numbers (ISSUES 23).
const char DEPARTED[] =
    "by name: valid=0 name=[] description=[] active=1 modifying=1 index=-1 "
    "status=OK\n"
    "by index: valid=0 name=[] description=[] active=1 modifying=1 index=-1 "
    "status=OK\n"
    "empty: valid=0 name=[] description=[] active=1 modifying=1 index=-1 "
    "status=OK\n"
    "ran: reason=[] value=[x]\n"
    "copied: name=[] description=[] active=1 modifying=1 index=-1\n"
    "ERR ValidationError 105: --word: no digits allowed in AB1\n"
    "cmd app n=1 miss=[] subs=[] order=[--word]\n"
    "opt word n=1 res=[ab1]\n"
    "desc <INT in [0 - 10]>\n"
    "[5] -> pass <5>\n"
    "[11] -> fail <Value 11 not in range [0 - 10]>\n"
    "desc <UINT in [65 - 100]>\n"
    "[5] -> fail <Value 5 not in range [65 - 100]>\n"
    "[11] -> fail <Value 11 not in range [65 - 100]>\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// DEPARTED_EXTRA
//   constant: what the steps of d_tests_arg_cpp_validator_departures_extra
// write: the module's own, where the face's extra validators depart from
// CLI11's. CLI11's ValidIPV4 reads each part as a flag's number, so that
// `true.0.0.1` passes (ISSUES 24); its Bound writes a value it clamps to
// every digit max_digits10 allows -- 0.05 into [0.1, 0.3] as
// `0.10000000000000001` -- and an 8-bit integer as a character, its bounds
// as `[A - d]` and 120 clamped to `d` (ISSUES 23), and its AsNumberWithUnit
// writes a float factor's product the same way, `0.1 x` with x as 3 as
// `0.300000012`; it leaves a value AsSizeValue rejects cut, `1` for `1zb`
// (ISSUES 37); and it throws ValidationError from AsNumberWithUnit's
// constructor for a unit it refuses, where here the validator made is a
// failed one, which an option records as its definition error (ISSUES 46).
const char DEPARTED_EXTRA[] =
    "desc <IPV4>\n"
    "[true.0.0.1] -> fail <Failed parsing number (true)>\n"
    "[1.2.3.4] -> pass <1.2.3.4>\n"
    "desc <FLOAT bounded to [0.1 - 0.3]>\n"
    "[0.05] -> pass <0.1>\n"
    "[0.2] -> pass <0.2>\n"
    "desc <UINT bounded to [65 - 100]>\n"
    "[120] -> pass <100>\n"
    "[70] -> pass <70>\n"
    "desc <FLOAT [UNIT]>\n"
    "[0.1 x] -> pass <0.3>\n"
    "[2 x] -> pass <6>\n"
    "ERR ValidationError 105: --size: zb unit not recognized. Allowed values: "
    "{b,e,eb,ei,eib,g,gb,gi,gib,k,kb,ki,kib,m,mb,mi,mib,p,pb,pi,pib,t,tb,ti,tib"
    "}\n"
    "cmd app n=1 miss=[] subs=[] order=[--size]\n"
    "opt size n=1 res=[1zb]\n"
    "failed: name=[] description=[] active=1 modifying=1 index=-1\n"
    "failed status=ValidationError\n"
    "reason kept=[Unit must contain only letters.]\n"
    "THROWN ValidationError: Unit must contain only letters.\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// DEPARTED_CPP11
//   constant: what the steps of d_tests_arg_cpp_validator_departures_cpp11
// write: the module's own, where a range over an enumeration departs from
// CLI11's: CLI11 writes an enumeration with an operator<< of its own
// through it -- `ENUM in [pale - deep]` -- where here every bound is written
// as the number the enumeration converts through, as CLI11 writes any other
// enumeration (ISSUES 53).
const char DEPARTED_CPP11[] =
    "its own text: pale,deep\n"
    "desc <ENUM in [0 - 3]>\n"
    "[1] -> pass <1>\n"
    "[2] -> pass <2>\n"
    "[5] -> fail <Value 5 not in range [0 - 3]>\n";

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// DEPARTED_CPP11_EXTRA
//   constant: what the steps of
// d_tests_arg_cpp_validator_departures_cpp11_extra write: the module's own,
// where a bound over an enumeration departs from CLI11's: CLI11 writes an
// enumeration with an operator<< of its own through it -- `ENUM bounded to
// [pale - deep]`, 5 clamped to `deep` -- where here every bound, and every
// value a bound clamps, is written as the number the enumeration converts
// through, as CLI11 writes any other enumeration (ISSUES 53).
const char DEPARTED_CPP11_EXTRA[] =
    "desc <ENUM bounded to [0 - 3]>\n"
    "[1] -> pass <1>\n"
    "[2] -> pass <2>\n"
    "[5] -> pass <3>\n";

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

// 1.1    Writers and functions
//------------------------------------------------------------------------------

/**
 * @brief Writes a validator's name, description and settings on one line.
 *
 * @param[in,out] _run        the run.
 * @param[in]     _label      what the line names the validator by.
 * @param[in]     _validator  the validator.
 */
void
write_validator(
    run&                  _run,
    const char*           _label,
    const cli::validator& _validator
)
{
    std::ostringstream line;

    line << _label
         << ": name=[" << _validator.get_name()
         << "] description=[" << escape(_validator.get_description())
         << "] active=" << _validator.get_active()
         << " modifying=" << _validator.get_modifying()
         << " index=" << _validator.get_application_index();
    _run.note(line.str());

    return;
}

// shouted
//   struct: a validator's function, as CLI11's Validator(op, desc, name)
// takes one: a value with a digit is rejected as it was given; any other
// passes, its letters made upper case.
struct shouted
{
    /**
     * @brief Judges a value, and makes its letters upper case.
     *
     * @param[in,out] _value  the value.
     * @return "" to pass it; the reason it is rejected.
     */
    std::string
    operator()(
        std::string& _value
    ) const
    {
        // a digit anywhere rejects the value, unchanged
        if (_value.find_first_of("0123456789") != std::string::npos)
        {
            return "no digits allowed in " + _value;
        }

        // each lower-case letter made upper case
        for (std::size_t index = 0u; index < _value.size(); ++index)
        {
            const char letter = _value[index];

            // a lower-case letter
            if ( (letter >= 'a') &&
                 (letter <= 'z') )
            {
                _value[index] = static_cast<char>(letter - 'a' + 'A');
            }
        }

        return std::string();
    }
};

// suffixed
//   struct: a validator's function that appends its text to every value.
struct suffixed
{
    const char* m_text;

    /**
     * @brief Appends the text.
     *
     * @param[in,out] _value  the value.
     * @return "": every value passes.
     */
    std::string
    operator()(
        std::string& _value
    ) const
    {
        _value += m_text;

        return std::string();
    }
};

// even_length
//   struct: a validator's function that rejects a value of an odd length.
struct even_length
{
    /**
     * @brief Judges a value's length.
     *
     * @param[in,out] _value  the value; unchanged.
     * @return "" for an even length; the reason otherwise.
     */
    std::string
    operator()(
        std::string& _value
    ) const
    {
        return ((_value.size() % 2u) == 0u)
               ? std::string()
               : (_value + " has an odd length");
    }
};

// half_done
//   struct: a validator's function that makes a value's letters upper case
// and only then rejects it for a digit.
struct half_done
{
    /**
     * @brief Changes a value, then judges it.
     *
     * @param[in,out] _value  the value.
     * @return "" to pass it; the reason it is rejected.
     */
    std::string
    operator()(
        std::string& _value
    ) const
    {
        // each lower-case letter made upper case
        for (std::size_t index = 0u; index < _value.size(); ++index)
        {
            const char letter = _value[index];

            // a lower-case letter
            if ( (letter >= 'a') &&
                 (letter <= 'z') )
            {
                _value[index] = static_cast<char>(letter - 'a' + 'A');
            }
        }

        return (_value.find_first_of("0123456789") != std::string::npos)
               ? ("no digits allowed in " + _value)
               : std::string();
    }
};

// dashless
//   struct: a filter, as CLI11's sets take one: a leading `-` dropped.
struct dashless
{
    /**
     * @brief Filters a value.
     *
     * @param[in] _value  the value.
     * @return the value without its leading dash.
     */
    std::string
    operator()(
        std::string _value
    ) const
    {
        // a leading dash dropped
        if ( (!_value.empty()) &&
             (_value[0] == '-') )
        {
            _value.erase(0u,
                         1u);
        }

        return _value;
    }
};

// 1.2    Default validators
//------------------------------------------------------------------------------

/**
 * @brief DEFAULTS 1: range over integers, real numbers and unsigned ones,
 *        between two bounds and from 0, described by CLI11 and by the
 *        caller.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_ranges(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "5", "1", "10", "0", "11", "-3", "2.5", "x", ""
    };

    transcribe(_run,
               cli::range(1,
                          10),
               VALUES,
               9u);
    transcribe(_run,
               cli::range(0.5,
                          2.5),
               VALUES,
               9u);
    transcribe(_run,
               cli::range(10u),
               VALUES,
               9u);
    transcribe(_run,
               cli::range(-5,
                          5,
                          "from -5 to 5"),
               VALUES,
               9u);

    return;
}

/**
 * @brief DEFAULTS 2: positive and non-negative numbers.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_numbers(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "1", "0", "-1", "0.5", "-0.5", "1e3", "x", ""
    };

    transcribe(_run,
               cli::positive_number(),
               VALUES,
               8u);
    transcribe(_run,
               cli::non_negative_number(),
               VALUES,
               8u);

    return;
}

/**
 * @brief DEFAULTS 3: escaped strings: plain, quoted with escapes, raw in
 *        single quotes, escaped unquoted, binary, and an escape refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
defaults_escaped(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "plain",
        "\"a\\tb\"",
        "'a\\tb'",
        "a\\nb",
        "\"q\\x41\"",
        "B\"(\\x41\\x42)\"",
        "\"bad\\q\""
    };

    transcribe(_run,
               cli::escaped_string(),
               VALUES,
               7u);

    return;
}

// DEFAULTS_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_defaults.
const step_fn DEFAULTS_STEPS[] =
{
    &defaults_ranges,
    &defaults_numbers,
    &defaults_escaped
};

// DEFAULTS_COUNT
//   constant: how many steps DEFAULTS_STEPS holds.
const std::size_t DEFAULTS_COUNT =
    sizeof(DEFAULTS_STEPS) / sizeof(DEFAULTS_STEPS[0]);

/**
 * @brief SETTINGS 1: a validator made from a function, a description and
 *        a name; described and named again, made inactive, non-modifying
 *        and applied to one value's index, each read back and run.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_function(
    run& _run
)
{
    static const char* const VALUES[] = { "abc", "Mixed Case", "a1", "" };
    cli::validator           loud(shouted(),
                                  "LOUD",
                                  "loud");

    write_validator(_run,
                    "made",
                    loud);
    transcribe(_run,
               loud,
               VALUES,
               4u);
    loud.description("LOUDER")
        .name("louder");
    write_validator(_run,
                    "renamed",
                    loud);
    loud.active(false);
    write_validator(_run,
                    "inactive",
                    loud);
    transcribe(_run,
               loud,
               VALUES,
               4u);
    loud.active();
    loud.non_modifying();
    write_validator(_run,
                    "non-modifying",
                    loud);
    transcribe(_run,
               loud,
               VALUES,
               4u);
    loud.non_modifying(false)
        .application_index(2);
    write_validator(_run,
                    "indexed",
                    loud);

    return;
}

/**
 * @brief SETTINGS 2: the default validator and one described alone, which
 *        pass every value.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_plain(
    run& _run
)
{
    static const char* const VALUES[]  = { "anything", "" };
    const cli::validator     none      = cli::validator();
    const cli::validator     described("described only");

    write_validator(_run,
                    "default",
                    none);
    transcribe(_run,
               none,
               VALUES,
               2u);
    write_validator(_run,
                    "described",
                    described);
    transcribe(_run,
               described,
               VALUES,
               2u);

    return;
}

/**
 * @brief SETTINGS 3: a validator called directly: on a value it may
 *        change, on a constant one, which it judges as a copy, and on a
 *        temporary; and one whose function changes a value before it
 *        rejects it, the change left in the value called on.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_called(
    run& _run
)
{
    const cli::validator loud(shouted(),
                              "LOUD",
                              "loud");
    const cli::validator halfway(half_done(),
                                 "HALF",
                                 "half");
    std::string          given("abc");
    const std::string    kept("xyz");
    std::string          changed("ab1");
    const std::string    first  = loud(given);
    const std::string    second = loud(kept);
    const std::string    third  = loud(std::string("a1"));
    const std::string    fourth = halfway(changed);

    _run.note("first=[" + escape(first) +
              "] given=[" + escape(given) +
              "] second=[" + escape(second) +
              "] kept=[" + escape(kept) +
              "] third=[" + escape(third) + "]");
    _run.note("fourth=[" + escape(fourth) +
              "] changed=[" + escape(changed) + "]");

    return;
}

// SETTINGS_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_settings.
const step_fn SETTINGS_STEPS[] =
{
    &settings_function,
    &settings_plain,
    &settings_called
};

// SETTINGS_COUNT
//   constant: how many steps SETTINGS_STEPS holds.
const std::size_t SETTINGS_COUNT =
    sizeof(SETTINGS_STEPS) / sizeof(SETTINGS_STEPS[0]);

/**
 * @brief COMPOSED 1: ranges and numbers combined with `&`, `|` and `!`,
 *        nested.
 *
 * @param[in,out] _run  the step's run.
 */
void
composed_ranges(
    run& _run
)
{
    static const char* const VALUES[] = { "3", "12", "7", "0", "x" };
    const cli::validator     small    = cli::range(1,
                                                   5);
    const cli::validator     large    = cli::range(10,
                                                   15);
    const cli::validator     either   = small | large;
    const cli::validator     both     = cli::range(1,
                                                   10) &
                                        cli::positive_number();
    const cli::validator     outside  = !small;
    const cli::validator     neither  = !(small | large);

    write_validator(_run,
                    "either",
                    either);
    transcribe(_run,
               either,
               VALUES,
               5u);
    write_validator(_run,
                    "both",
                    both);
    transcribe(_run,
               both,
               VALUES,
               5u);
    write_validator(_run,
                    "outside",
                    outside);
    transcribe(_run,
               outside,
               VALUES,
               5u);
    write_validator(_run,
                    "neither",
                    neither);
    transcribe(_run,
               neither,
               VALUES,
               5u);

    return;
}

/**
 * @brief COMPOSED 2: validators made from functions, combined: changes made
 *        in turn, a reason from either side, and a negation.
 *
 * @param[in,out] _run  the step's run.
 */
void
composed_functions(
    run& _run
)
{
    static const char* const VALUES[] = { "ab", "abc", "a1" };
    const suffixed           bang     = { "!" };
    const cli::validator     loud(shouted(),
                                  "LOUD",
                                  "loud");
    const cli::validator     marked(bang,
                                    "MARKED",
                                    "marked");
    const cli::validator     even(even_length(),
                                  "EVEN",
                                  "even");

    write_validator(_run,
                    "loud & marked",
                    loud & marked);
    transcribe(_run,
               loud & marked,
               VALUES,
               3u);
    write_validator(_run,
                    "marked & loud",
                    marked & loud);
    transcribe(_run,
               marked & loud,
               VALUES,
               3u);
    write_validator(_run,
                    "loud | even",
                    loud | even);
    transcribe(_run,
               loud | even,
               VALUES,
               3u);
    write_validator(_run,
                    "!even",
                    !even);
    transcribe(_run,
               !even,
               VALUES,
               3u);

    return;
}

// COMPOSED_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_composed.
const step_fn COMPOSED_STEPS[] =
{
    &composed_ranges,
    &composed_functions
};

// COMPOSED_COUNT
//   constant: how many steps COMPOSED_STEPS holds.
const std::size_t COMPOSED_COUNT =
    sizeof(COMPOSED_STEPS) / sizeof(COMPOSED_STEPS[0]);

/**
 * @brief IN_A_PARSE 1: a check and a transformation given to options,
 *        passed and failed.
 *
 * @param[in,out] _run  the step's run.
 */
void
parse_checks(
    run& _run
)
{
    static const char* const GIVEN[]   = { "--number", "5", "--word", "abc" };
    static const char* const OUTSIDE[] = { "--number", "11" };
    static const char* const DIGITS[]  = { "--word", "a1" };
    cli::option              number    = _run.watch(
        "number",
        _run.app().add_option("--number"));
    cli::option              word      = _run.watch(
        "word",
        _run.app().add_option("--word"));

    number->check(cli::range(1,
                             10));
    word->transform(cli::validator(shouted(),
                                   "LOUD",
                                   "loud"));
    _run.parse(GIVEN,
               4u);
    _run.parse(OUTSIDE,
               2u);
    _run.parse(DIGITS,
               2u);

    return;
}

/**
 * @brief IN_A_PARSE 2: a check applied to one value's index, a
 *        non-modifying transformation, and settings changed through the
 *        option's validators between parses.
 *
 * @param[in,out] _run  the step's run.
 */
void
parse_settings(
    run& _run
)
{
    static const char* const EARLY[] = { "--list", "9", "3" };
    static const char* const LATE[]  = { "--list", "1", "9" };
    static const char* const WORD[]  = { "--word", "abc" };
    cli::option              list    = _run.watch(
        "list",
        _run.app().add_option("--list")
                  ->expected(1,
                             3));
    cli::option              word    = _run.watch(
        "word",
        _run.app().add_option("--word"));

    list->check(cli::range(1,
                           5).application_index(1),
                "second");
    word->transform(cli::validator(shouted(),
                                   "LOUD",
                                   "loud").non_modifying());
    _run.parse(EARLY,
               3u);
    _run.parse(LATE,
               3u);
    _run.parse(WORD,
               2u);
    word->get_validator("loud")->non_modifying(false);
    list->get_validator("second")->active(false);
    _run.parse(WORD,
               2u);
    _run.parse(LATE,
               3u);

    return;
}

// IN_A_PARSE_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_in_a_parse.
const step_fn IN_A_PARSE_STEPS[] =
{
    &parse_checks,
    &parse_settings
};

// IN_A_PARSE_COUNT
//   constant: how many steps IN_A_PARSE_STEPS holds.
const std::size_t IN_A_PARSE_COUNT =
    sizeof(IN_A_PARSE_STEPS) / sizeof(IN_A_PARSE_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 1.3    Extra validators
//------------------------------------------------------------------------------

// point
//   struct: a type read and written with stream operators, `x,y`. It has a
// constructor, so it is no aggregate: from C++20, CLI11 makes an aggregate
// from a number (ISSUES 53).
struct point
{
    int m_x;
    int m_y;

    /**
     * @brief Makes the point at the origin.
     */
    point()
        : m_x(0),
          m_y(0)
    {}
};

/**
 * @brief Reads a point, `x,y`.
 *
 * @param[in,out] _in     the stream.
 * @param[out]    _point  receives the point.
 * @return the stream, failed for anything else.
 */
std::istream&
operator>>(
    std::istream& _in,
    point&        _point
)
{
    char comma = '\0';

    _in >> _point.m_x >> comma >> _point.m_y;

    // anything but a comma between the numbers
    if (comma != ',')
    {
        _in.setstate(std::ios_base::failbit);
    }

    return _in;
}

/**
 * @brief Writes a point, `x,y`.
 *
 * @param[in,out] _out    the stream.
 * @param[in]     _point  the point.
 * @return the stream.
 */
std::ostream&
operator<<(
    std::ostream& _out,
    const point&  _point
)
{
    return _out << _point.m_x << ',' << _point.m_y;
}

/**
 * @brief EXTRA 1: values that must convert to a type -- built-in, text and
 *        streamed -- and to a number.
 *
 * @param[in,out] _run  the step's run.
 */
void
extra_types(
    run& _run
)
{
    static const char* const NUMBERS[] =
    {
        "12", "-3", "1.5", "300", "1e2", "x", ""
    };
    static const char* const POINTS[]  = { "3,4", "3;4", "x" };

    transcribe(_run,
               cli::type_validator<int>(),
               NUMBERS,
               7u);
    transcribe(_run,
               cli::type_validator<double>(),
               NUMBERS,
               7u);
    transcribe(_run,
               cli::type_validator<unsigned char>(),
               NUMBERS,
               7u);
    transcribe(_run,
               cli::type_validator<std::string>(),
               NUMBERS,
               7u);
    transcribe(_run,
               cli::type_validator<point>(),
               POINTS,
               3u);
    transcribe(_run,
               cli::number(),
               NUMBERS,
               7u);

    return;
}

/**
 * @brief EXTRA 2: bounds over integers, unsigned ones and real numbers,
 *        between two bounds and from 0, clamping what is past them.
 *
 * @param[in,out] _run  the step's run.
 */
void
extra_bounds(
    run& _run
)
{
    static const char* const INTEGERS[] =
    {
        "0", "5", "11", "-7", "2.75", "x", ""
    };
    static const char* const REALS[]    = { "0.25", "1.5", "3", "x" };

    transcribe(_run,
               cli::bound(1,
                          10),
               INTEGERS,
               7u);
    transcribe(_run,
               cli::bound(10u),
               INTEGERS,
               7u);
    transcribe(_run,
               cli::bound(-5,
                          5),
               INTEGERS,
               7u);
    transcribe(_run,
               cli::bound(0.5,
                          2.5),
               REALS,
               4u);

    return;
}

/**
 * @brief EXTRA 3: IPv4 addresses, and what is not one.
 *
 * @param[in,out] _run  the step's run.
 */
void
extra_ipv4(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "1.2.3.4", "0.0.0.0", "255.255.255.255", "256.1.1.1", "1.2.3",
        "1.2.3.4.5", "a.b.c.d", "1..2.3", "-1.2.3.4", ""
    };

    transcribe(_run,
               cli::valid_ipv4(),
               VALUES,
               10u);

    return;
}

/**
 * @brief EXTRA 4: a bound, a size and a set given to options as
 *        transformations, in a parse that passes and one that fails.
 *
 * @param[in,out] _run  the step's run.
 */
void
extra_in_a_parse(
    run& _run
)
{
    static const char* const GIVEN[]   =
    {
        "--count", "15",
        "--size", "2kb",
        "--choice", "B"
    };
    static const char* const REFUSED[] = { "--choice", "d" };
    std::vector<std::string> choices;
    cli::option              count     = _run.watch(
        "count",
        _run.app().add_option("--count"));
    cli::option              size      = _run.watch(
        "size",
        _run.app().add_option("--size"));
    cli::option              choice    = _run.watch(
        "choice",
        _run.app().add_option("--choice"));

    choices.push_back("a");
    choices.push_back("b");
    count->transform(cli::bound(1,
                                10));
    size->transform(cli::as_size_value(false));
    choice->transform(cli::is_member(choices,
                                     cli::ignore_case));
    _run.parse(GIVEN,
               6u);
    _run.parse(REFUSED,
               2u);

    return;
}

// EXTRA_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_extra.
const step_fn EXTRA_STEPS[] =
{
    &extra_types,
    &extra_bounds,
    &extra_ipv4,
    &extra_in_a_parse
};

// EXTRA_COUNT
//   constant: how many steps EXTRA_STEPS holds.
const std::size_t EXTRA_COUNT = sizeof(EXTRA_STEPS) / sizeof(EXTRA_STEPS[0]);

/**
 * @brief MEMBER 1: sets over the standard containers -- a vector of text,
 *        a set of integers, a map's keys.
 *
 * @param[in,out] _run  the step's run.
 */
void
member_containers(
    run& _run
)
{
    static const char* const WORDS[]   =
    {
        "alpha", "beta", "gamma", "Alpha", ""
    };
    static const char* const NUMBERS[] = { "1", "10", "01", "3", "x" };
    static const char* const KEYS[]    = { "one", "1", "three" };
    std::vector<std::string>   words;
    std::set<int>              numbers;
    std::map<std::string, int> keyed;

    words.push_back("alpha");
    words.push_back("beta");
    numbers.insert(1);
    numbers.insert(2);
    numbers.insert(10);
    keyed["one"] = 1;
    keyed["two"] = 2;
    transcribe(_run,
               cli::is_member(words),
               WORDS,
               5u);
    transcribe(_run,
               cli::is_member(numbers),
               NUMBERS,
               5u);
    transcribe(_run,
               cli::is_member(keyed),
               KEYS,
               3u);

    return;
}

/**
 * @brief MEMBER 2: a set under CLI11's filters, one, two and all three,
 *        and under a function of the caller's.
 *
 * @param[in,out] _run  the step's run.
 */
void
member_filters(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "alpha", "ALPHA", "betavalue", "beta_value", "Gamma Ray",
        "gammaray", "GAMMARAY", "-Alpha", "delta"
    };
    std::vector<std::string> members;

    members.push_back("Alpha");
    members.push_back("Beta_Value");
    members.push_back("Gamma Ray");
    transcribe(_run,
               cli::is_member(members,
                              cli::ignore_case),
               VALUES,
               9u);
    transcribe(_run,
               cli::is_member(members,
                              cli::ignore_case,
                              cli::ignore_underscore),
               VALUES,
               9u);
    transcribe(_run,
               cli::is_member(members,
                              cli::ignore_space),
               VALUES,
               9u);
    transcribe(_run,
               cli::is_member(members,
                              cli::ignore_case,
                              cli::ignore_underscore,
                              cli::ignore_space),
               VALUES,
               9u);
    transcribe(_run,
               cli::is_member(members,
                              dashless()),
               VALUES,
               9u);

    return;
}

/**
 * @brief MEMBER 3: sets of numbers, a value compared as a number of their
 *        type.
 *
 * @param[in,out] _run  the step's run.
 */
void
member_typed(
    run& _run
)
{
    static const char* const INTEGERS[] = { "2", "02", "2.0", "3", "x" };
    static const char* const REALS[]    = { "0.5", "0.50", "5e-1", "1", "x" };
    std::vector<int>         integers;
    std::vector<double>      reals;

    integers.push_back(1);
    integers.push_back(2);
    integers.push_back(10);
    reals.push_back(0.5);
    reals.push_back(1.5);
    transcribe(_run,
               cli::is_member(integers),
               INTEGERS,
               5u);
    transcribe(_run,
               cli::is_member(reals),
               REALS,
               5u);

    return;
}

// MEMBER_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_member.
const step_fn MEMBER_STEPS[] =
{
    &member_containers,
    &member_filters,
    &member_typed
};

// MEMBER_COUNT
//   constant: how many steps MEMBER_STEPS holds.
const std::size_t MEMBER_COUNT =
    sizeof(MEMBER_STEPS) / sizeof(MEMBER_STEPS[0]);

/**
 * @brief TRANSFORM 1: transformers and checked transformers over a map of
 *        text, pairs in a vector, and a map of numbers.
 *
 * @param[in,out] _run  the step's run.
 */
void
transform_maps(
    run& _run
)
{
    static const char* const NAMES[]  = { "one", "two", "1", "three", "" };
    static const char* const LEVELS[] = { "low", "high", "9", "mid" };
    static const char* const COUNTS[] = { "ten", "10", "eleven" };
    std::map<std::string, std::string>               names;
    std::vector<std::pair<std::string, std::string> > levels;
    std::map<std::string, int>                       counts;

    names["one"] = "1";
    names["two"] = "2";
    levels.push_back(std::make_pair(std::string("low"),
                                    std::string("0")));
    levels.push_back(std::make_pair(std::string("high"),
                                    std::string("9")));
    counts["one"] = 1;
    counts["ten"] = 10;
    transcribe(_run,
               cli::transformer(names),
               NAMES,
               5u);
    transcribe(_run,
               cli::checked_transformer(names),
               NAMES,
               5u);
    transcribe(_run,
               cli::transformer(levels),
               LEVELS,
               4u);
    transcribe(_run,
               cli::checked_transformer(levels),
               LEVELS,
               4u);
    transcribe(_run,
               cli::transformer(counts),
               COUNTS,
               3u);
    transcribe(_run,
               cli::checked_transformer(counts),
               COUNTS,
               3u);

    return;
}

/**
 * @brief TRANSFORM 2: transformers under filters.
 *
 * @param[in,out] _run  the step's run.
 */
void
transform_filters(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "ONE", "One", "O_NE", "o ne", "1", "x"
    };
    std::map<std::string, std::string> names;

    names["one"] = "1";
    names["two"] = "2";
    transcribe(_run,
               cli::transformer(names,
                                cli::ignore_case),
               VALUES,
               6u);
    transcribe(_run,
               cli::checked_transformer(names,
                                        cli::ignore_case,
                                        cli::ignore_underscore),
               VALUES,
               6u);
    transcribe(_run,
               cli::checked_transformer(names,
                                        cli::ignore_case,
                                        cli::ignore_underscore,
                                        cli::ignore_space),
               VALUES,
               6u);

    return;
}

// TRANSFORM_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_transform.
const step_fn TRANSFORM_STEPS[] =
{
    &transform_maps,
    &transform_filters
};

// TRANSFORM_COUNT
//   constant: how many steps TRANSFORM_STEPS holds.
const std::size_t TRANSFORM_COUNT =
    sizeof(TRANSFORM_STEPS) / sizeof(TRANSFORM_STEPS[0]);

/**
 * @brief UNITS 1: numbers with units, of integer and real factors, under
 *        the default options and under others, named.
 *
 * @param[in,out] _run  the step's run.
 */
void
units_numbers(
    run& _run
)
{
    static const char* const SIZES[]   =
    {
        "10", "10kb", "10 KB", "10KB", "2mb", "4096mb", "1.5kb", "kb",
        "10x", "x", ""
    };
    static const char* const LENGTHS[] = { "1.5km", "2 m", "3", "x" };
    std::map<std::string, int>    sizes;
    std::map<std::string, double> lengths;

    sizes["b"]   = 1;
    sizes["kb"]  = 1024;
    sizes["mb"]  = 1048576;
    lengths["m"]  = 1.0;
    lengths["km"] = 1000.0;
    transcribe(_run,
               cli::as_number_with_unit(sizes),
               SIZES,
               11u);
    transcribe(_run,
               cli::as_number_with_unit(sizes,
                                        cli::unit_options::case_sensitive |
                                        cli::unit_options::unit_required,
                                        "SIZE"),
               SIZES,
               11u);
    transcribe(_run,
               cli::as_number_with_unit(lengths),
               LENGTHS,
               4u);

    return;
}

/**
 * @brief UNITS 2: sizes, a kilobyte a thousand bytes and 1024 of them.
 *
 * @param[in,out] _run  the step's run.
 */
void
units_sizes(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "1k", "1kb", "1KiB", "2 MB", "1.5k", "1ki", "10", "x"
    };

    transcribe(_run,
               cli::as_size_value(true),
               VALUES,
               8u);
    transcribe(_run,
               cli::as_size_value(false),
               VALUES,
               8u);

    return;
}

// UNITS_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_units.
const step_fn UNITS_STEPS[] =
{
    &units_numbers,
    &units_sizes
};

// UNITS_COUNT
//   constant: how many steps UNITS_STEPS holds.
const std::size_t UNITS_COUNT = sizeof(UNITS_STEPS) / sizeof(UNITS_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 1.4    Path validators
//------------------------------------------------------------------------------

// FILES
//   constant: the files the path steps work among, beside a folder each
// step makes.
const file_entry FILES[] =
{
    { "data.txt", "12345" },
    { "empty.txt", "" },
    { D_NULLPTR, D_NULLPTR }
};

/**
 * @brief PATHS 1: files, folders and paths that exist, and that do not.
 *
 * @param[in,out] _run  the step's run.
 */
void
paths_existence(
    run& _run
)
{
    static const char* const PATHS_GIVEN[] =
    {
        "data.txt", "folder", "missing.txt"
    };
    scratch_directory        files(FILES);

    // the folder beside the files
    if ( (!files.make_folder("folder")) ||
         (!files.ok()) )
    {
        _run.note("the scratch directory was not made");
    }

    transcribe(_run,
               cli::existing_file(),
               PATHS_GIVEN,
               3u);
    transcribe(_run,
               cli::existing_directory(),
               PATHS_GIVEN,
               3u);
    transcribe(_run,
               cli::existing_path(),
               PATHS_GIVEN,
               3u);
    transcribe(_run,
               cli::nonexistent_path(),
               PATHS_GIVEN,
               3u);

    return;
}

/**
 * @brief PATHS 2: a file looked for on a default path, its error returned
 *        and not.
 *
 * @param[in,out] _run  the step's run.
 */
void
paths_search(
    run& _run
)
{
    static const char* const NAMES[] =
    {
        "inner.txt", "data.txt", "missing.txt"
    };
    scratch_directory        files(FILES);

    // the folder, and a file in it
    if ( (!files.make_folder("folder"))         ||
         (!files.make("folder/inner.txt",
                      "x"))                     ||
         (!files.ok()) )
    {
        _run.note("the scratch directory was not made");
    }

    transcribe(_run,
               cli::file_on_default_path("folder"),
               NAMES,
               3u);
    transcribe(_run,
               cli::file_on_default_path("folder",
                                         false),
               NAMES,
               3u);

    return;
}

/**
 * @brief PATHS 3: what files permit, and which are empty.
 *
 * @param[in,out] _run  the step's run.
 */
void
paths_files(
    run& _run
)
{
    static const char* const PATHS_GIVEN[] =
    {
        "data.txt", "empty.txt", "folder", "missing.txt"
    };
    scratch_directory        files(FILES);

    // the folder beside the files
    if ( (!files.make_folder("folder")) ||
         (!files.ok()) )
    {
        _run.note("the scratch directory was not made");
    }

    transcribe(_run,
               cli::read_permissions(),
               PATHS_GIVEN,
               4u);
    transcribe(_run,
               cli::write_permissions(),
               PATHS_GIVEN,
               4u);
    transcribe(_run,
               cli::exec_permissions(),
               PATHS_GIVEN,
               4u);
    transcribe(_run,
               cli::non_empty_file(),
               PATHS_GIVEN,
               4u);

    return;
}

// PATHS_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_paths.
const step_fn PATHS_STEPS[] =
{
    &paths_existence,
    &paths_search,
    &paths_files
};

// PATHS_COUNT
//   constant: how many steps PATHS_STEPS holds.
const std::size_t PATHS_COUNT = sizeof(PATHS_STEPS) / sizeof(PATHS_STEPS[0]);

#if defined(UINT64_MAX)

/**
 * @brief PATH_SIZES 1: files' sizes, between bounds and from a least.
 *
 * @param[in,out] _run  the step's run.
 */
void
paths_sizes(
    run& _run
)
{
    static const char* const PATHS_GIVEN[] =
    {
        "data.txt", "empty.txt", "folder", "missing.txt"
    };
    scratch_directory        files(FILES);

    // the folder beside the files
    if ( (!files.make_folder("folder")) ||
         (!files.ok()) )
    {
        _run.note("the scratch directory was not made");
    }

    transcribe(_run,
               cli::file_size(1u,
                              10u),
               PATHS_GIVEN,
               4u);
    transcribe(_run,
               cli::file_size(6u),
               PATHS_GIVEN,
               4u);

    return;
}

#else

/**
 * @brief PATH_SIZES 1: where no 64-bit integer is declared, the face
 *        declares no file_size, which the step says.
 *
 * @param[in,out] _run  the step's run.
 */
void
paths_sizes(
    run& _run
)
{
    _run.note("file_size: no 64-bit integer is declared here");

    return;
}

#endif  // UINT64_MAX

// PATH_SIZES_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_path_sizes.
const step_fn PATH_SIZES_STEPS[] =
{
    &paths_sizes
};

// PATH_SIZES_COUNT
//   constant: how many steps PATH_SIZES_STEPS holds.
const std::size_t PATH_SIZES_COUNT =
    sizeof(PATH_SIZES_STEPS) / sizeof(PATH_SIZES_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.5    What C++11 brings
//------------------------------------------------------------------------------

// colour
//   enum: an enumeration of its own scope, over `char`.
enum class colour : char
{
    red   = 1,
    green = 2
};

// letter
//   enum: an enumeration over `char`, not of its own scope.
enum letter : char
{
    first_letter  = 1,
    second_letter = 2
};

/**
 * @brief CPP11 1: ranges over enumerations over `char`, of their own scope
 *        and not.
 *
 * @param[in,out] _run  the step's run.
 */
void
cpp11_enumerations(
    run& _run
)
{
    static const char* const VALUES[] = { "1", "2", "5", "x" };

    transcribe(_run,
               cli::range(colour::red,
                          colour::green),
               VALUES,
               4u);
    transcribe(_run,
               cli::range(first_letter,
                          second_letter),
               VALUES,
               4u);

    return;
}

// CPP11_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_cpp11.
const step_fn CPP11_STEPS[] =
{
    &cpp11_enumerations
};

// CPP11_COUNT
//   constant: how many steps CPP11_STEPS holds.
const std::size_t CPP11_COUNT = sizeof(CPP11_STEPS) / sizeof(CPP11_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief CPP11_EXTRA 1: bounds over enumerations over `char`, of their own
 *        scope and not.
 *
 * @param[in,out] _run  the step's run.
 */
void
cpp11_bounds(
    run& _run
)
{
    static const char* const VALUES[] = { "1", "2", "5", "x" };

    transcribe(_run,
               cli::bound(colour::red,
                          colour::green),
               VALUES,
               4u);
    transcribe(_run,
               cli::bound(first_letter,
                          second_letter),
               VALUES,
               4u);

    return;
}

/**
 * @brief CPP11_EXTRA 2: sets and mappings given as braced lists.
 *
 * @param[in,out] _run  the step's run.
 */
void
cpp11_lists(
    run& _run
)
{
    static const char* const WORDS[]   = { "red", "Red", "blue" };
    static const char* const NUMBERS[] = { "1", "3", "x" };
    static const char* const NAMES[]   = { "one", "1", "two" };

    transcribe(_run,
               cli::is_member({ "red", "green" }),
               WORDS,
               3u);
    transcribe(_run,
               cli::is_member({ "Red", "Green" },
                              cli::ignore_case),
               WORDS,
               3u);
    transcribe(_run,
               cli::is_member({ 1, 2 }),
               NUMBERS,
               3u);
    transcribe(_run,
               cli::transformer({ { "one", "1" } }),
               NAMES,
               3u);
    transcribe(_run,
               cli::checked_transformer({ { "one", "1" } }),
               NAMES,
               3u);

    return;
}

// CPP11_EXTRA_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_cpp11_extra.
const step_fn CPP11_EXTRA_STEPS[] =
{
    &cpp11_bounds,
    &cpp11_lists
};

// CPP11_EXTRA_COUNT
//   constant: how many steps CPP11_EXTRA_STEPS holds.
const std::size_t CPP11_EXTRA_COUNT =
    sizeof(CPP11_EXTRA_STEPS) / sizeof(CPP11_EXTRA_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: a validator as a C value, the handles on an
// option's validators, moved validators, filters as values, and the
// departures. Each is a step written here, through the whole face.

namespace
{

// 2.1    The value
//------------------------------------------------------------------------------

// appended
//   struct: a validator's function holding its own text, which it appends
// to every value: a function whose copy a validator must own.
struct appended
{
    std::string m_text;

    /**
     * @brief Appends the text.
     *
     * @param[in,out] _value  the value.
     * @return "": every value passes.
     */
    std::string
    operator()(
        std::string& _value
    ) const
    {
        _value += m_text;

        return std::string();
    }
};

/**
 * @brief VALUE 1: copies, by construction and assignment, and an
 *        assignment from itself, each a validator of its own.
 *
 * @param[in,out] _run  the step's run.
 */
void
value_copied(
    run& _run
)
{
    static const char* const VALUES[] = { "5", "11" };
    cli::validator           first    = cli::range(1,
                                                   10);

    first.name("first");

    cli::validator  second(first);
    cli::validator  third;
    cli::validator& alias = third;

    second.description("second's own")
          .name("second");
    third = first;
    third = alias;
    write_validator(_run,
                    "first",
                    first);
    write_validator(_run,
                    "second",
                    second);
    write_validator(_run,
                    "third",
                    third);
    transcribe(_run,
               third,
               VALUES,
               2u);

    return;
}

/**
 * @brief VALUE 2: two validators swapped; one handed over to C, which runs
 *        and releases it, the face's left the default validator.
 *
 * @param[in,out] _run  the step's run.
 */
void
value_swapped(
    run& _run
)
{
    cli::validator first = cli::range(1,
                                      10);
    cli::validator second = cli::positive_number();

    first.name("first");
    second.name("second");
    first.swap(second);
    write_validator(_run,
                    "first",
                    first);
    write_validator(_run,
                    "second",
                    second);

    ::d_arg_validator  raw        = first.detach();
    ::d_arg_validation validation = ::d_arg_validation();

    write_validator(_run,
                    "detached",
                    first);
    d_arg_validation_init(&validation,
                          cli::text("-1"));

    const int status = d_arg_validator_run(&raw,
                                           &validation);

    _run.note(std::string("raw: name=[") +
              ((raw.name != D_NULLPTR) ? raw.name : "") +
              "] status=" + cli::status(status).get_name() +
              " passed=" +
              (d_arg_validation_passed(&validation) ? "1" : "0") +
              " reason=[" +
              escape(cli::text(d_arg_buffer_text(&validation.message))) +
              "]");
    d_arg_validation_release(&validation);
    d_arg_validator_release(&raw);

    return;
}

/**
 * @brief VALUE 3: a validator made from a function that holds text, kept
 *        past the function and the validator it was made as, and copied:
 *        each runs a copy of the function of its own.
 *
 * @param[in,out] _run  the step's run.
 */
void
value_owned(
    run& _run
)
{
    static const char* const VALUES[] = { "ab" };
    cli::validator           kept;

    // made where neither the function nor the validator outlives the block
    {
        const appended       adding = { std::string("+") };
        const cli::validator made(adding,
                                  "ADDED",
                                  "added");

        kept = made;
    }

    const cli::validator again(kept);

    write_validator(_run,
                    "kept",
                    kept);
    transcribe(_run,
               kept,
               VALUES,
               1u);
    transcribe(_run,
               again,
               VALUES,
               1u);

    return;
}

// VALUE_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_value.
const step_fn VALUE_STEPS[] =
{
    &value_copied,
    &value_swapped,
    &value_owned
};

// VALUE_COUNT
//   constant: how many steps VALUE_STEPS holds.
const std::size_t VALUE_COUNT = sizeof(VALUE_STEPS) / sizeof(VALUE_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief VALUE_EXTRA 1: a set over a C array, a form the face has and
 *        CLI11's IsMember has not, with and without a filter.
 *
 * @param[in,out] _run  the step's run.
 */
void
value_array(
    run& _run
)
{
    static const char* const COLOURS[] = { "red", "green" };
    static const int         NUMBERS[] = { 1, 2 };
    static const char* const VALUES[]  = { "red", "RED", "1", "3" };

    transcribe(_run,
               cli::is_member(COLOURS),
               VALUES,
               4u);
    transcribe(_run,
               cli::is_member(COLOURS,
                              cli::ignore_case),
               VALUES,
               4u);
    transcribe(_run,
               cli::is_member(NUMBERS),
               VALUES,
               4u);

    return;
}

// VALUE_EXTRA_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_value_extra.
const step_fn VALUE_EXTRA_STEPS[] =
{
    &value_array
};

// VALUE_EXTRA_COUNT
//   constant: how many steps VALUE_EXTRA_STEPS holds.
const std::size_t VALUE_EXTRA_COUNT =
    sizeof(VALUE_EXTRA_STEPS) / sizeof(VALUE_EXTRA_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// 2.2    Handles
//------------------------------------------------------------------------------

/**
 * @brief Writes what a handle reads, on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the handle by.
 * @param[in]     _handle  the handle.
 */
void
write_handle(
    run&                         _run,
    const char*                  _label,
    const cli::validator_handle& _handle
)
{
    std::ostringstream line;

    line << _label
         << ": valid=" << _handle.valid()
         << " name=[" << _handle->get_name()
         << "] description=[" << escape(_handle->get_description())
         << "] active=" << _handle->get_active()
         << " modifying=" << _handle->get_modifying()
         << " index=" << _handle->get_application_index()
         << " status=" << _handle->get_status().get_name();
    _run.note(line.str());

    return;
}

/**
 * @brief HANDLES 1: handles on an option's validators, by name and by
 *        index; settings made through one, in a chain; a copy made from
 *        one, which outlives the option.
 *
 * @param[in,out] _run  the step's run.
 */
void
handles_found(
    run& _run
)
{
    static const char* const VALUES[] = { "5", "11" };
    cli::validator           copied;

    // the option's parser is the run's, whose validators the copy outlives
    {
        cli::option number = _run.app().add_option("--number");

        number->check(cli::range(1,
                                 10),
                      "ranged");
        number->check(cli::positive_number());

        cli::validator_handle       ranged = number->get_validator("ranged");
        const cli::validator_handle second = number->get_validator(1u);
        const cli::validator_handle first  = number->get_validator();

        write_handle(_run,
                     "ranged",
                     ranged);
        write_handle(_run,
                     "second",
                     second);
        write_handle(_run,
                     "first",
                     first);
        ranged->description("changed")
              .application_index(0)
              .active(false);
        write_handle(_run,
                     "set",
                     number->get_validator("ranged"));
        copied = ranged->copy();
        _run.note(std::string("same=") +
                  ((ranged.get() == number->get_validator("ranged").get())
                   ? "1" : "0"));
    }

    copied.active();
    write_validator(_run,
                    "copied",
                    copied);
    transcribe(_run,
               copied,
               VALUES,
               2u);

    return;
}

// HANDLES_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_handles.
const step_fn HANDLES_STEPS[] =
{
    &handles_found
};

// HANDLES_COUNT
//   constant: how many steps HANDLES_STEPS holds.
const std::size_t HANDLES_COUNT =
    sizeof(HANDLES_STEPS) / sizeof(HANDLES_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 2.3    Moved validators
//------------------------------------------------------------------------------

/**
 * @brief MOVED 1: validators moved by construction and assignment, each
 *        source left the default validator; operands moved into `|`; a
 *        validator moved into an option.
 *
 * @param[in,out] _run  the step's run.
 */
void
moved_values(
    run& _run
)
{
    static const char* const VALUES[] = { "3", "12", "7" };
    cli::validator           first    = cli::range(1,
                                                   10);

    first.name("first");

    cli::validator second(std::move(first));
    cli::validator third;

    write_validator(_run,
                    "first, moved from",
                    first);
    write_validator(_run,
                    "second",
                    second);
    third = std::move(second);
    write_validator(_run,
                    "second, moved from",
                    second);
    write_validator(_run,
                    "third",
                    third);

    cli::validator small = cli::range(1,
                                      5);
    cli::validator large = cli::range(10,
                                      15);
    cli::validator either = std::move(small) | std::move(large);

    write_validator(_run,
                    "small, moved from",
                    small);
    write_validator(_run,
                    "either",
                    either);
    transcribe(_run,
               either,
               VALUES,
               3u);

    cli::option number = _run.app().add_option("--number");

    number->check(std::move(third),
                  "moved");
    write_validator(_run,
                    "third, moved into the option",
                    third);
    _run.note(std::string("held: ") +
              number->get_validator("moved")->get_description());

    return;
}

// MOVED_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_moved.
const step_fn MOVED_STEPS[] =
{
    &moved_values
};

// MOVED_COUNT
//   constant: how many steps MOVED_STEPS holds.
const std::size_t MOVED_COUNT = sizeof(MOVED_STEPS) / sizeof(MOVED_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 2.4    Filters
//------------------------------------------------------------------------------

// plusless
//   struct: a filter that drops a leading `+`.
struct plusless
{
    /**
     * @brief Filters a value.
     *
     * @param[in] _value  the value.
     * @return the value without its leading plus.
     */
    std::string
    operator()(
        std::string _value
    ) const
    {
        // a leading plus dropped
        if ( (!_value.empty()) &&
             (_value[0] == '+') )
        {
            _value.erase(0u,
                         1u);
        }

        return _value;
    }
};

/**
 * @brief Writes a filter's flags and whether it holds a function.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line names the filter by.
 * @param[in]     _filter  the filter.
 */
void
write_filter(
    run&               _run,
    const char*        _label,
    const cli::filter& _filter
)
{
    std::ostringstream line;

    line << _label
         << ": flags=" << static_cast<unsigned long>(_filter.get_flags())
         << " function=" << _filter.has_function();
    _run.note(line.str());

    return;
}

/**
 * @brief FILTERS 1: filters as values: none, each of CLI11's, by the
 *        function and by its call, combined, a function's, copied and
 *        swapped.
 *
 * @param[in,out] _run  the step's run.
 */
void
filters_values(
    run& _run
)
{
    const cli::filter none;
    const cli::filter cased(cli::ignore_case);
    const cli::filter called     = cli::ignore_case();
    const cli::filter underscore = cli::ignore_underscore();
    const cli::filter space      = cli::ignore_space();
    const cli::filter both       = cased | underscore;
    const cli::filter function   = dashless();
    const cli::filter chained    = function | space;
    cli::filter       left(cased);
    cli::filter       right(function);

    write_filter(_run,
                 "none",
                 none);
    write_filter(_run,
                 "case",
                 cased);
    write_filter(_run,
                 "case, called",
                 called);
    write_filter(_run,
                 "underscore",
                 underscore);
    write_filter(_run,
                 "space",
                 space);
    write_filter(_run,
                 "case | underscore",
                 both);
    write_filter(_run,
                 "function",
                 function);
    write_filter(_run,
                 "function | space",
                 chained);
    left.swap(right);
    write_filter(_run,
                 "left, swapped",
                 left);
    write_filter(_run,
                 "right, swapped",
                 right);

    return;
}

/**
 * @brief FILTERS 2: filters combined with `|` given to sets: flags
 *        together, and functions run in the order combined.
 *
 * @param[in,out] _run  the step's run.
 */
void
filters_used(
    run& _run
)
{
    static const char* const VALUES[] =
    {
        "ALPHA", "betavalue", "-+Alpha", "+-Alpha", "x"
    };
    std::vector<std::string> members;

    members.push_back("Alpha");
    members.push_back("Beta_Value");
    transcribe(_run,
               cli::is_member(members,
                              cli::filter(cli::ignore_case) |
                              cli::filter(cli::ignore_underscore)),
               VALUES,
               5u);
    transcribe(_run,
               cli::is_member(members,
                              cli::filter(dashless()) |
                              cli::filter(plusless())),
               VALUES,
               5u);

    return;
}

// FILTERS_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_filters.
const step_fn FILTERS_STEPS[] =
{
    &filters_values,
    &filters_used
};

// FILTERS_COUNT
//   constant: how many steps FILTERS_STEPS holds.
const std::size_t FILTERS_COUNT =
    sizeof(FILTERS_STEPS) / sizeof(FILTERS_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// 2.5    Departures
//------------------------------------------------------------------------------

/**
 * @brief DEPARTED 1: handles on validators an option does not have, by
 *        name and by index, and the handle made empty: each NULL, reading
 *        as the default validator, ignoring settings, running nothing.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_none_found(
    run& _run
)
{
    cli::option                 number   = _run.app().add_option("--number");
    cli::validator_handle       by_name  = number->get_validator("nope");
    const cli::validator_handle by_index = number->get_validator(5u);
    const cli::validator_handle empty;
    std::string                 value("x");

    by_name->description("ignored")
           .active(false);
    write_handle(_run,
                 "by name",
                 by_name);
    write_handle(_run,
                 "by index",
                 by_index);
    write_handle(_run,
                 "empty",
                 empty);

    const std::string reason = by_name(value);
    const cli::validator copied = by_name->copy();

    _run.note("ran: reason=[" + escape(reason) + "] value=[" +
              escape(value) + "]");
    write_validator(_run,
                    "copied",
                    copied);

    return;
}

/**
 * @brief DEPARTED 2: a function that changes a value and then rejects it,
 *        as a transformation in a parse: the option's result stays as it
 *        was given.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_partial(
    run& _run
)
{
    static const char* const GIVEN[] = { "--word", "ab1" };
    cli::option              word    = _run.watch(
        "word",
        _run.app().add_option("--word"));

    word->transform(cli::validator(half_done(),
                                   "HALF",
                                   "half"));
    _run.parse(GIVEN,
               2u);

    return;
}

/**
 * @brief DEPARTED 3: ranges over the 8-bit integer types, described with
 *        their bounds as numbers.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_ranges(
    run& _run
)
{
    static const char* const VALUES[] = { "5", "11" };
    const signed char        lowest   = 0;
    const signed char        highest  = 10;
    const unsigned char      least    = 65u;
    const unsigned char      most     = 100u;

    transcribe(_run,
               cli::range(lowest,
                          highest),
               VALUES,
               2u);
    transcribe(_run,
               cli::range(least,
                          most),
               VALUES,
               2u);

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_none_found,
    &departed_partial,
    &departed_ranges
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief Writes a validator's status beside its settings, on one line.
 *
 * @param[in,out] _run        the run.
 * @param[in]     _label      what the line names the validator by.
 * @param[in]     _validator  the validator.
 */
void
write_status(
    run&                  _run,
    const char*           _label,
    const cli::validator& _validator
)
{
    write_validator(_run,
                    _label,
                    _validator);
    _run.note(std::string(_label) + " status=" +
              _validator.get_status().get_name());

    return;
}

/**
 * @brief DEPARTED_EXTRA 1: an address whose part is a flag's word,
 *        refused.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_ipv4(
    run& _run
)
{
    static const char* const VALUES[] = { "true.0.0.1", "1.2.3.4" };

    transcribe(_run,
               cli::valid_ipv4(),
               VALUES,
               2u);

    return;
}

/**
 * @brief DEPARTED_EXTRA 2: values written as their types write them: a
 *        real number a bound clamps, an 8-bit bound and what it clamps, a
 *        float factor's product.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_written(
    run& _run
)
{
    static const char* const REALS[]   = { "0.05", "0.2" };
    static const char* const BYTES[]   = { "120", "70" };
    static const char* const LENGTHS[] = { "0.1 x", "2 x" };
    const unsigned char          least = 65u;
    const unsigned char          most  = 100u;
    std::map<std::string, float> units;

    units["x"] = 3.0f;
    transcribe(_run,
               cli::bound(0.1,
                          0.3),
               REALS,
               2u);
    transcribe(_run,
               cli::bound(least,
                          most),
               BYTES,
               2u);
    transcribe(_run,
               cli::as_number_with_unit(units),
               LENGTHS,
               2u);

    return;
}

/**
 * @brief DEPARTED_EXTRA 3: a size refused in a parse, the option's result
 *        the value as given.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_cut(
    run& _run
)
{
    static const char* const GIVEN[] = { "--size", "1zb" };
    cli::option              size    = _run.watch(
        "size",
        _run.app().add_option("--size"));

    size->transform(cli::as_size_value(false));
    _run.parse(GIVEN,
               2u);

    return;
}

/**
 * @brief DEPARTED_EXTRA 4: units refused when the validator is made: a
 *        failed validator, describing nothing, its status and the reason it
 *        keeps, and the definition error an option given it records.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_failed(
    run& _run
)
{
    std::map<std::string, int> units;

    units["k1"] = 1000;

    const cli::validator     failed = cli::as_number_with_unit(units);
    const ::d_arg_validator& raw    = failed;
    cli::option              size   = _run.app().add_option("--size");

    write_status(_run,
                 "failed",
                 failed);
    _run.note(std::string("reason kept=[") +
              ((raw.description != D_NULLPTR) ? raw.description : "") +
              "]");
    size->transform(failed);

    return;
}

// DEPARTED_EXTRA_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_departures_extra.
const step_fn DEPARTED_EXTRA_STEPS[] =
{
    &departed_ipv4,
    &departed_written,
    &departed_cut,
    &departed_failed
};

// DEPARTED_EXTRA_COUNT
//   constant: how many steps DEPARTED_EXTRA_STEPS holds.
const std::size_t DEPARTED_EXTRA_COUNT =
    sizeof(DEPARTED_EXTRA_STEPS) / sizeof(DEPARTED_EXTRA_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// shade
//   enum: an enumeration with an operator<< of its own.
enum shade
{
    pale = 0,
    deep = 3
};

/**
 * @brief Writes a shade by its name.
 *
 * @param[in,out] _out    the stream.
 * @param[in]     _shade  the shade.
 * @return the stream.
 */
std::ostream&
operator<<(
    std::ostream& _out,
    shade         _shade
)
{
    return _out << ((_shade == pale) ? "pale" : "deep");
}

/**
 * @brief DEPARTED_CPP11 1: a range over an enumeration with an operator<<
 *        of its own -- which writes it as a word -- its bounds written as
 *        numbers.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_enumerations(
    run& _run
)
{
    static const char* const VALUES[] = { "1", "2", "5" };
    std::ostringstream       own;

    own << pale << "," << deep;
    _run.note("its own text: " + own.str());
    transcribe(_run,
               cli::range(pale,
                          deep),
               VALUES,
               3u);

    return;
}

// DEPARTED_CPP11_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_departures_cpp11.
const step_fn DEPARTED_CPP11_STEPS[] =
{
    &departed_enumerations
};

// DEPARTED_CPP11_COUNT
//   constant: how many steps DEPARTED_CPP11_STEPS holds.
const std::size_t DEPARTED_CPP11_COUNT =
    sizeof(DEPARTED_CPP11_STEPS) / sizeof(DEPARTED_CPP11_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/**
 * @brief DEPARTED_CPP11_EXTRA 1: a bound over the same enumeration, its
 *        bounds and what it clamps written as numbers.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_enumerations_bounded(
    run& _run
)
{
    static const char* const VALUES[] = { "1", "2", "5" };

    transcribe(_run,
               cli::bound(pale,
                          deep),
               VALUES,
               3u);

    return;
}

// DEPARTED_CPP11_EXTRA_STEPS
//   constant: the steps of d_tests_arg_cpp_validator_departures_cpp11_extra.
const step_fn DEPARTED_CPP11_EXTRA_STEPS[] =
{
    &departed_enumerations_bounded
};

// DEPARTED_CPP11_EXTRA_COUNT
//   constant: how many steps DEPARTED_CPP11_EXTRA_STEPS holds.
const std::size_t DEPARTED_CPP11_EXTRA_COUNT =
    sizeof(DEPARTED_CPP11_EXTRA_STEPS) / sizeof(DEPARTED_CPP11_EXTRA_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

}  // namespace

//==============================================================================
// 3.  DEFAULT VALIDATORS
//==============================================================================

/*
d_tests_arg_cpp_validator_defaults
  CLI11's default validators, made by the face's functions, run over values.
  Tests the following:
  - range over integers, real numbers and unsigned ones, between two bounds
    and from 0, described by CLI11 and by the caller
  - positive and non-negative numbers
  - escaped strings: plain, quoted, raw, unquoted, binary, refused
*/
bool
d_tests_arg_cpp_validator_defaults()
{
    return steps_hold("DEFAULTS",
                      DEFAULTS_STEPS,
                      DEFAULTS_COUNT,
                      DEFAULTS);
}

/*
d_tests_arg_cpp_validator_settings
  A validator's settings and readers, as CLI11's Validator has them, and a
validator called directly.
  Tests the following:
  - a validator from a function, a description and a name; described and
    named again; inactive, passing every value untouched; non-modifying,
    judging a copy; given an application index
  - the default validator, and one described alone
  - operator() on a value it may change, on a constant one, on a temporary;
    a change made before a rejection left in the value called on
*/
bool
d_tests_arg_cpp_validator_settings()
{
    return steps_hold("SETTINGS",
                      SETTINGS_STEPS,
                      SETTINGS_COUNT,
                      SETTINGS);
}

/*
d_tests_arg_cpp_validator_composed
  Validators combined with `&`, `|` and `!`, as CLI11's operators combine
them.
  Tests the following:
  - ranges and numbers combined and nested: their descriptions, names and
    results
  - functions combined: changes made in turn, reasons from either side
*/
bool
d_tests_arg_cpp_validator_composed()
{
    return steps_hold("COMPOSED",
                      COMPOSED_STEPS,
                      COMPOSED_COUNT,
                      COMPOSED);
}

/*
d_tests_arg_cpp_validator_in_a_parse
  Validators given to options, as CLI11's check() and transform() give them,
in parses.
  Tests the following:
  - a check and a transformation, passed and failed
  - a check on one value's index; a non-modifying transformation; settings
    changed through the option's validators between parses
*/
bool
d_tests_arg_cpp_validator_in_a_parse()
{
    return steps_hold("IN_A_PARSE",
                      IN_A_PARSE_STEPS,
                      IN_A_PARSE_COUNT,
                      IN_A_PARSE);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// 4.  EXTRA VALIDATORS
//==============================================================================

/*
d_tests_arg_cpp_validator_extra
  CLI11's extra validators of types, bounds and addresses.
  Tests the following:
  - type_validator over built-in types, text and a streamed type; number()
  - bounds over integers, unsigned ones and real numbers, clamping
  - IPv4 addresses
  - a bound, a size and a set as transformations in parses
*/
bool
d_tests_arg_cpp_validator_extra()
{
    return steps_hold("EXTRA",
                      EXTRA_STEPS,
                      EXTRA_COUNT,
                      EXTRA);
}

/*
d_tests_arg_cpp_validator_member
  CLI11's IsMember over the standard containers, under filters.
  Tests the following:
  - a vector of text, a set of integers, a map's keys
  - CLI11's filters, one, two and three, and a function of the caller's
  - sets of numbers, a value compared as a number of their type
*/
bool
d_tests_arg_cpp_validator_member()
{
    return steps_hold("MEMBER",
                      MEMBER_STEPS,
                      MEMBER_COUNT,
                      MEMBER);
}

/*
d_tests_arg_cpp_validator_transform
  CLI11's Transformer and CheckedTransformer over mappings, under filters.
  Tests the following:
  - a map of text, pairs in a vector, a map of numbers
  - filters, one, two and three
*/
bool
d_tests_arg_cpp_validator_transform()
{
    return steps_hold("TRANSFORM",
                      TRANSFORM_STEPS,
                      TRANSFORM_COUNT,
                      TRANSFORM);
}

/*
d_tests_arg_cpp_validator_units
  CLI11's AsNumberWithUnit and AsSizeValue.
  Tests the following:
  - integer and real factors; the default options and others; a name
  - sizes, a kilobyte a thousand bytes and 1024
*/
bool
d_tests_arg_cpp_validator_units()
{
    return steps_hold("UNITS",
                      UNITS_STEPS,
                      UNITS_COUNT,
                      UNITS);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

//==============================================================================
// 5.  PATH VALIDATORS
//==============================================================================

/*
d_tests_arg_cpp_validator_paths
  CLI11's path validators, among files a scratch directory holds.
  Tests the following:
  - files, folders and paths, existing and not
  - a file on a default path, its error returned and not
  - permissions -- execution as the system grants it -- and files that are
    not empty
*/
bool
d_tests_arg_cpp_validator_paths()
{
    return steps_hold("PATHS",
                      PATHS_STEPS,
                      PATHS_COUNT,
                      (D_TESTS_ARG_FILES_WINDOWS == 1) ? PATHS_WINDOWS
                                                       : PATHS);
}

/*
d_tests_arg_cpp_validator_path_sizes
  CLI11's FileSizeValidator, which the face has where a 64-bit integer is
declared -- every build but ISO strict C++98 on a system whose 64-bit
integer is `long long`, where the section holds its absence.
  Tests the following:
  - a file's size between two bounds, and from a least alone, for a file, an
    empty one, a folder and a path that does not exist
*/
bool
d_tests_arg_cpp_validator_path_sizes()
{
    return steps_hold("PATH_SIZES",
                      PATH_SIZES_STEPS,
                      PATH_SIZES_COUNT,
                      PATH_SIZES);
}

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 6.  WHAT C++11 BRINGS
//==============================================================================

/*
d_tests_arg_cpp_validator_cpp11
  What C++11 brings to CLI11's default validators: enumerations.
  Tests the following:
  - ranges over enumerations over `char`, of their own scope and not:
    compared and described as their numbers
*/
bool
d_tests_arg_cpp_validator_cpp11()
{
    return steps_hold("CPP11",
                      CPP11_STEPS,
                      CPP11_COUNT,
                      CPP11);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_validator_cpp11_extra
  What C++11 brings to CLI11's extra validators: enumerations and braced
lists.
  Tests the following:
  - bounds over enumerations over `char`, of their own scope and not:
    compared, described and clamped as their numbers
  - is_member over braced lists of text and numbers, and under a filter;
    transformer and checked_transformer over braced lists of pairs
*/
bool
d_tests_arg_cpp_validator_cpp11_extra()
{
    return steps_hold("CPP11_EXTRA",
                      CPP11_EXTRA_STEPS,
                      CPP11_EXTRA_COUNT,
                      CPP11_EXTRA);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 7.  THE FACE'S OWN
//==============================================================================

/*
d_tests_arg_cpp_validator_value
  A validator as a value: what the face's cli::validator is beyond CLI11's.
  Tests the following:
  - copies by construction and assignment, and an assignment from itself,
    each a validator of its own
  - swapped; handed over to C, which runs and releases it
  - a function's copy owned by each validator made or copied from it
*/
bool
d_tests_arg_cpp_validator_value()
{
    return steps_hold("VALUE",
                      VALUE_STEPS,
                      VALUE_COUNT,
                      VALUE);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_validator_value_extra
  A set over a C array: the face's is_member in a form CLI11's IsMember has
not.
  Tests the following:
  - is_member over a C array of text, with and without a filter, and over
    one of numbers
*/
bool
d_tests_arg_cpp_validator_value_extra()
{
    return steps_hold("VALUE_EXTRA",
                      VALUE_EXTRA_STEPS,
                      VALUE_EXTRA_COUNT,
                      VALUE_EXTRA);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

/*
d_tests_arg_cpp_validator_handles
  Handles on the validators an option holds.
  Tests the following:
  - by name, by index, and the first; settings made through one in a
    chain, read back through another
  - a copy made from one, which outlives the option's parser
*/
bool
d_tests_arg_cpp_validator_handles()
{
    return steps_hold("HANDLES",
                      HANDLES_STEPS,
                      HANDLES_COUNT,
                      HANDLES);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_validator_moved
  Validators moved, from C++11.
  Tests the following:
  - by construction and assignment, each source the default validator
  - operands moved into `|`; a validator moved into an option
*/
bool
d_tests_arg_cpp_validator_moved()
{
    return steps_hold("MOVED",
                      MOVED_STEPS,
                      MOVED_COUNT,
                      MOVED);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_validator_filters
  Filters as values, beside CLI11's filter functions.
  Tests the following:
  - none, each of CLI11's, by the function and by its call, combined with
    `|`, a function's, copied and swapped
  - combined filters given to sets: flags together, functions in order
*/
bool
d_tests_arg_cpp_validator_filters()
{
    return steps_hold("FILTERS",
                      FILTERS_STEPS,
                      FILTERS_COUNT,
                      FILTERS);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

//==============================================================================
// 8.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_validator_departures
  Where the face's validators depart from CLI11's (ISSUES 23, 37, 50).
  Tests the following:
  - handles on validators an option does not have: NULL, reading as the
    default validator
  - a change a rejecting function made, kept out of the option's result
  - 8-bit ranges described with their bounds as numbers
*/
bool
d_tests_arg_cpp_validator_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_validator_departures_extra
  Where the face's extra validators depart from CLI11's (ISSUES 23, 24, 37,
46).
  Tests the following:
  - `true.0.0.1` refused as an address
  - a clamped real number, 8-bit bounds and a float factor's product,
    written as their types write values
  - a refused size left as given in a parse
  - units refused when the validator is made: a failed validator, the
    definition error of an option given it
*/
bool
d_tests_arg_cpp_validator_departures_extra()
{
    return steps_hold("DEPARTED_EXTRA",
                      DEPARTED_EXTRA_STEPS,
                      DEPARTED_EXTRA_COUNT,
                      DEPARTED_EXTRA);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_validator_departures_cpp11
  Where ranges over enumerations depart from CLI11's (ISSUES 53).
  Tests the following:
  - an enumeration with an operator<< of its own: a range's bounds written
    as numbers
*/
bool
d_tests_arg_cpp_validator_departures_cpp11()
{
    return steps_hold("DEPARTED_CPP11",
                      DEPARTED_CPP11_STEPS,
                      DEPARTED_CPP11_COUNT,
                      DEPARTED_CPP11);
}

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

/*
d_tests_arg_cpp_validator_departures_cpp11_extra
  Where bounds over enumerations depart from CLI11's (ISSUES 53).
  Tests the following:
  - an enumeration with an operator<< of its own: a bound's bounds, and the
    values it clamps, written as numbers
*/
bool
d_tests_arg_cpp_validator_departures_cpp11_extra()
{
    return steps_hold("DEPARTED_CPP11_EXTRA",
                      DEPARTED_CPP11_EXTRA_STEPS,
                      DEPARTED_CPP11_EXTRA_COUNT,
                      DEPARTED_CPP11_EXTRA);
}

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 9.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_validator
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_validator[] =
{
    { "cpp_validator_defaults",
      d_tests_arg_cpp_validator_defaults },
    { "cpp_validator_settings",
      d_tests_arg_cpp_validator_settings },
    { "cpp_validator_composed",
      d_tests_arg_cpp_validator_composed },
    { "cpp_validator_in_a_parse",
      d_tests_arg_cpp_validator_in_a_parse },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_validator_extra",
      d_tests_arg_cpp_validator_extra },
    { "cpp_validator_member",
      d_tests_arg_cpp_validator_member },
    { "cpp_validator_transform",
      d_tests_arg_cpp_validator_transform },
    { "cpp_validator_units",
      d_tests_arg_cpp_validator_units },
#endif
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    { "cpp_validator_paths",
      d_tests_arg_cpp_validator_paths },
    { "cpp_validator_path_sizes",
      d_tests_arg_cpp_validator_path_sizes },
#endif
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_validator_cpp11",
      d_tests_arg_cpp_validator_cpp11 },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_validator_cpp11_extra",
      d_tests_arg_cpp_validator_cpp11_extra },
#endif
#endif
    { "cpp_validator_value",
      d_tests_arg_cpp_validator_value },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_validator_value_extra",
      d_tests_arg_cpp_validator_value_extra },
#endif
    { "cpp_validator_handles",
      d_tests_arg_cpp_validator_handles },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_validator_moved",
      d_tests_arg_cpp_validator_moved },
#endif
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_validator_filters",
      d_tests_arg_cpp_validator_filters },
#endif
    { "cpp_validator_departures",
      d_tests_arg_cpp_validator_departures },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_validator_departures_extra",
      d_tests_arg_cpp_validator_departures_extra },
#endif
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_validator_departures_cpp11",
      d_tests_arg_cpp_validator_departures_cpp11 },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { "cpp_validator_departures_cpp11_extra",
      d_tests_arg_cpp_validator_departures_cpp11_extra },
#endif
#endif
};

// d_tests_arg_cpp_validator_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_validator_count =
    sizeof(d_tests_arg_cpp_validator) / sizeof(d_tests_arg_cpp_validator[0]);
