/*******************************************************************************
* djinterp [parse]                                  arg_validator_member_tests.c
*
* The suite of CLI11's sets and mappings, declared in
* arg_validator_member_tests.h.
*   The transcripts are CLI11 2.7.2's, written as the core suite writes them
* (arg_tests_support.h): each set or mapping run over the values CLI11's was
* run over, under CLI11's rule for leading zeros. A set of a type stops at a
* value that does not convert, as CLI11's throws, and the value's line says
* `status`. One transcript departs from CLI11, and its constant says why: a
* set of a type of the caller's describes its members as they were given,
* where CLI11 streams them. What a set lists for completion, what owns a
* filter's context, and what cannot be made are this module's own, and are
* held to its contracts.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_member_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/
#include "./arg_validator_member_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcpy, strcpy
// djinterp
#include "../../../../../../inc/djinterp/c/util/sink_common.h"  // d_pack_text, d_pack_sink
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_common.h"  // statuses, buffers, sinks
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_type.h"  // descriptors
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator.h"  // running, choices, composites
#include "../../../../../../inc/djinterp/parse/cli/arg_parser/c/arg_validator_extra.h"  // the sets and mappings
#include "../../../c/tests_section.h"  // d_tests_section
#include "./arg_tests_support.h"  // transcripts, parses, kinds


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// WORDS
//   constant: the values the sets and mappings are run over, as CLI11's
// were: the members in and out of case, with and without underscores and
// spaces, other words, numbers in several spellings, and the empty text.
static const char* const WORDS[] =
{
    "alpha", "ALPHA", "Beta", "beta", "gamma_ray", "gammaray", "GAMMA_RAY",
    "gamma ray", "gam ma_ray", "delta", "", "1", "01", "0x2", "10", "1.0",
    "abc", " 3", "3 ", "2", "010", "one", "ONE", "two", "Three", "three", "x",
    "-0", "0", "0.5", "1.5", "1.50", "nan"
};

// WORD_COUNT
//   constant: how many values WORDS holds.
static const size_t WORD_COUNT = sizeof(WORDS) / sizeof(WORDS[0]);

// LETTERS
//   constant: the values the sets whose members differ only in case are run
// over, as CLI11's were.
static const char* const LETTERS[] =
{
    "a", "A", "b", "B", "c"
};

// LETTER_COUNT
//   constant: how many values LETTERS holds.
static const size_t LETTER_COUNT = sizeof(LETTERS) / sizeof(LETTERS[0]);

// MAPPED
//   constant: the values the mappings with a filter of the caller's are run
// over: keys, words that begin as keys do, a key in another case, a value a
// key maps to, another word, and the empty text.
static const char* const MAPPED[] =
{
    "one", "onerous", "two", "twofold", "Three", "Thresh", "three", "1", "x",
    ""
};

// MAPPED_COUNT
//   constant: how many values MAPPED holds.
static const size_t MAPPED_COUNT = sizeof(MAPPED) / sizeof(MAPPED[0]);

// MEMBERS
//   constant: CLI11's set {"alpha", "Beta", "gamma_ray"}.
static const char* const MEMBERS[] =
{
    "alpha", "Beta", "gamma_ray"
};

// MEMBER_COUNT
//   constant: how many members MEMBERS holds.
static const size_t MEMBER_COUNT = sizeof(MEMBERS) / sizeof(MEMBERS[0]);

// INTEGERS
//   constant: CLI11's set of `int` {1, 2, 3, 10}, as text.
static const char* const INTEGERS[] =
{
    "1", "2", "3", "10"
};

// REALS
//   constant: CLI11's set of `double` {0.5, 1.5, -0.0}, as text.
static const char* const REALS[] =
{
    "0.5", "1.5", "-0"
};

// LOWER_FIRST
//   constant: CLI11's set {"a", "A", "b"}: two members that one filter makes
// the same, the lowercase one first.
static const char* const LOWER_FIRST[] =
{
    "a", "A", "b"
};

// UPPER_FIRST
//   constant: CLI11's set {"A", "a", "b"}, the uppercase one first.
static const char* const UPPER_FIRST[] =
{
    "A", "a", "b"
};

// ONE_TWO
//   constant: CLI11's set of `int` {1, 2}, as text.
static const char* const ONE_TWO[] =
{
    "1", "2"
};

// PICKS
//   constant: CLI11's set {"a", "b"}, of the options picking one of them.
static const char* const PICKS[] =
{
    "a", "b"
};

// PAIRS
//   constant: CLI11's TransformPairs {{"one", "1"}, {"two", "2"},
// {"Three", "3"}}.
static const struct d_arg_mapping PAIRS[] =
{
    { "one",   "1" },
    { "two",   "2" },
    { "Three", "3" }
};

// PAIR_COUNT
//   constant: how many pairs PAIRS holds.
static const size_t PAIR_COUNT = sizeof(PAIRS) / sizeof(PAIRS[0]);

// INTEGER_PAIRS
//   constant: CLI11's pairs of `int` and text {{1, "one"}, {2, "two"},
// {10, "ten"}}.
static const struct d_arg_mapping INTEGER_PAIRS[] =
{
    { "1",  "one" },
    { "2",  "two" },
    { "10", "ten" }
};

// LEVELS
//   constant: CLI11's std::map {{"low", "1"}, {"high", "2"}}, in the order
// the map keeps its keys.
static const struct d_arg_mapping LEVELS[] =
{
    { "high", "2" },
    { "low",  "1" }
};

// CASE
//   constant: CLI11's ignore_case.
static const struct d_arg_filter CASE =
{
    D_ARG_FILTER_IGNORE_CASE,  // flags
    NULL,                      // function
    NULL,                      // context
    NULL,                      // copy
    NULL                       // release
};

// UNDERSCORE
//   constant: CLI11's ignore_underscore.
static const struct d_arg_filter UNDERSCORE =
{
    D_ARG_FILTER_IGNORE_UNDERSCORE,  // flags
    NULL,                            // function
    NULL,                            // context
    NULL,                            // copy
    NULL                             // release
};

// CASE_AND_UNDERSCORE
//   constant: CLI11's ignore_case and ignore_underscore, given together.
static const struct d_arg_filter CASE_AND_UNDERSCORE =
{
    D_ARG_FILTER_IGNORE_CASE |
    D_ARG_FILTER_IGNORE_UNDERSCORE,  // flags
    NULL,                            // function
    NULL,                            // context
    NULL,                            // copy
    NULL                             // release
};

// SPACE
//   constant: CLI11's ignore_space.
static const struct d_arg_filter SPACE =
{
    D_ARG_FILTER_IGNORE_SPACE,  // flags
    NULL,                       // function
    NULL,                       // context
    NULL,                       // copy
    NULL                        // release
};

// M_TEXT
//   constant: CLI11's IsMember({"alpha", "Beta", "gamma_ray"}).
static const char M_TEXT[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> fail <ALPHA not in {alpha,Beta,gamma_ray}>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> fail <beta not in {alpha,Beta,gamma_ray}>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> fail <gammaray not in {alpha,Beta,gamma_ray}>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,Beta,gamma_ray}>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// M_EMPTY
//   constant: CLI11's IsMember of no members.
static const char M_EMPTY[] =
    "desc <{}>\n"
    "[alpha] -> fail <alpha not in {}>\n"
    "[ALPHA] -> fail <ALPHA not in {}>\n"
    "[Beta] -> fail <Beta not in {}>\n"
    "[beta] -> fail <beta not in {}>\n"
    "[gamma_ray] -> fail <gamma_ray not in {}>\n"
    "[gammaray] -> fail <gammaray not in {}>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {}>\n"
    "[gamma ray] -> fail <gamma ray not in {}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {}>\n"
    "[delta] -> fail <delta not in {}>\n"
    "[] -> fail < not in {}>\n"
    "[1] -> fail <1 not in {}>\n"
    "[01] -> fail <01 not in {}>\n"
    "[0x2] -> fail <0x2 not in {}>\n"
    "[10] -> fail <10 not in {}>\n"
    "[1.0] -> fail <1.0 not in {}>\n"
    "[abc] -> fail <abc not in {}>\n"
    "[ 3] -> fail < 3 not in {}>\n"
    "[3 ] -> fail <3  not in {}>\n"
    "[2] -> fail <2 not in {}>\n"
    "[010] -> fail <010 not in {}>\n"
    "[one] -> fail <one not in {}>\n"
    "[ONE] -> fail <ONE not in {}>\n"
    "[two] -> fail <two not in {}>\n"
    "[Three] -> fail <Three not in {}>\n"
    "[three] -> fail <three not in {}>\n"
    "[x] -> fail <x not in {}>\n"
    "[-0] -> fail <-0 not in {}>\n"
    "[0] -> fail <0 not in {}>\n"
    "[0.5] -> fail <0.5 not in {}>\n"
    "[1.5] -> fail <1.5 not in {}>\n"
    "[1.50] -> fail <1.50 not in {}>\n"
    "[nan] -> fail <nan not in {}>\n";

// M_CASE
//   constant: CLI11's IsMember of MEMBERS with ignore_case.
static const char M_CASE[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <alpha>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <Beta>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> fail <gammaray not in {alpha,Beta,gamma_ray}>\n"
    "[GAMMA_RAY] -> pass <gamma_ray>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// M_UNDER
//   constant: CLI11's IsMember of MEMBERS with ignore_underscore.
static const char M_UNDER[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> fail <ALPHA not in {alpha,Beta,gamma_ray}>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> fail <beta not in {alpha,Beta,gamma_ray}>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gamma_ray>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,Beta,gamma_ray}>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// M_BOTH
//   constant: CLI11's IsMember of MEMBERS with ignore_case and
// ignore_underscore.
static const char M_BOTH[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <alpha>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <Beta>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gamma_ray>\n"
    "[GAMMA_RAY] -> pass <gamma_ray>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// M_SPACE
//   constant: CLI11's IsMember of MEMBERS with ignore_space.
static const char M_SPACE[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> fail <ALPHA not in {alpha,Beta,gamma_ray}>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> fail <beta not in {alpha,Beta,gamma_ray}>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> fail <gammaray not in {alpha,Beta,gamma_ray}>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,Beta,gamma_ray}>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> pass <gamma_ray>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// M_INT
//   constant: CLI11's IsMember of `int` {1, 2, 3, 10}.
static const char M_INT[] =
    "desc <{1,2,3,10}>\n"
    "[alpha] -> status <alpha>\n"
    "[ALPHA] -> status <ALPHA>\n"
    "[Beta] -> status <Beta>\n"
    "[beta] -> status <beta>\n"
    "[gamma_ray] -> status <gamma_ray>\n"
    "[gammaray] -> status <gammaray>\n"
    "[GAMMA_RAY] -> status <GAMMA_RAY>\n"
    "[gamma ray] -> status <gamma ray>\n"
    "[gam ma_ray] -> status <gam ma_ray>\n"
    "[delta] -> status <delta>\n"
    "[] -> status <>\n"
    "[1] -> pass <1>\n"
    "[01] -> pass <01>\n"
    "[0x2] -> pass <0x2>\n"
    "[10] -> pass <10>\n"
    "[1.0] -> status <1.0>\n"
    "[abc] -> status <abc>\n"
    "[ 3] -> pass < 3>\n"
    "[3 ] -> pass <3 >\n"
    "[2] -> pass <2>\n"
    "[010] -> fail <010 not in {1,2,3,10}>\n"
    "[one] -> status <one>\n"
    "[ONE] -> status <ONE>\n"
    "[two] -> status <two>\n"
    "[Three] -> status <Three>\n"
    "[three] -> status <three>\n"
    "[x] -> status <x>\n"
    "[-0] -> fail <-0 not in {1,2,3,10}>\n"
    "[0] -> fail <0 not in {1,2,3,10}>\n"
    "[0.5] -> status <0.5>\n"
    "[1.5] -> status <1.5>\n"
    "[1.50] -> status <1.50>\n"
    "[nan] -> status <nan>\n";

// M_DBL
//   constant: CLI11's IsMember of `double` {0.5, 1.5, -0.0}.
static const char M_DBL[] =
    "desc <{0.5,1.5,-0}>\n"
    "[alpha] -> status <alpha>\n"
    "[ALPHA] -> status <ALPHA>\n"
    "[Beta] -> status <Beta>\n"
    "[beta] -> status <beta>\n"
    "[gamma_ray] -> status <gamma_ray>\n"
    "[gammaray] -> status <gammaray>\n"
    "[GAMMA_RAY] -> status <GAMMA_RAY>\n"
    "[gamma ray] -> status <gamma ray>\n"
    "[gam ma_ray] -> status <gam ma_ray>\n"
    "[delta] -> status <delta>\n"
    "[] -> status <>\n"
    "[1] -> fail <1 not in {0.5,1.5,-0}>\n"
    "[01] -> fail <01 not in {0.5,1.5,-0}>\n"
    "[0x2] -> fail <0x2 not in {0.5,1.5,-0}>\n"
    "[10] -> fail <10 not in {0.5,1.5,-0}>\n"
    "[1.0] -> fail <1.0 not in {0.5,1.5,-0}>\n"
    "[abc] -> status <abc>\n"
    "[ 3] -> fail < 3 not in {0.5,1.5,-0}>\n"
    "[3 ] -> fail <3  not in {0.5,1.5,-0}>\n"
    "[2] -> fail <2 not in {0.5,1.5,-0}>\n"
    "[010] -> fail <010 not in {0.5,1.5,-0}>\n"
    "[one] -> status <one>\n"
    "[ONE] -> status <ONE>\n"
    "[two] -> status <two>\n"
    "[Three] -> status <Three>\n"
    "[three] -> status <three>\n"
    "[x] -> status <x>\n"
    "[-0] -> pass <-0>\n"
    "[0] -> pass <0>\n"
    "[0.5] -> pass <0.5>\n"
    "[1.5] -> pass <1.5>\n"
    "[1.50] -> pass <1.50>\n"
    "[nan] -> fail <nan not in {0.5,1.5,-0}>\n";

// M_DUPES
//   constant: CLI11's IsMember({"a", "A", "b"}, ignore_case), run over
// LETTERS.
static const char M_DUPES[] =
    "desc <{a,A,b}>\n"
    "[a] -> pass <a>\n"
    "[A] -> pass <a>\n"
    "[b] -> pass <b>\n"
    "[B] -> pass <b>\n"
    "[c] -> fail <c not in {a,A,b}>\n";

// M_DUPES2
//   constant: CLI11's IsMember({"A", "a", "b"}, ignore_case), run over
// LETTERS.
static const char M_DUPES2[] =
    "desc <{A,a,b}>\n"
    "[a] -> pass <a>\n"
    "[A] -> pass <a>\n"
    "[b] -> pass <b>\n"
    "[B] -> pass <b>\n"
    "[c] -> fail <c not in {A,a,b}>\n";

// X_TEXT
//   constant: CLI11's Transformer of PAIRS.
static const char X_TEXT[] =
    "desc <{one->1,two->2,Three->3}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <ALPHA>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <beta>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gammaray>\n"
    "[GAMMA_RAY] -> pass <GAMMA_RAY>\n"
    "[gamma ray] -> pass <gamma ray>\n"
    "[gam ma_ray] -> pass <gam ma_ray>\n"
    "[delta] -> pass <delta>\n"
    "[] -> pass <>\n"
    "[1] -> pass <1>\n"
    "[01] -> pass <01>\n"
    "[0x2] -> pass <0x2>\n"
    "[10] -> pass <10>\n"
    "[1.0] -> pass <1.0>\n"
    "[abc] -> pass <abc>\n"
    "[ 3] -> pass < 3>\n"
    "[3 ] -> pass <3 >\n"
    "[2] -> pass <2>\n"
    "[010] -> pass <010>\n"
    "[one] -> pass <1>\n"
    "[ONE] -> pass <ONE>\n"
    "[two] -> pass <2>\n"
    "[Three] -> pass <3>\n"
    "[three] -> pass <three>\n"
    "[x] -> pass <x>\n"
    "[-0] -> pass <-0>\n"
    "[0] -> pass <0>\n"
    "[0.5] -> pass <0.5>\n"
    "[1.5] -> pass <1.5>\n"
    "[1.50] -> pass <1.50>\n"
    "[nan] -> pass <nan>\n";

// X_CASE
//   constant: CLI11's Transformer of PAIRS with ignore_case.
static const char X_CASE[] =
    "desc <{one->1,two->2,Three->3}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <ALPHA>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <beta>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gammaray>\n"
    "[GAMMA_RAY] -> pass <GAMMA_RAY>\n"
    "[gamma ray] -> pass <gamma ray>\n"
    "[gam ma_ray] -> pass <gam ma_ray>\n"
    "[delta] -> pass <delta>\n"
    "[] -> pass <>\n"
    "[1] -> pass <1>\n"
    "[01] -> pass <01>\n"
    "[0x2] -> pass <0x2>\n"
    "[10] -> pass <10>\n"
    "[1.0] -> pass <1.0>\n"
    "[abc] -> pass <abc>\n"
    "[ 3] -> pass < 3>\n"
    "[3 ] -> pass <3 >\n"
    "[2] -> pass <2>\n"
    "[010] -> pass <010>\n"
    "[one] -> pass <1>\n"
    "[ONE] -> pass <1>\n"
    "[two] -> pass <2>\n"
    "[Three] -> pass <3>\n"
    "[three] -> pass <3>\n"
    "[x] -> pass <x>\n"
    "[-0] -> pass <-0>\n"
    "[0] -> pass <0>\n"
    "[0.5] -> pass <0.5>\n"
    "[1.5] -> pass <1.5>\n"
    "[1.50] -> pass <1.50>\n"
    "[nan] -> pass <nan>\n";

// X_INT
//   constant: CLI11's Transformer of INTEGER_PAIRS, keyed by `int`.
static const char X_INT[] =
    "desc <{1->one,2->two,10->ten}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <ALPHA>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <beta>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gammaray>\n"
    "[GAMMA_RAY] -> pass <GAMMA_RAY>\n"
    "[gamma ray] -> pass <gamma ray>\n"
    "[gam ma_ray] -> pass <gam ma_ray>\n"
    "[delta] -> pass <delta>\n"
    "[] -> pass <>\n"
    "[1] -> pass <one>\n"
    "[01] -> pass <one>\n"
    "[0x2] -> pass <two>\n"
    "[10] -> pass <ten>\n"
    "[1.0] -> pass <1.0>\n"
    "[abc] -> pass <abc>\n"
    "[ 3] -> pass < 3>\n"
    "[3 ] -> pass <3 >\n"
    "[2] -> pass <two>\n"
    "[010] -> pass <010>\n"
    "[one] -> pass <one>\n"
    "[ONE] -> pass <ONE>\n"
    "[two] -> pass <two>\n"
    "[Three] -> pass <Three>\n"
    "[three] -> pass <three>\n"
    "[x] -> pass <x>\n"
    "[-0] -> pass <-0>\n"
    "[0] -> pass <0>\n"
    "[0.5] -> pass <0.5>\n"
    "[1.5] -> pass <1.5>\n"
    "[1.50] -> pass <1.50>\n"
    "[nan] -> pass <nan>\n";

// C_TEXT
//   constant: CLI11's CheckedTransformer of PAIRS.
static const char C_TEXT[] =
    "desc <value in {one->1,two->2,Three->3} OR {1,2,3}>\n"
    "[alpha] -> fail <Check alpha value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[ALPHA] -> fail <Check ALPHA value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[Beta] -> fail <Check Beta value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[beta] -> fail <Check beta value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[gamma_ray] -> fail <Check gamma_ray value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[gammaray] -> fail <Check gammaray value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[GAMMA_RAY] -> fail <Check GAMMA_RAY value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[gamma ray] -> fail <Check gamma ray value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[gam ma_ray] -> fail <Check gam ma_ray value in {one->1,two->2,Three->3} "
    "OR {1,2,3} FAILED>\n"
    "[delta] -> fail <Check delta value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[] -> fail <Check  value in {one->1,two->2,Three->3} OR {1,2,3} FAILED>\n"
    "[1] -> pass <1>\n"
    "[01] -> fail <Check 01 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[0x2] -> fail <Check 0x2 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[10] -> fail <Check 10 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1.0] -> fail <Check 1.0 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[abc] -> fail <Check abc value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[ 3] -> fail <Check  3 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[3 ] -> fail <Check 3  value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[2] -> pass <2>\n"
    "[010] -> fail <Check 010 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[one] -> pass <1>\n"
    "[ONE] -> fail <Check ONE value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[two] -> pass <2>\n"
    "[Three] -> pass <3>\n"
    "[three] -> fail <Check three value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[x] -> fail <Check x value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[-0] -> fail <Check -0 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[0] -> fail <Check 0 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[0.5] -> fail <Check 0.5 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1.5] -> fail <Check 1.5 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1.50] -> fail <Check 1.50 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[nan] -> fail <Check nan value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n";

// C_CASE
//   constant: CLI11's CheckedTransformer of PAIRS with ignore_case.
static const char C_CASE[] =
    "desc <value in {one->1,two->2,Three->3} OR {1,2,3}>\n"
    "[alpha] -> fail <Check alpha value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[ALPHA] -> fail <Check ALPHA value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[Beta] -> fail <Check Beta value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[beta] -> fail <Check beta value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[gamma_ray] -> fail <Check gamma_ray value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[gammaray] -> fail <Check gammaray value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[GAMMA_RAY] -> fail <Check GAMMA_RAY value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[gamma ray] -> fail <Check gamma ray value in {one->1,two->2,Three->3} OR "
    "{1,2,3} FAILED>\n"
    "[gam ma_ray] -> fail <Check gam ma_ray value in {one->1,two->2,Three->3} "
    "OR {1,2,3} FAILED>\n"
    "[delta] -> fail <Check delta value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[] -> fail <Check  value in {one->1,two->2,Three->3} OR {1,2,3} FAILED>\n"
    "[1] -> pass <1>\n"
    "[01] -> fail <Check 01 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[0x2] -> fail <Check 0x2 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[10] -> fail <Check 10 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1.0] -> fail <Check 1.0 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[abc] -> fail <Check abc value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[ 3] -> fail <Check  3 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[3 ] -> fail <Check 3  value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[2] -> pass <2>\n"
    "[010] -> fail <Check 010 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[one] -> pass <1>\n"
    "[ONE] -> pass <1>\n"
    "[two] -> pass <2>\n"
    "[Three] -> pass <3>\n"
    "[three] -> pass <3>\n"
    "[x] -> fail <Check x value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[-0] -> fail <Check -0 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[0] -> fail <Check 0 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[0.5] -> fail <Check 0.5 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1.5] -> fail <Check 1.5 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1.50] -> fail <Check 1.50 value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[nan] -> fail <Check nan value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n";

// C_INT
//   constant: CLI11's CheckedTransformer of INTEGER_PAIRS, keyed by `int`.
static const char C_INT[] =
    "desc <value in {1->one,2->two,10->ten} OR {one,two,ten}>\n"
    "[alpha] -> fail <Check alpha value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[ALPHA] -> fail <Check ALPHA value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[Beta] -> fail <Check Beta value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[beta] -> fail <Check beta value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[gamma_ray] -> fail <Check gamma_ray value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[gammaray] -> fail <Check gammaray value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[GAMMA_RAY] -> fail <Check GAMMA_RAY value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[gamma ray] -> fail <Check gamma ray value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[gam ma_ray] -> fail <Check gam ma_ray value in {1->one,2->two,10->ten} "
    "OR {one,two,ten} FAILED>\n"
    "[delta] -> fail <Check delta value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[] -> fail <Check  value in {1->one,2->two,10->ten} OR {one,two,ten} "
    "FAILED>\n"
    "[1] -> pass <one>\n"
    "[01] -> pass <one>\n"
    "[0x2] -> pass <two>\n"
    "[10] -> pass <ten>\n"
    "[1.0] -> fail <Check 1.0 value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[abc] -> fail <Check abc value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[ 3] -> fail <Check  3 value in {1->one,2->two,10->ten} OR {one,two,ten} "
    "FAILED>\n"
    "[3 ] -> fail <Check 3  value in {1->one,2->two,10->ten} OR {one,two,ten} "
    "FAILED>\n"
    "[2] -> pass <two>\n"
    "[010] -> fail <Check 010 value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[one] -> pass <one>\n"
    "[ONE] -> fail <Check ONE value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[two] -> pass <two>\n"
    "[Three] -> fail <Check Three value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[three] -> fail <Check three value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[x] -> fail <Check x value in {1->one,2->two,10->ten} OR {one,two,ten} "
    "FAILED>\n"
    "[-0] -> fail <Check -0 value in {1->one,2->two,10->ten} OR {one,two,ten} "
    "FAILED>\n"
    "[0] -> fail <Check 0 value in {1->one,2->two,10->ten} OR {one,two,ten} "
    "FAILED>\n"
    "[0.5] -> fail <Check 0.5 value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[1.5] -> fail <Check 1.5 value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[1.50] -> fail <Check 1.50 value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n"
    "[nan] -> fail <Check nan value in {1->one,2->two,10->ten} OR "
    "{one,two,ten} FAILED>\n";

// K_AND
//   constant: CLI11's IsMember of `int` {1, 2} & Range(0, 5).
static const char K_AND[] =
    "desc <({1,2}) AND (INT in [0 - 5])>\n"
    "[alpha] -> status <alpha>\n"
    "[ALPHA] -> status <ALPHA>\n"
    "[Beta] -> status <Beta>\n"
    "[beta] -> status <beta>\n"
    "[gamma_ray] -> status <gamma_ray>\n"
    "[gammaray] -> status <gammaray>\n"
    "[GAMMA_RAY] -> status <GAMMA_RAY>\n"
    "[gamma ray] -> status <gamma ray>\n"
    "[gam ma_ray] -> status <gam ma_ray>\n"
    "[delta] -> status <delta>\n"
    "[] -> status <>\n"
    "[1] -> pass <1>\n"
    "[01] -> pass <01>\n"
    "[0x2] -> pass <0x2>\n"
    "[10] -> fail <(10 not in {1,2}) AND (Value 10 not in range [0 - 5])>\n"
    "[1.0] -> status <1.0>\n"
    "[abc] -> status <abc>\n"
    "[ 3] -> fail < 3 not in {1,2}>\n"
    "[3 ] -> fail <3  not in {1,2}>\n"
    "[2] -> pass <2>\n"
    "[010] -> fail <(010 not in {1,2}) AND (Value 010 not in range [0 - 5])>\n"
    "[one] -> status <one>\n"
    "[ONE] -> status <ONE>\n"
    "[two] -> status <two>\n"
    "[Three] -> status <Three>\n"
    "[three] -> status <three>\n"
    "[x] -> status <x>\n"
    "[-0] -> fail <-0 not in {1,2}>\n"
    "[0] -> fail <0 not in {1,2}>\n"
    "[0.5] -> status <0.5>\n"
    "[1.5] -> status <1.5>\n"
    "[1.50] -> status <1.50>\n"
    "[nan] -> status <nan>\n";

// F_PREFIX
//   constant: CLI11's IsMember of MEMBERS with a filter function that keeps
// the first three characters.
static const char F_PREFIX[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> fail <ALPHA not in {alpha,Beta,gamma_ray}>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> fail <beta not in {alpha,Beta,gamma_ray}>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gamma_ray>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,Beta,gamma_ray}>\n"
    "[gamma ray] -> pass <gamma_ray>\n"
    "[gam ma_ray] -> pass <gamma_ray>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// F_ORDER
//   constant: CLI11's IsMember of MEMBERS with ignore_case, then a filter
// function that drops uppercase letters, which finds none left to drop.
static const char F_ORDER[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <alpha>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <Beta>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> fail <gammaray not in {alpha,Beta,gamma_ray}>\n"
    "[GAMMA_RAY] -> pass <gamma_ray>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// F_UNDER
//   constant: CLI11's IsMember of MEMBERS with a filter function that drops
// `_`.
static const char F_UNDER[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> fail <ALPHA not in {alpha,Beta,gamma_ray}>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> fail <beta not in {alpha,Beta,gamma_ray}>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> pass <gamma_ray>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,Beta,gamma_ray}>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// F_A
//   constant: CLI11's IsMember of MEMBERS with a filter function that drops
// `a`.
static const char F_A[] =
    "desc <{alpha,Beta,gamma_ray}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> fail <ALPHA not in {alpha,Beta,gamma_ray}>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> fail <beta not in {alpha,Beta,gamma_ray}>\n"
    "[gamma_ray] -> pass <gamma_ray>\n"
    "[gammaray] -> fail <gammaray not in {alpha,Beta,gamma_ray}>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,Beta,gamma_ray}>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,Beta,gamma_ray}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,Beta,gamma_ray}>\n"
    "[delta] -> fail <delta not in {alpha,Beta,gamma_ray}>\n"
    "[] -> fail < not in {alpha,Beta,gamma_ray}>\n"
    "[1] -> fail <1 not in {alpha,Beta,gamma_ray}>\n"
    "[01] -> fail <01 not in {alpha,Beta,gamma_ray}>\n"
    "[0x2] -> fail <0x2 not in {alpha,Beta,gamma_ray}>\n"
    "[10] -> fail <10 not in {alpha,Beta,gamma_ray}>\n"
    "[1.0] -> fail <1.0 not in {alpha,Beta,gamma_ray}>\n"
    "[abc] -> fail <abc not in {alpha,Beta,gamma_ray}>\n"
    "[ 3] -> fail < 3 not in {alpha,Beta,gamma_ray}>\n"
    "[3 ] -> fail <3  not in {alpha,Beta,gamma_ray}>\n"
    "[2] -> fail <2 not in {alpha,Beta,gamma_ray}>\n"
    "[010] -> fail <010 not in {alpha,Beta,gamma_ray}>\n"
    "[one] -> fail <one not in {alpha,Beta,gamma_ray}>\n"
    "[ONE] -> fail <ONE not in {alpha,Beta,gamma_ray}>\n"
    "[two] -> fail <two not in {alpha,Beta,gamma_ray}>\n"
    "[Three] -> fail <Three not in {alpha,Beta,gamma_ray}>\n"
    "[three] -> fail <three not in {alpha,Beta,gamma_ray}>\n"
    "[x] -> fail <x not in {alpha,Beta,gamma_ray}>\n"
    "[-0] -> fail <-0 not in {alpha,Beta,gamma_ray}>\n"
    "[0] -> fail <0 not in {alpha,Beta,gamma_ray}>\n"
    "[0.5] -> fail <0.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.5] -> fail <1.5 not in {alpha,Beta,gamma_ray}>\n"
    "[1.50] -> fail <1.50 not in {alpha,Beta,gamma_ray}>\n"
    "[nan] -> fail <nan not in {alpha,Beta,gamma_ray}>\n";

// FX_PREFIX
//   constant: CLI11's Transformer of PAIRS with the filter function that
// keeps the first three characters, run over MAPPED.
static const char FX_PREFIX[] =
    "desc <{one->1,two->2,Three->3}>\n"
    "[one] -> pass <1>\n"
    "[onerous] -> pass <1>\n"
    "[two] -> pass <2>\n"
    "[twofold] -> pass <2>\n"
    "[Three] -> pass <3>\n"
    "[Thresh] -> pass <3>\n"
    "[three] -> pass <three>\n"
    "[1] -> pass <1>\n"
    "[x] -> pass <x>\n"
    "[] -> pass <>\n";

// FC_PREFIX
//   constant: CLI11's CheckedTransformer of PAIRS with the same filter, run
// over MAPPED.
static const char FC_PREFIX[] =
    "desc <value in {one->1,two->2,Three->3} OR {1,2,3}>\n"
    "[one] -> pass <1>\n"
    "[onerous] -> pass <1>\n"
    "[two] -> pass <2>\n"
    "[twofold] -> pass <2>\n"
    "[Three] -> pass <3>\n"
    "[Thresh] -> pass <3>\n"
    "[three] -> fail <Check three value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[1] -> pass <1>\n"
    "[x] -> fail <Check x value in {one->1,two->2,Three->3} OR {1,2,3} "
    "FAILED>\n"
    "[] -> fail <Check  value in {one->1,two->2,Three->3} OR {1,2,3} FAILED>\n";

// L_SAME
//   constant: CLI11's IsMember of a type of the caller's -- text read whole
// and lowered, equal when the text is -- with the members "alpha" and
// "beta".
static const char L_SAME[] =
    "desc <{alpha,beta}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <ALPHA>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <beta>\n"
    "[gamma_ray] -> fail <gamma_ray not in {alpha,beta}>\n"
    "[gammaray] -> fail <gammaray not in {alpha,beta}>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {alpha,beta}>\n"
    "[gamma ray] -> fail <gamma ray not in {alpha,beta}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {alpha,beta}>\n"
    "[delta] -> fail <delta not in {alpha,beta}>\n"
    "[] -> status <>\n"
    "[1] -> fail <1 not in {alpha,beta}>\n"
    "[01] -> fail <01 not in {alpha,beta}>\n"
    "[0x2] -> fail <0x2 not in {alpha,beta}>\n"
    "[10] -> fail <10 not in {alpha,beta}>\n"
    "[1.0] -> fail <1.0 not in {alpha,beta}>\n"
    "[abc] -> fail <abc not in {alpha,beta}>\n"
    "[ 3] -> fail < 3 not in {alpha,beta}>\n"
    "[3 ] -> fail <3  not in {alpha,beta}>\n"
    "[2] -> fail <2 not in {alpha,beta}>\n"
    "[010] -> fail <010 not in {alpha,beta}>\n"
    "[one] -> fail <one not in {alpha,beta}>\n"
    "[ONE] -> fail <ONE not in {alpha,beta}>\n"
    "[two] -> fail <two not in {alpha,beta}>\n"
    "[Three] -> fail <Three not in {alpha,beta}>\n"
    "[three] -> fail <three not in {alpha,beta}>\n"
    "[x] -> fail <x not in {alpha,beta}>\n"
    "[-0] -> fail <-0 not in {alpha,beta}>\n"
    "[0] -> fail <0 not in {alpha,beta}>\n"
    "[0.5] -> fail <0.5 not in {alpha,beta}>\n"
    "[1.5] -> fail <1.5 not in {alpha,beta}>\n"
    "[1.50] -> fail <1.50 not in {alpha,beta}>\n"
    "[nan] -> fail <nan not in {alpha,beta}>\n";

// L_GIVEN
//   constant: the same set made from the members "Alpha" and "BETA". The
// members are described, and listed in a refusal, as they were given, where
// CLI11 streams the objects it made of them, `{alpha,beta}`: a set keeps
// its members as text and converts them only to compare, so the text given
// is the only text it has. Every verdict is CLI11's.
static const char L_GIVEN[] =
    "desc <{Alpha,BETA}>\n"
    "[alpha] -> pass <alpha>\n"
    "[ALPHA] -> pass <ALPHA>\n"
    "[Beta] -> pass <Beta>\n"
    "[beta] -> pass <beta>\n"
    "[gamma_ray] -> fail <gamma_ray not in {Alpha,BETA}>\n"
    "[gammaray] -> fail <gammaray not in {Alpha,BETA}>\n"
    "[GAMMA_RAY] -> fail <GAMMA_RAY not in {Alpha,BETA}>\n"
    "[gamma ray] -> fail <gamma ray not in {Alpha,BETA}>\n"
    "[gam ma_ray] -> fail <gam ma_ray not in {Alpha,BETA}>\n"
    "[delta] -> fail <delta not in {Alpha,BETA}>\n"
    "[] -> status <>\n"
    "[1] -> fail <1 not in {Alpha,BETA}>\n"
    "[01] -> fail <01 not in {Alpha,BETA}>\n"
    "[0x2] -> fail <0x2 not in {Alpha,BETA}>\n"
    "[10] -> fail <10 not in {Alpha,BETA}>\n"
    "[1.0] -> fail <1.0 not in {Alpha,BETA}>\n"
    "[abc] -> fail <abc not in {Alpha,BETA}>\n"
    "[ 3] -> fail < 3 not in {Alpha,BETA}>\n"
    "[3 ] -> fail <3  not in {Alpha,BETA}>\n"
    "[2] -> fail <2 not in {Alpha,BETA}>\n"
    "[010] -> fail <010 not in {Alpha,BETA}>\n"
    "[one] -> fail <one not in {Alpha,BETA}>\n"
    "[ONE] -> fail <ONE not in {Alpha,BETA}>\n"
    "[two] -> fail <two not in {Alpha,BETA}>\n"
    "[Three] -> fail <Three not in {Alpha,BETA}>\n"
    "[three] -> fail <three not in {Alpha,BETA}>\n"
    "[x] -> fail <x not in {Alpha,BETA}>\n"
    "[-0] -> fail <-0 not in {Alpha,BETA}>\n"
    "[0] -> fail <0 not in {Alpha,BETA}>\n"
    "[0.5] -> fail <0.5 not in {Alpha,BETA}>\n"
    "[1.5] -> fail <1.5 not in {Alpha,BETA}>\n"
    "[1.50] -> fail <1.50 not in {Alpha,BETA}>\n"
    "[nan] -> fail <nan not in {Alpha,BETA}>\n";

// P_PICK_CHECKED
//   constant: CLI11's `--pick A`, the option bound to text and checked by
// IsMember({"a", "b"}, ignore_case).
static const char P_PICK_CHECKED[] =
    "cmd app n=1 miss=[] subs=[] order=[--pick]\n"
    "opt --pick n=1 res=[A]\n"
    "value [A]\n";

// P_PICK_REFUSED
//   constant: CLI11's `--pick z`, to the same option.
static const char P_PICK_REFUSED[] =
    "ERR ValidationError 105: --pick: z not in {a,b}\n"
    "cmd app n=1 miss=[] subs=[] order=[--pick]\n"
    "opt --pick n=1 res=[z]\n"
    "value []\n";

// P_PICK_TRANSFORMED
//   constant: CLI11's `--pick A`, the set added by transform() instead.
static const char P_PICK_TRANSFORMED[] =
    "cmd app n=1 miss=[] subs=[] order=[--pick]\n"
    "opt --pick n=1 res=[a]\n"
    "value [a]\n";

// P_LEVEL_KEY
//   constant: CLI11's `--level HIGH`, the option bound to text and
// transformed by CheckedTransformer of LEVELS with ignore_case.
static const char P_LEVEL_KEY[] =
    "cmd app n=1 miss=[] subs=[] order=[--level]\n"
    "opt --level n=1 res=[2]\n"
    "value [2]\n";

// P_LEVEL_VALUE
//   constant: CLI11's `--level 2`, to the same option.
static const char P_LEVEL_VALUE[] =
    "cmd app n=1 miss=[] subs=[] order=[--level]\n"
    "opt --level n=1 res=[2]\n"
    "value [2]\n";

// P_LEVEL_REFUSED
//   constant: CLI11's `--level 3`, to the same option.
static const char P_LEVEL_REFUSED[] =
    "ERR ValidationError 105: --level: Check 3 value in {high->2,low->1} OR "
    "{2,1} FAILED\n"
    "cmd app n=1 miss=[] subs=[] order=[--level]\n"
    "opt --level n=1 res=[3]\n"
    "value []\n";

// P_MEMBER_INT_WORD
//   constant: CLI11's `--u abc`, the option bound to an `int` and checked
// by IsMember of `int` {1, 2}.
static const char P_MEMBER_INT_WORD[] =
    "ERR ValidationError 105: --u: abc\n"
    "cmd app n=1 miss=[] subs=[] order=[--u]\n"
    "opt --u n=1 res=[abc]\n"
    "value [0]\n";

// P_MEMBER_INT_OUT
//   constant: CLI11's `--u 3`, to the same option.
static const char P_MEMBER_INT_OUT[] =
    "ERR ValidationError 105: --u: 3 not in {1,2}\n"
    "cmd app n=1 miss=[] subs=[] order=[--u]\n"
    "opt --u n=1 res=[3]\n"
    "value [0]\n";

// P_MEMBER_INT_IN
//   constant: CLI11's `--u 2`, to the same option.
static const char P_MEMBER_INT_IN[] =
    "cmd app n=1 miss=[] subs=[] order=[--u]\n"
    "opt --u n=1 res=[2]\n"
    "value [2]\n";

// P_MEMBER_INT_EMPTY
//   constant: CLI11's `--u ""`, to the same option. The set cannot convert
// the empty value and stops with the value itself as its message, as CLI11
// throws it; an empty message refuses nothing, so the value passes, and
// becomes the type's zero.
static const char P_MEMBER_INT_EMPTY[] =
    "cmd app n=1 miss=[] subs=[] order=[--u]\n"
    "opt --u n=1 res=[]\n"
    "value [0]\n";

// PICK_OPTION
//   constant: CLI11's `add_option("--pick", text)`, checked.
static const struct d_tests_arg_checked PICK_OPTION =
{
    "--pick",
    D_ARG_BUILTIN_STRING,
    false
};

// PICK_TRANSFORMED
//   constant: the same option, its set added by transform().
static const struct d_tests_arg_checked PICK_TRANSFORMED =
{
    "--pick",
    D_ARG_BUILTIN_STRING,
    true
};

// LEVEL_OPTION
//   constant: CLI11's `add_option("--level", text)`, transformed.
static const struct d_tests_arg_checked LEVEL_OPTION =
{
    "--level",
    D_ARG_BUILTIN_STRING,
    true
};

// MEMBER_OPTION
//   constant: CLI11's `add_option("--u", whole)`, checked.
static const struct d_tests_arg_checked MEMBER_OPTION =
{
    "--u",
    D_ARG_BUILTIN_INT,
    false
};

// LOWER_MAX
//   constant: how many characters a LOWER holds.
enum
{
    LOWER_MAX = 32
};

// d_tests_arg_member_lower
//   struct: an object of LOWER, a type of the caller's: text with its ASCII
// letters lowered, as CLI11's reference reads its own type from a stream,
// whole, and its length.
struct d_tests_arg_member_lower
{
    char   letters[LOWER_MAX + 1];
    size_t length;
};

// d_tests_arg_member_counts
//   struct: what has been done to a filter's context and its copies: how
// many copies were made, how many contexts were freed, and how many copies
// may be made before the next is refused, as an allocation would be.
struct d_tests_arg_member_counts
{
    int copies;
    int releases;
    int allowed;
};

// d_tests_arg_member_drop
//   struct: the context of the filter that drops one character: the
// character, and the counts it shares with its copies.
struct d_tests_arg_member_drop
{
    char                              character;
    struct d_tests_arg_member_counts* counts;
};

/**
 * @brief LOWER's conversion: one value of one to LOWER_MAX characters, its
 *        ASCII letters lowered.
 *
 * @param[in]  _type     the descriptor; unused.
 * @param[in]  _values   the values.
 * @param[in]  _count    how many; one converts.
 * @param[out] _target   receives the LOWER.
 * @param[in]  _context  the conversion's context; unused.
 * @return D_ARG_OK; D_ARG_ERROR_CONVERSION for a count other than one, or a
 *         value empty or too long.
 *
 * The empty text is no LOWER, as the reference's stream reads nothing from
 * it.
 */
static int
d_tests_arg_member_lower_convert(
    const struct d_arg_type*            _type,
    const struct d_pack_text*           _values,
    size_t                              _count,
    void*                               _target,
    const struct d_arg_convert_context* _context
)
{
    struct d_tests_arg_member_lower lower = { { 0 }, 0u };

    (void)_type;
    (void)_context;

    // one value, of a length a LOWER holds
    if ( (_count != 1u)                          ||
         (_values[0].length == 0u)               ||
         (_values[0].length > (size_t)LOWER_MAX) )
    {
        return D_ARG_ERROR_CONVERSION;
    }

    // each character, an uppercase letter lowered
    for (size_t index = 0u; index < _values[0].length; ++index)
    {
        const char character = _values[0].data[index];

        lower.letters[index] = ( (character >= 'A') &&
                                 (character <= 'Z') )
                               ? (char)(character - 'A' + 'a')
                               : character;
    }

    lower.length = _values[0].length;
    memcpy(_target,
           &lower,
           sizeof(lower));

    return D_ARG_OK;
}

/**
 * @brief LOWER's text: its letters, as the reference streams its type.
 *
 * @param[in] _type   the descriptor; unused.
 * @param[in] _value  the LOWER.
 * @param[in] _sink   receives the text.
 * @return D_ARG_OK, or the sink's refusal.
 */
static int
d_tests_arg_member_lower_format(
    const struct d_arg_type* _type,
    const void*              _value,
    struct d_pack_sink       _sink
)
{
    const struct d_tests_arg_member_lower* const lower = _value;

    (void)_type;

    return d_arg_sink_write(_sink,
                            lower->letters,
                            lower->length);
}

// LOWER_TYPE
//   constant: LOWER's descriptor, which converts and writes its values, and
// so is compared by the text it writes.
static const struct d_arg_type LOWER_TYPE =
{
    "LOWER",                                  // name
    d_tests_arg_member_lower_convert,         // convert
    d_tests_arg_member_lower_format,          // format
    NULL,                                     // data
    sizeof(struct d_tests_arg_member_lower),  // size
    1,                                        // type_size_min
    1,                                        // type_size_max
    1,                                        // expected
    0u                                        // flags
};

/**
 * @brief A filter of the caller's: the first three characters of a value,
 *        as the reference's `s.substr(0, 3)`.
 *
 * @param[in]     _value    the text to filter.
 * @param[in,out] _out      the empty buffer that receives the filtered text.
 * @param[in]     _context  unused.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_member_prefix(
    struct d_pack_text   _value,
    struct d_arg_buffer* _out,
    void*                _context
)
{
    const size_t length = (_value.length < 3u)
                          ? _value.length
                          : 3u;

    (void)_context;

    return d_arg_buffer_append(_out,
                               _value.data,
                               length);
}

/**
 * @brief A filter of the caller's: a value without its uppercase ASCII
 *        letters.
 *
 * @param[in]     _value    the text to filter.
 * @param[in,out] _out      the empty buffer that receives the filtered text.
 * @param[in]     _context  unused.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_member_drop_upper(
    struct d_pack_text   _value,
    struct d_arg_buffer* _out,
    void*                _context
)
{
    int status = D_ARG_OK;

    (void)_context;

    // each character but an uppercase letter
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _value.length);
         ++index)
    {
        const char character = _value.data[index];

        // an uppercase letter is dropped
        if ( (character >= 'A') &&
             (character <= 'Z') )
        {
            continue;
        }

        status = d_arg_buffer_append_char(_out,
                                          character);
    }

    return status;
}

/**
 * @brief A filter of the caller's that reads its context: a value without
 *        the character a d_tests_arg_member_drop names.
 *
 * @param[in]     _value    the text to filter.
 * @param[in,out] _out      the empty buffer that receives the filtered text.
 * @param[in]     _context  the d_tests_arg_member_drop.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_member_drop_filter(
    struct d_pack_text   _value,
    struct d_arg_buffer* _out,
    void*                _context
)
{
    const struct d_tests_arg_member_drop* const drop   = _context;
    int                                         status = D_ARG_OK;

    // each character but the one named
    for (size_t index = 0u;
         (status == D_ARG_OK) && (index < _value.length);
         ++index)
    {
        // the character named is dropped
        if (_value.data[index] == drop->character)
        {
            continue;
        }

        status = d_arg_buffer_append_char(_out,
                                          _value.data[index]);
    }

    return status;
}

/**
 * @brief Copies a drop filter's context, counting the copy.
 *
 * @param[in] _context  the d_tests_arg_member_drop to copy.
 * @return the copy, which d_tests_arg_member_drop_release frees; NULL once
 *         the copies its counts allow are made, as a refused allocation
 *         would be, or when one is refused.
 */
static void*
d_tests_arg_member_drop_copy(
    const void* _context
)
{
    const struct d_tests_arg_member_drop* const source = _context;

    // a copy past those allowed is refused
    if (source->counts->copies >= source->counts->allowed)
    {
        return NULL;
    }

    struct d_tests_arg_member_drop* const copy = malloc(sizeof(*copy));

    // a copy made counts
    if (copy != NULL)
    {
        *copy = *source;
        ++copy->counts->copies;
    }

    return copy;
}

/**
 * @brief Frees a copy of a drop filter's context, counting the release.
 *
 * @param[in] _context  the copy.
 */
static void
d_tests_arg_member_drop_release(
    void* _context
)
{
    struct d_tests_arg_member_drop* const drop = _context;

    ++drop->counts->releases;
    free(drop);

    return;
}

/**
 * @brief A filter of the caller's that cannot filter.
 *
 * @param[in]     _value    unused.
 * @param[in,out] _out      unused.
 * @param[in]     _context  unused.
 * @return D_ARG_ERROR_OUT_OF_MEMORY, as from a filter whose buffer could
 *         not grow.
 */
static int
d_tests_arg_member_refuse(
    struct d_pack_text   _value,
    struct d_arg_buffer* _out,
    void*                _context
)
{
    (void)_value;
    (void)_out;
    (void)_context;

    return D_ARG_ERROR_OUT_OF_MEMORY;
}

/**
 * @brief Runs a set or mapping over WORDS and compares its transcript with
 *        the one expected.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the set or mapping, consumed.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_member_words(
    const char*            _what,
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    return d_tests_arg_holds(_what,
                             _validator,
                             WORDS,
                             WORD_COUNT,
                             _expected);
}

/**
 * @brief CLI11's IsMember of MEMBERS, as text, run over WORDS.
 *
 * @param[in] _what      what the transcript is, for a printed mismatch.
 * @param[in] _filter    the set's filter; NULL for none.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_member_set(
    const char*                _what,
    const struct d_arg_filter* _filter,
    const char*                _expected
)
{
    return d_tests_arg_member_words(_what,
                                    d_arg_validator_is_member(NULL,
                                                              MEMBERS,
                                                              MEMBER_COUNT,
                                                              _filter),
                                    _expected);
}

/**
 * @brief Runs a mapping over MAPPED and compares its transcript with the
 *        one expected.
 *
 * @param[in] _what       what the transcript is, for a printed mismatch.
 * @param[in] _validator  the mapping, consumed.
 * @param[in] _expected   the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_member_mapped(
    const char*            _what,
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    return d_tests_arg_holds(_what,
                             _validator,
                             MAPPED,
                             MAPPED_COUNT,
                             _expected);
}

/**
 * @brief CLI11's IsMember of MEMBERS through the drop filter.
 *
 * @param[in] _drop   the filter's context.
 * @param[in] _owned  whether the set owns a copy of the context, made and
 *                    freed through the counting copy and release; when
 *                    false the set borrows it, and the release is ignored.
 * @return the set, or a failed validator, as d_arg_validator_is_member's.
 */
static struct d_arg_validator
d_tests_arg_member_dropping(
    struct d_tests_arg_member_drop* _drop,
    bool                            _owned
)
{
    const struct d_arg_filter filter =
    {
        0u,                                                // flags
        d_tests_arg_member_drop_filter,                    // function
        _drop,                                             // context
        (_owned) ? d_tests_arg_member_drop_copy : NULL,    // copy
        d_tests_arg_member_drop_release                    // release
    };

    return d_arg_validator_is_member(NULL,
                                     MEMBERS,
                                     MEMBER_COUNT,
                                     &filter);
}

/**
 * @brief A d_arg_choice_fn that appends each choice and a `|` to a buffer.
 *
 * @param[in] _choice   the choice.
 * @param[in] _context  the d_arg_buffer.
 * @return D_ARG_OK, or the buffer's status.
 */
static int
d_tests_arg_member_collect(
    struct d_pack_text _choice,
    void*              _context
)
{
    struct d_arg_buffer* const out = _context;

    (void)d_arg_buffer_append_text(out,
                                   _choice);

    return d_arg_buffer_append_char(out,
                                    '|');
}

/**
 * @brief A d_arg_choice_fn that stops a listing at `b`.
 *
 * @param[in] _choice   the choice.
 * @param[in] _context  unused.
 * @return D_ARG_ERROR_HORRIBLE for `b`; D_ARG_OK for any other choice.
 */
static int
d_tests_arg_member_stop_at_b(
    struct d_pack_text _choice,
    void*              _context
)
{
    (void)_context;

    return ( (_choice.length == 1u) &&
             (_choice.data[0] == 'b') )
           ? D_ARG_ERROR_HORRIBLE
           : D_ARG_OK;
}

/**
 * @brief Whether a validator lists the choices expected.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _expected   the choices, each followed by `|`.
 * @return whether the listing succeeded and was as expected.
 */
static bool
d_tests_arg_member_listed(
    struct d_arg_validator _validator,
    const char*            _expected
)
{
    struct d_arg_buffer listed = D_ARG_BUFFER_INIT;
    const int           status =
        d_arg_validator_each_choice(&_validator,
                                    d_tests_arg_member_collect,
                                    &listed);
    const bool          same   = d_tests_arg_same("choices",
                                                  d_arg_buffer_text(&listed),
                                                  _expected);

    d_arg_buffer_release(&listed);
    d_arg_validator_release(&_validator);

    return ( (status == D_ARG_OK) &&
             (same) );
}

/**
 * @brief Whether running a validator over a value stops with the status
 *        expected.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _value      the value.
 * @param[in] _status     the status expected.
 * @return whether d_arg_validator_run returned it.
 */
static bool
d_tests_arg_member_runs_as(
    struct d_arg_validator _validator,
    const char*            _value,
    int                    _status
)
{
    struct d_arg_validation validation =
    {
        { NULL, 0u },
        D_ARG_BUFFER_INIT,
        D_ARG_BUFFER_INIT,
        NULL,
        0
    };

    d_arg_validation_init(&validation,
                          d_pack_text_from_cstr(_value));

    const int status = d_arg_validator_run(&_validator,
                                           &validation);

    d_arg_validation_release(&validation);
    d_arg_validator_release(&_validator);

    return (status == _status);
}

/**
 * @brief Whether a set or mapping could not be made.
 *
 * @param[in] _validator  the validator, consumed.
 * @param[in] _status     the failure expected.
 * @return whether the validator holds the failure and runs as it.
 */
static bool
d_tests_arg_member_unmade(
    struct d_arg_validator _validator,
    int                    _status
)
{
    const bool holds = (_validator.status == _status);

    return ( (d_tests_arg_member_runs_as(_validator,
                                         "1",
                                         _status)) &&
             (holds) );
}

/**
 * @brief The sets and mappings whose text is missing.
 *
 * @return whether each is a failed validator holding
 *         D_ARG_ERROR_BAD_ARGUMENT.
 */
static bool
d_tests_arg_member_unmade_text(void)
{
    static const char* const          HOLED[]   = { "a", NULL };
    static const struct d_arg_mapping NO_TO[]   = { { "a", NULL } };
    static const struct d_arg_mapping NO_FROM[] = { { NULL, "a" } };

    const int bad = D_ARG_ERROR_BAD_ARGUMENT;

    return ( (d_tests_arg_member_unmade(
                  d_arg_validator_is_member(NULL,
                                            NULL,
                                            2u,
                                            NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_transformer(NULL,
                                              NULL,
                                              1u,
                                              NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_is_member(NULL,
                                            HOLED,
                                            2u,
                                            NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_transformer(NULL,
                                              NO_TO,
                                              1u,
                                              NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_checked_transformer(NULL,
                                                      NO_FROM,
                                                      1u,
                                                      NULL),
                  bad)) );
}

/**
 * @brief The sets and mappings whose type cannot compare them.
 *
 * @return whether each is a failed validator holding
 *         D_ARG_ERROR_BAD_ARGUMENT.
 */
static bool
d_tests_arg_member_unmade_types(void)
{
    static const char* const          UNREAD[]     = { "1", "x" };
    static const struct d_arg_mapping UNREAD_KEY[] = { { "x", "1" } };

    const struct d_arg_type* const whole =
        d_arg_type_builtin(D_ARG_BUILTIN_INT);
    struct d_arg_type              mute  = LOWER_TYPE;
    struct d_arg_type              list  = LOWER_TYPE;
    struct d_arg_type              blank = LOWER_TYPE;
    const int                      bad   = D_ARG_ERROR_BAD_ARGUMENT;

    mute.convert = NULL;
    list.flags  |= D_ARG_TYPE_FLAG_CONTAINER;
    blank.format = NULL;

    return ( (d_tests_arg_member_unmade(
                  d_arg_validator_is_member(whole,
                                            UNREAD,
                                            2u,
                                            NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_transformer(whole,
                                              UNREAD_KEY,
                                              1u,
                                              NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_is_member(&mute,
                                            MEMBERS,
                                            MEMBER_COUNT,
                                            NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_is_member(&list,
                                            MEMBERS,
                                            MEMBER_COUNT,
                                            NULL),
                  bad))                                    &&
             (d_tests_arg_member_unmade(
                  d_arg_validator_is_member(&blank,
                                            MEMBERS,
                                            MEMBER_COUNT,
                                            NULL),
                  bad)) );
}

/**
 * @brief A filter's context given with no copy function is borrowed.
 *
 * @return whether the set reads the caller's context -- a change to it
 *         changes what the set finds -- and neither the set nor a copy of
 *         it frees the context.
 */
static bool
d_tests_arg_member_borrowed(void)
{
    struct d_tests_arg_member_counts counts = { 0, 0, 0 };
    struct d_tests_arg_member_drop   drop   = { '_', &counts };
    struct d_arg_validator           set    =
        d_tests_arg_member_dropping(&drop,
                                    false);
    const bool                       before =
        d_tests_arg_member_words("F_UNDER",
                                 d_arg_validator_copy(&set),
                                 F_UNDER);

    drop.character = 'a';

    const bool after = d_tests_arg_member_words("F_A",
                                                set,
                                                F_A);

    return ( (before)              &&
             (after)               &&
             (counts.copies == 0)  &&
             (counts.releases == 0) );
}

/**
 * @brief A filter's context given with a copy function is owned.
 *
 * @return whether the set keeps a copy of its own, made when the set is,
 *         so that a change to the caller's changes nothing; a copy of the
 *         set copies it again; and each copy is freed with its set.
 */
static bool
d_tests_arg_member_owned(void)
{
    struct d_tests_arg_member_counts counts = { 0, 0, 8 };
    struct d_tests_arg_member_drop   drop   = { '_', &counts };
    struct d_arg_validator           set    =
        d_tests_arg_member_dropping(&drop,
                                    true);
    const bool                       made   = ( (set.status == D_ARG_OK) &&
                                                (counts.copies == 1) );

    drop.character = 'a';

    const bool copied = d_tests_arg_member_words("F_UNDER",
                                                 d_arg_validator_copy(&set),
                                                 F_UNDER);
    const bool kept   = d_tests_arg_member_words("F_UNDER",
                                                 set,
                                                 F_UNDER);

    return ( (made)                &&
             (copied)              &&
             (kept)                &&
             (counts.copies == 2)  &&
             (counts.releases == 2) );
}

/**
 * @brief A context that cannot be copied, and a filter that cannot filter.
 *
 * @return whether a refused copy fails what needed it, keeping nothing --
 *         the set, when it is made, or a copy of the set -- and a filter's
 *         failure stops the validation with its status.
 */
static bool
d_tests_arg_member_refused(void)
{
    struct d_tests_arg_member_counts counts  = { 0, 0, 0 };
    struct d_tests_arg_member_drop   drop    = { '_', &counts };
    const bool                       unmade  =
        d_tests_arg_member_unmade(d_tests_arg_member_dropping(&drop,
                                                              true),
                                  D_ARG_ERROR_OUT_OF_MEMORY);
    const struct d_arg_filter        refuse  =
    {
        0u,                         // flags
        d_tests_arg_member_refuse,  // function
        NULL,                       // context
        NULL,                       // copy
        NULL                        // release
    };

    counts.allowed = 1;

    struct d_arg_validator set  = d_tests_arg_member_dropping(&drop,
                                                              true);
    struct d_arg_validator copy = d_arg_validator_copy(&set);
    const bool             once = ( (set.status == D_ARG_OK)                  &&
                                    (copy.status ==
                                     D_ARG_ERROR_OUT_OF_MEMORY) );

    d_arg_validator_release(&copy);
    d_arg_validator_release(&set);

    return ( (unmade)                                                      &&
             (once)                                                        &&
             (counts.copies == 1)                                          &&
             (counts.releases == 1)                                        &&
             (d_tests_arg_member_runs_as(
                  d_arg_validator_is_member(NULL,
                                            MEMBERS,
                                            MEMBER_COUNT,
                                            &refuse),
                  "alpha",
                  D_ARG_ERROR_OUT_OF_MEMORY)) );
}

/**
 * @brief What sets list.
 *
 * @return whether a set lists its members as given, in order -- of a type
 *         too, and through a filter -- a set of no members nothing, and a
 *         listing that stops returns the status it stopped with.
 */
static bool
d_tests_arg_member_set_choices(void)
{
    struct d_arg_validator stopped = d_arg_validator_is_member(NULL,
                                                               PICKS,
                                                               2u,
                                                               NULL);
    const bool             stops   =
        (d_arg_validator_each_choice(&stopped,
                                     d_tests_arg_member_stop_at_b,
                                     NULL) == D_ARG_ERROR_HORRIBLE);

    d_arg_validator_release(&stopped);

    return ( (d_tests_arg_member_listed(
                  d_arg_validator_is_member(NULL,
                                            MEMBERS,
                                            MEMBER_COUNT,
                                            &CASE),
                  "alpha|Beta|gamma_ray|"))                      &&
             (d_tests_arg_member_listed(
                  d_arg_validator_is_member(
                      d_arg_type_builtin(D_ARG_BUILTIN_INT),
                      INTEGERS,
                      4u,
                      NULL),
                  "1|2|3|10|"))                                  &&
             (d_tests_arg_member_listed(
                  d_arg_validator_is_member(NULL,
                                            NULL,
                                            0u,
                                            NULL),
                  ""))                                           &&
             (stops) );
}

/**
 * @brief What mappings list.
 *
 * @return whether a mapping lists its keys, a checked mapping its keys then
 *         each value not listed already, and neither completes as a number,
 *         nor does a set of numbers.
 */
static bool
d_tests_arg_member_map_choices(void)
{
    static const struct d_arg_mapping CHAINED[] =
    {
        { "a", "b" },
        { "b", "c" },
        { "d", "c" }
    };

    return ( (d_tests_arg_member_listed(
                  d_arg_validator_transformer(NULL,
                                              PAIRS,
                                              PAIR_COUNT,
                                              NULL),
                  "one|two|Three|"))                             &&
             (d_tests_arg_member_listed(
                  d_arg_validator_checked_transformer(NULL,
                                                      PAIRS,
                                                      PAIR_COUNT,
                                                      NULL),
                  "one|two|Three|1|2|3|"))                       &&
             (d_tests_arg_member_listed(
                  d_arg_validator_checked_transformer(NULL,
                                                      CHAINED,
                                                      3u,
                                                      NULL),
                  "a|b|d|c|"))                                   &&
             (d_tests_arg_kind_of(
                  d_arg_validator_is_member(
                      d_arg_type_builtin(D_ARG_BUILTIN_INT),
                      ONE_TWO,
                      2u,
                      NULL)) == 0u)                              &&
             (d_tests_arg_kind_of(
                  d_arg_validator_checked_transformer(NULL,
                                                      PAIRS,
                                                      PAIR_COUNT,
                                                      NULL)) == 0u) );
}

/**
 * @brief CLI11's `--pick <value>`, the option checked, or transformed, by
 *        IsMember({"a", "b"}, ignore_case).
 *
 * @param[in] _option    the option: checked or transformed.
 * @param[in] _value     the value given.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_member_pick(
    const struct d_tests_arg_checked* _option,
    const char*                       _value,
    const char*                       _expected
)
{
    return d_tests_arg_validates(_option,
                                 d_arg_validator_is_member(NULL,
                                                           PICKS,
                                                           2u,
                                                           &CASE),
                                 _value,
                                 _expected);
}

/**
 * @brief CLI11's `--level <value>`, the option transformed by
 *        CheckedTransformer of LEVELS with ignore_case.
 *
 * @param[in] _value     the value given.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_member_level(
    const char* _value,
    const char* _expected
)
{
    return d_tests_arg_validates(
               &LEVEL_OPTION,
               d_arg_validator_checked_transformer(NULL,
                                                   LEVELS,
                                                   2u,
                                                   &CASE),
               _value,
               _expected);
}

/**
 * @brief CLI11's `--u <value>`, the option checked by IsMember of `int`
 *        {1, 2}.
 *
 * @param[in] _value     the value given.
 * @param[in] _expected  the transcript expected.
 * @return whether the transcript was as expected.
 */
static bool
d_tests_arg_member_whole(
    const char* _value,
    const char* _expected
)
{
    return d_tests_arg_validates(
               &MEMBER_OPTION,
               d_arg_validator_is_member(
                   d_arg_type_builtin(D_ARG_BUILTIN_INT),
                   ONE_TWO,
                   2u,
                   NULL),
               _value,
               _expected);
}

//==============================================================================
// 1.  SETS
//==============================================================================

/*
d_tests_arg_member_text
  CLI11's IsMember of text without a filter, and of no members.
  Tests the following:
  - described as its members between braces, joined by commas, as given
  - a value equal to a member byte for byte passes, as it was given
  - any other value is refused with "<value> not in {<members>}"
  - a set of no members is described as "{}" and refuses every value
*/
bool
d_tests_arg_member_text(void)
{
    const bool text  = d_tests_arg_member_set("M_TEXT",
                                              NULL,
                                              M_TEXT);
    const bool empty = d_tests_arg_member_words(
                           "M_EMPTY",
                           d_arg_validator_is_member(NULL,
                                                     NULL,
                                                     0u,
                                                     NULL),
                           M_EMPTY);

    return ( (text) &&
             (empty) );
}

/*
d_tests_arg_member_filters
  CLI11's IsMember of text with its built-in filters.
  Tests the following:
  - a value and every member are filtered before they are compared: case
    ignored, underscores dropped, spaces and tabs dropped, and two filters
    at once
  - a value found through a filter is replaced by the member, as written
  - a value equal to a member as it is passes as it was given
*/
bool
d_tests_arg_member_filters(void)
{
    const bool lower = d_tests_arg_member_set("M_CASE",
                                              &CASE,
                                              M_CASE);
    const bool under = d_tests_arg_member_set("M_UNDER",
                                              &UNDERSCORE,
                                              M_UNDER);
    const bool both  = d_tests_arg_member_set("M_BOTH",
                                              &CASE_AND_UNDERSCORE,
                                              M_BOTH);
    const bool space = d_tests_arg_member_set("M_SPACE",
                                              &SPACE,
                                              M_SPACE);

    return ( (lower) &&
             (under) &&
             (both)  &&
             (space) );
}

/*
d_tests_arg_member_typed
  CLI11's IsMember of `int` and of `double`.
  Tests the following:
  - members and values compared as numbers: a value finds a member it
    converts to, in any spelling the type reads, and passes as given
  - a leading zero is octal under CLI11's rule, so `010` is no member
  - -0 is the member 0 -- and so no member of {1, 2, 3, 10}
  - a value that does not convert stops the validation, the value itself
    its message, as CLI11 throws it
*/
bool
d_tests_arg_member_typed(void)
{
    const bool whole = d_tests_arg_member_words(
                           "M_INT",
                           d_arg_validator_is_member(
                               d_arg_type_builtin(D_ARG_BUILTIN_INT),
                               INTEGERS,
                               4u,
                               NULL),
                           M_INT);
    const bool real  = d_tests_arg_member_words(
                           "M_DBL",
                           d_arg_validator_is_member(
                               d_arg_type_builtin(D_ARG_BUILTIN_DOUBLE),
                               REALS,
                               3u,
                               NULL),
                           M_DBL);

    return ( (whole) &&
             (real) );
}

/*
d_tests_arg_member_duplicates
  CLI11's IsMember with ignore_case over members one filter makes the same.
  Tests the following:
  - a value equal to a member as it is finds that member
  - one found only through the filter finds the first member the filter
    makes equal to it, which replaces it
*/
bool
d_tests_arg_member_duplicates(void)
{
    const bool lower = d_tests_arg_holds("M_DUPES",
                                         d_arg_validator_is_member(
                                             NULL,
                                             LOWER_FIRST,
                                             3u,
                                             &CASE),
                                         LETTERS,
                                         LETTER_COUNT,
                                         M_DUPES);
    const bool upper = d_tests_arg_holds("M_DUPES2",
                                         d_arg_validator_is_member(
                                             NULL,
                                             UPPER_FIRST,
                                             3u,
                                             &CASE),
                                         LETTERS,
                                         LETTER_COUNT,
                                         M_DUPES2);

    return ( (lower) &&
             (upper) );
}

//==============================================================================
// 2.  MAPPINGS
//==============================================================================

/*
d_tests_arg_member_transformer
  CLI11's Transformer of text, with ignore_case, and keyed by `int`.
  Tests the following:
  - described as "{<from>-><to>,...}"
  - a value matching a key becomes the key's value; any other value passes
    as it was given, one that does not convert to the key type too
  - keys compared as numbers for a key type: `01`, `0x2` and ` 2` find 1
    and 2
*/
bool
d_tests_arg_member_transformer(void)
{
    const bool text  = d_tests_arg_member_words(
                           "X_TEXT",
                           d_arg_validator_transformer(NULL,
                                                       PAIRS,
                                                       PAIR_COUNT,
                                                       NULL),
                           X_TEXT);
    const bool lower = d_tests_arg_member_words(
                           "X_CASE",
                           d_arg_validator_transformer(NULL,
                                                       PAIRS,
                                                       PAIR_COUNT,
                                                       &CASE),
                           X_CASE);
    const bool whole = d_tests_arg_member_words(
                           "X_INT",
                           d_arg_validator_transformer(
                               d_arg_type_builtin(D_ARG_BUILTIN_INT),
                               INTEGER_PAIRS,
                               3u,
                               NULL),
                           X_INT);

    return ( (text)  &&
             (lower) &&
             (whole) );
}

/*
d_tests_arg_member_checked
  CLI11's CheckedTransformer of text, with ignore_case, and keyed by `int`.
  Tests the following:
  - described as "value in {<from>-><to>,...} OR {<to>,...}"
  - a value matching a key becomes the key's value; one equal, as text, to
    a value passes as it is
  - any other value is refused with "Check <value> value in {...} OR {...}
    FAILED", one that does not convert to the key type too
*/
bool
d_tests_arg_member_checked(void)
{
    const bool text  = d_tests_arg_member_words(
                           "C_TEXT",
                           d_arg_validator_checked_transformer(NULL,
                                                               PAIRS,
                                                               PAIR_COUNT,
                                                               NULL),
                           C_TEXT);
    const bool lower = d_tests_arg_member_words(
                           "C_CASE",
                           d_arg_validator_checked_transformer(NULL,
                                                               PAIRS,
                                                               PAIR_COUNT,
                                                               &CASE),
                           C_CASE);
    const bool whole = d_tests_arg_member_words(
                           "C_INT",
                           d_arg_validator_checked_transformer(
                               d_arg_type_builtin(D_ARG_BUILTIN_INT),
                               INTEGER_PAIRS,
                               3u,
                               NULL),
                           C_INT);

    return ( (text)  &&
             (lower) &&
             (whole) );
}

//==============================================================================
// 3.  COMPOSITES
//==============================================================================

/*
d_tests_arg_member_composites
  A set in one of CLI11's composites.
  Tests the following:
  - IsMember of `int` {1, 2} & Range(0, 5): both judge each value, the
    range judging what the set passed, and a value the set cannot convert
    stops the composite
*/
bool
d_tests_arg_member_composites(void)
{
    const int low  = 0;
    const int high = 5;

    return d_tests_arg_member_words(
               "K_AND",
               d_arg_validator_and(
                   d_arg_validator_is_member(
                       d_arg_type_builtin(D_ARG_BUILTIN_INT),
                       ONE_TWO,
                       2u,
                       NULL),
                   d_arg_validator_range(
                       d_arg_type_builtin(D_ARG_BUILTIN_INT),
                       &low,
                       &high)),
               K_AND);
}

//==============================================================================
// 4.  FILTERS OF THE CALLER'S
//==============================================================================

/*
d_tests_arg_member_filter_function
  Filters of the caller's, as CLI11 takes a filter function.
  Tests the following:
  - a function alone filters a value and every member before they are
    compared, and a member found replaces the value
  - given with built-in filters, the function runs after them, as CLI11
    nests ignore_case and a function given after it: dropping uppercase
    letters after case is ignored drops nothing
  - a mapping, checked or not, filters its keys and values alike
*/
bool
d_tests_arg_member_filter_function(void)
{
    const struct d_arg_filter prefix =
    {
        0u,                         // flags
        d_tests_arg_member_prefix,  // function
        NULL,                       // context
        NULL,                       // copy
        NULL                        // release
    };
    const struct d_arg_filter upper  =
    {
        D_ARG_FILTER_IGNORE_CASE,       // flags
        d_tests_arg_member_drop_upper,  // function
        NULL,                           // context
        NULL,                           // copy
        NULL                            // release
    };
    const bool                alone  = d_tests_arg_member_set("F_PREFIX",
                                                              &prefix,
                                                              F_PREFIX);
    const bool                after  = d_tests_arg_member_set("F_ORDER",
                                                              &upper,
                                                              F_ORDER);
    const bool                mapped = d_tests_arg_member_mapped(
                                           "FX_PREFIX",
                                           d_arg_validator_transformer(
                                               NULL,
                                               PAIRS,
                                               PAIR_COUNT,
                                               &prefix),
                                           FX_PREFIX);
    const bool                held   = d_tests_arg_member_mapped(
                                           "FC_PREFIX",
                                           d_arg_validator_checked_transformer(
                                               NULL,
                                               PAIRS,
                                               PAIR_COUNT,
                                               &prefix),
                                           FC_PREFIX);

    return ( (alone)  &&
             (after)  &&
             (mapped) &&
             (held) );
}

/*
d_tests_arg_member_filter_context
  The context a filter's function reads, and who owns it.
  Tests the following:
  - without a copy function the context is borrowed: the set reads the
    caller's, so a change to it is seen, and nothing frees it
  - with one, the set owns a copy, made when the set is, so a change to
    the caller's is not seen; a copy of the set copies it again, and each
    copy is freed with its set
  - a context that cannot be copied fails what needed the copy -- the set,
    D_ARG_ERROR_OUT_OF_MEMORY, or a copy of it -- keeping nothing
  - a filter that cannot filter stops the validation with its status
*/
bool
d_tests_arg_member_filter_context(void)
{
    const bool borrowed = d_tests_arg_member_borrowed();
    const bool owned    = d_tests_arg_member_owned();
    const bool refused  = d_tests_arg_member_refused();

    return ( (borrowed) &&
             (owned)    &&
             (refused) );
}

//==============================================================================
// 5.  TYPES OF THE CALLER'S
//==============================================================================

/*
d_tests_arg_member_caller_type
  IsMember of LOWER, a type of the caller's that is no number and writes its
values, compared as CLI11 compares such a type with its own ==.
  Tests the following:
  - members and values compared by the text the type writes: a value in
    any case finds a member, and passes as it was given
  - a value that does not convert -- the empty text -- stops the
    validation, as CLI11 throws it
  - members described as they were given, which departs from CLI11 where
    they were not given as the type writes them
*/
bool
d_tests_arg_member_caller_type(void)
{
    static const char* const SAME[]  = { "alpha", "beta" };
    static const char* const GIVEN[] = { "Alpha", "BETA" };

    const bool same  = d_tests_arg_member_words(
                           "L_SAME",
                           d_arg_validator_is_member(&LOWER_TYPE,
                                                     SAME,
                                                     2u,
                                                     NULL),
                           L_SAME);
    const bool given = d_tests_arg_member_words(
                           "L_GIVEN",
                           d_arg_validator_is_member(&LOWER_TYPE,
                                                     GIVEN,
                                                     2u,
                                                     NULL),
                           L_GIVEN);

    return ( (same) &&
             (given) );
}

//==============================================================================
// 6.  CHOICES AND CONSTRUCTION
//==============================================================================

/*
d_tests_arg_member_choices
  What the sets and mappings list for shell completion.
  Tests the following:
  - a set lists its members, as given, in order -- of a type too, and
    through a filter, which lists what was given
  - a mapping lists its keys; a checked mapping its keys, then each value
    not listed already, as a key or as an earlier value
  - a set of no members lists nothing
  - a listing stops at the first status other than D_ARG_OK, and returns it
  - none completes as a number or a path: each lists its choices instead
*/
bool
d_tests_arg_member_choices(void)
{
    const bool sets     = d_tests_arg_member_set_choices();
    const bool mappings = d_tests_arg_member_map_choices();

    return ( (sets) &&
             (mappings) );
}

/*
d_tests_arg_member_construction
  Sets and mappings that cannot be made, and the text a set keeps.
  Tests the following:
  - a NULL array with members promised, a NULL member, or a pair with a
    NULL side is a failed validator holding D_ARG_ERROR_BAD_ARGUMENT,
    which runs as that error
  - so is a member or key that does not convert to the type compared as
  - so is a type that cannot be compared as: one without a conversion, a
    container's, or one that is no number and does not write its values
  - a set copies its members' text: the caller's array may change, or go,
    once the set is made
*/
bool
d_tests_arg_member_construction(void)
{
    char                   given[] = "alpha";
    const char* const      held[]  = { given };
    struct d_arg_validator set     = d_arg_validator_is_member(NULL,
                                                               held,
                                                               1u,
                                                               NULL);

    memcpy(given,
           "omega",
           sizeof(given));

    const bool copied = d_tests_arg_member_listed(set,
                                                  "alpha|");

    return ( (d_tests_arg_member_unmade_text())  &&
             (d_tests_arg_member_unmade_types()) &&
             (copied) );
}

//==============================================================================
// 7.  IN A PARSE
//==============================================================================

/*
d_tests_arg_member_in_a_parse
  The sets and mappings on options, as CLI11 adds them.
  Tests the following:
  - a set added by check() passes a value it finds through its filter as
    the value was given, and one added by transform() replaces it with the
    member
  - a value a set refuses fails the parse with ValidationError, its
    message after the option's name
  - a checked mapping turns a key into its value, keeps a value, and
    refuses anything else
  - a set of `int`: a word stops with the word as the message, a number
    not in the set is refused, and one in it reaches the variable; the
    empty value stops with an empty message, which refuses nothing, and
    the variable becomes zero
*/
bool
d_tests_arg_member_in_a_parse(void)
{
    const bool picked      = d_tests_arg_member_pick(&PICK_OPTION,
                                                     "A",
                                                     P_PICK_CHECKED);
    const bool refused     = d_tests_arg_member_pick(&PICK_OPTION,
                                                     "z",
                                                     P_PICK_REFUSED);
    const bool transformed = d_tests_arg_member_pick(&PICK_TRANSFORMED,
                                                     "A",
                                                     P_PICK_TRANSFORMED);
    const bool key         = d_tests_arg_member_level("HIGH",
                                                      P_LEVEL_KEY);
    const bool value       = d_tests_arg_member_level("2",
                                                      P_LEVEL_VALUE);
    const bool other       = d_tests_arg_member_level("3",
                                                      P_LEVEL_REFUSED);
    const bool word        = d_tests_arg_member_whole("abc",
                                                      P_MEMBER_INT_WORD);
    const bool out         = d_tests_arg_member_whole("3",
                                                      P_MEMBER_INT_OUT);
    const bool in          = d_tests_arg_member_whole("2",
                                                      P_MEMBER_INT_IN);
    const bool empty       = d_tests_arg_member_whole("",
                                                      P_MEMBER_INT_EMPTY);

    return ( (picked)      &&
             (refused)     &&
             (transformed) &&
             (key)         &&
             (value)       &&
             (other)       &&
             (word)        &&
             (out)         &&
             (in)          &&
             (empty) );
}

//==============================================================================
// 8.  THE SUITE
//==============================================================================

// d_tests_arg_member
//   constant: the suite, in the header's order.
const struct d_tests_section d_tests_arg_member[] =
{
    { "arg_member_text",
      d_tests_arg_member_text },
    { "arg_member_filters",
      d_tests_arg_member_filters },
    { "arg_member_typed",
      d_tests_arg_member_typed },
    { "arg_member_duplicates",
      d_tests_arg_member_duplicates },
    { "arg_member_transformer",
      d_tests_arg_member_transformer },
    { "arg_member_checked",
      d_tests_arg_member_checked },
    { "arg_member_composites",
      d_tests_arg_member_composites },
    { "arg_member_filter_function",
      d_tests_arg_member_filter_function },
    { "arg_member_filter_context",
      d_tests_arg_member_filter_context },
    { "arg_member_caller_type",
      d_tests_arg_member_caller_type },
    { "arg_member_choices",
      d_tests_arg_member_choices },
    { "arg_member_construction",
      d_tests_arg_member_construction },
    { "arg_member_in_a_parse",
      d_tests_arg_member_in_a_parse }
};

// d_tests_arg_member_count
//   constant: how many sections the table above holds.
const size_t d_tests_arg_member_count =
    sizeof(d_tests_arg_member) / sizeof(d_tests_arg_member[0]);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
