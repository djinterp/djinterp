/*******************************************************************************
* djinterp [parse]                                          arg_parser_tests.cpp
*
* The argument parser's C++ parser suite's sections.
*   The sections held to CLI11 run what the CASES section defines: parsers
* given definitions, parsed in each of the parse's forms and ended as main
* ends them, as a program would with CLI11's App. tools/cppref/cppref.py
* compiles that section against CLI11 2.7.2, through the face's names for
* CLI11's (cli11_shim.hpp), and the transcripts below are what CLI11 wrote;
* so the CASES section is written in the part of the face CLI11 shares --
* every parse through the run, which has a form for each of the parser's,
* `->` on every handle, no status taken from a call -- and C++98 unless a
* guard says otherwise. The sections that are the module's own are written
* apart, after it, each saying why CLI11 cannot write its transcript; the
* zero-overhead checks are assertions the build itself makes, beside the
* lines a run writes.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_parser_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_parser_tests.hpp"  // corresponding header
// std
#include <cstddef>      // std::size_t
#include <sstream>      // std::ostringstream, std::istringstream
#include <string>       // std::string, std::wstring
#include <vector>       // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#include <type_traits>  // std::is_trivially_copyable, std::is_standard_layout
#endif
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
using cli::testing::number;
using cli::testing::scratch_directory;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// EXITS
//   constant: what the steps of d_tests_arg_cpp_parser_exits write, as CLI11
// writes them.
const char EXITS[] =
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--help]\n"
    "exit 0\n"
    "out d\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n  -n,     --number "
    "           a number\\n          --help-all          everything\\n         "
    " --version           Display program version information and "
    "exit\\n\\nSUBCOMMANDS:\\n  sub                         a subcommand\\n\n"
    "ERR CallForAllHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--help-all]\n"
    "exit 0\n"
    "out d\\n\\n\\nprog [OPTIONS] [SUBCOMMANDS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n  -n,     --number "
    "           a number\\n          --help-all          everything\\n         "
    " --version           Display program version information and "
    "exit\\n\\nSUBCOMMANDS:\\nsub\\na subcommand\\n\\n\\nOPTIONS:\\n          "
    "--deep              deep down\\n\\n\n"
    "ERR CallForVersion 0: 2.0\n"
    "cmd app n=1 miss=[] subs=[] order=[--version]\n"
    "exit 0\n"
    "out 2.0\\n\n"
    "ERR RequiredError 106: --needed is required\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "opt count n=0 res=[]\n"
    "opt other n=0 res=[]\n"
    "opt needed n=0 res=[]\n"
    "exit 106\n"
    "err --needed is required\\nRun with --help for more information.\\n\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[--needed]\n"
    "opt count n=0 res=[]\n"
    "opt other n=0 res=[]\n"
    "opt needed n=1 res=[1]\n"
    "exit 109\n"
    "err prog: The following argument was not expected: --nope\\nRun with "
    "--help for more information.\\n\n"
    "ERR ConversionError 104: Could not convert: --count = x\n"
    "cmd app n=1 miss=[] subs=[] order=[--needed|--count]\n"
    "opt count n=1 res=[x]\n"
    "opt other n=0 res=[]\n"
    "opt needed n=1 res=[1]\n"
    "exit 104\n"
    "err Could not convert: --count = x\\nRun with --help for more "
    "information.\\n\n"
    "ERR ExcludesError 108: --count excludes --other\n"
    "cmd app n=1 miss=[] subs=[] order=[--needed|--count|--other]\n"
    "opt count n=1 res=[2]\n"
    "opt other n=1 res=[3]\n"
    "opt needed n=1 res=[1]\n"
    "exit 108\n"
    "err --count excludes --other\\nRun with --help for more information.\\n\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[] subs=[] order=[--help]\n"
    "exit 0\n"
    "out a subcommand\\n\\n\\nprog sub [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n          --deep   "
    "           deep down\\n\n"
    "ERR ExtrasError 109: sub: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[--nope] subs=[] order=[]\n"
    "exit 109\n"
    "err sub: The following argument was not expected: --nope\\nRun with "
    "--help for more information.\\n\n"
    "ERR ExtrasError 109: sub: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[] subs=[sub] order=[]\n"
    "cmd sub n=1 miss=[--nope] subs=[] order=[]\n"
    "exit 109\n"
    "err ERROR: ExtrasError: sub: The following argument was not expected: "
    "--nope\\na subcommand\\n\\n\\nprog sub [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h, "
    "    --help              Print this help message and exit\\n          "
    "--deep              deep down\\n\n"
    "cmd app n=1 miss=[] subs=[] order=[-v]\n"
    "opt verbose n=1 res=[true]\n"
    "exit 0\n";

// MACRO
//   constant: what the steps of d_tests_arg_cpp_parser_macro write, as
// CLI11's CLI11_PARSE writes them.
const char MACRO[] =
    "cmd app n=1 miss=[] subs=[] order=[-v]\n"
    "main -1\n"
    "ERR CallForHelp 0: This should be caught in your main function, see "
    "examples\n"
    "cmd app n=1 miss=[] subs=[] order=[--help]\n"
    "main 0\n"
    "out d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help          "
    "    Print this help message and exit\\n  -v                          a "
    "flag\\n  -V                          Display program version information "
    "and exit\\n\n"
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[]\n"
    "main 109\n"
    "err prog: The following argument was not expected: --nope\\nRun with "
    "--help for more information.\\n\n"
    "ERR CallForVersion 0: 3.1\n"
    "cmd app n=1 miss=[] subs=[] order=[-V]\n"
    "main 0\n"
    "out 3.1\\n\n";

// FORMS
//   constant: what the steps of d_tests_arg_cpp_parser_forms write, as CLI11
// writes them.
const char FORMS[] =
    "cmd app n=1 miss=[] subs=[] order=[--number|-v]\n"
    "opt number n=1 res=[1]\n"
    "opt text n=0 res=[]\n"
    "opt flag n=1 res=[true]\n"
    "cmd app n=1 miss=[] subs=[] order=[--text|--number]\n"
    "opt number n=1 res=[2]\n"
    "opt text n=1 res=[a b]\n"
    "opt flag n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[--text|-v]\n"
    "opt number n=0 res=[]\n"
    "opt text n=1 res=[x y]\n"
    "opt flag n=1 res=[true]\n"
    "cmd app n=1 miss=[] subs=[] order=[--text]\n"
    "opt number n=0 res=[]\n"
    "opt text n=1 res=[a\\x00b]\n"
    "opt flag n=0 res=[]\n"
    "cmd app n=1 miss=[first|third] subs=[] order=[-n]\n"
    "opt number n=1 res=[5]\n"
    "left: [third,first]\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// FORMS_MOVED
//   constant: what the steps of d_tests_arg_cpp_parser_forms_moved write, as
// CLI11 writes them.
const char FORMS_MOVED[] =
    "cmd app n=1 miss=[] subs=[] order=[-n]\n"
    "opt number n=1 res=[6]\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// FORMS_STREAM
//   constant: what the steps of d_tests_arg_cpp_parser_forms_stream write, as
// CLI11 writes them.
const char FORMS_STREAM[] =
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt number n=1 res=[5]\n"
    "opt flag n=1 res=[true]\n"
    "opt deep n=1 res=[1]\n";

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// FORMS_WIDE
//   constant: what the steps of d_tests_arg_cpp_parser_forms_wide write, as
// CLI11 writes them.
const char FORMS_WIDE[] =
    "cmd app n=1 miss=[] subs=[] order=[--text|-n]\n"
    "opt text n=1 res=[\xc3\xa9t\xc3\xa9]\n"
    "opt number n=1 res=[3]\n"
    "cmd app n=1 miss=[] subs=[] order=[--text|-n]\n"
    "opt text n=1 res=[w x]\n"
    "opt number n=1 res=[4]\n"
    "cmd app n=1 miss=[] subs=[] order=[-n]\n"
    "opt text n=0 res=[]\n"
    "opt number n=1 res=[5]\n"
    "named: [prog]\n";

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// NAMES
//   constant: what the steps of d_tests_arg_cpp_parser_names write, as CLI11
// writes them.
const char NAMES[] =
    "unnamed: []\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "from main: [prog]\n"
    "cmd app n=1 miss=[] subs=[] order=[-x]\n"
    "from a line: [tool]\n"
    "cmd app n=1 miss=[] subs=[] order=[-x]\n"
    "from quotes: [quoted tool]\n"
    "cmd app n=1 miss=[] subs=[] order=[-x]\n"
    "set: [fixed]\n"
    "cmd app n=1 miss=[] subs=[] order=[]\n"
    "kept: [fixed]\n";

// STATE
//   constant: what the steps of d_tests_arg_cpp_parser_state write, as CLI11
// writes them.
const char STATE[] =
    "cmd app n=1 miss=[] subs=[sub] order=[--n]\n"
    "cmd sub n=1 miss=[left] subs=[] order=[]\n"
    "opt n n=1 res=[1]\n"
    "cmd app n=0 miss=[] subs=[] order=[]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt n n=0 res=[]\n"
    "cmd app n=1 miss=[] subs=[] order=[--n]\n"
    "cmd sub n=0 miss=[] subs=[] order=[]\n"
    "opt n n=1 res=[2]\n";

// ERRORS
//   constant: what the steps of d_tests_arg_cpp_parser_errors write: the
// module's own, as CLI11 reports a failure by throwing it, where the face
// keeps it in the parser and gives views of it (ISSUES 49), and keeps a
// definition that failed for the next parse to report.
const char ERRORS[] =
    "made: status=OK ok=1 exit=0 what=[]\n"
    "failed: status=ExtrasError ok=0 exit=109 what=[prog: The following "
    "argument was not expected: --nope]\n"
    "the view after the next parse: status=OK ok=1 exit=0 what=[]\n"
    "definition: status=OptionAlreadyAdded ok=0 exit=102 what=[cannot add "
    "'--n': long name '--n' already used by '--n']\n"
    "reported: status=OptionAlreadyAdded ok=0 exit=102 what=[cannot add '--n': "
    "long name '--n' already used by '--n']\n"
    "cleared: status=OK ok=1 exit=0 what=[]\n"
    "parsed: status=OK ok=1 exit=0 what=[]\n"
    "forgotten: status=OK ok=1 exit=0 what=[]\n";

// OUTPUT
//   constant: what the steps of d_tests_arg_cpp_parser_output write: the
// module's own, as CLI11's exit() writes to the streams given it or to the
// standard ones, where the face's parser has an output and an error output
// of its own, sinks or streams, which exit() writes to unless given others.
const char OUTPUT[] =
    "strings: codes=109,0 out=[output\\n\\n\\ntool "
    "[OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     --help              Print this "
    "help message and exit\\n          --n                 a number\\n         "
    " --version           Display program version information and exit\\n] "
    "err=[tool: The following argument was not expected: --nope\\nRun with "
    "--help for more information.\\n]\n"
    "streams: codes=109,0,109 out=[] err=[tool: The following argument was not "
    "expected: --nope\\nRun with --help for more information.\\n] given=[This "
    "should be caught in your main function, see examples\\n|tool: The "
    "following argument was not expected: --nope\\nRun with --help for more "
    "information.\\n]\n";

// STATUSES
//   constant: what the steps of d_tests_arg_cpp_parser_statuses write: the
// module's own, as CLI11's exit() takes an Error, where the face's takes a
// status too, one that need not be the last parse's.
const char STATUSES[] =
    "ok: status=OK exit=0 out=[] err=[]\n"
    "success: status=Success exit=0 out=[] err=[]\n"
    "runtime: status=RuntimeError exit=1 out=[] err=[]\n"
    "validation: status=ValidationError exit=105 out=[] err=[Validation "
    "error\\nRun with --help for more information.\\n]\n"
    "none: status=Error exit=127 out=[] err=[Error\\nRun with --help for more "
    "information.\\n]\n"
    "the last parse's: status=ExtrasError exit=109 out=[] err=[prog: The "
    "following argument was not expected: --nope\\nRun with --help for more "
    "information.\\n]\n"
    "another: status=RequiredError exit=106 out=[] err=[Required error\\nRun "
    "with --help for more information.\\n]\n";

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_FILES
//   constant: what the steps of d_tests_arg_cpp_parser_response_files write:
// the module's own, as CLI11 reads no response files.
const char RESPONSE_FILES[] =
    "made: enabled=0 native=1\n"
    "set: enabled=1 posix=1\n"
    "cmd app n=1 miss=[] subs=[] order=[--n|-v|rest]\n"
    "opt n n=1 res=[5]\n"
    "opt verbose n=1 res=[true]\n"
    "opt rest n=1 res=[a b]\n"
    "cmd app n=1 miss=[] subs=[] order=[rest]\n"
    "opt n n=0 res=[]\n"
    "opt verbose n=0 res=[]\n"
    "opt rest n=1 res=[@args.rsp]\n"
    "files: made\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

// SUGGESTIONS
//   constant: what the steps of d_tests_arg_cpp_parser_suggestions write: the
// module's own, as CLI11 suggests nothing for a name mistyped.
const char SUGGESTIONS[] =
    "off: 0\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--numbr 1\n"
    "cmd app n=1 miss=[--numbr|1] subs=[] order=[]\n"
    "on: 1\n"
    "ERR ExtrasError 109: prog: The following arguments were not expected: "
    "--numbr 1\\nDid you mean '--number' for '--numbr'?\n"
    "cmd app n=1 miss=[--numbr|1] subs=[] order=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

// COMPLETION_FLAG
//   constant: what the steps of d_tests_arg_cpp_parser_completion_flag write:
// the module's own, as CLI11 writes no completion scripts.
const char COMPLETION_FLAG[] =
    "made: valid=0 name=[] description=[]\n"
    "set: valid=1 name=[--completion] description=[write a completion script]\n"
    "asked: status=CallForCompletion exit=0 first line=[# bash completion for "
    "prog, generated by djinterp's arg_parser.] the message=1 err=[]\n"
    "removed: valid=0 name=[] description=[]\n";

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

// LAYOUT
//   constant: what the steps of d_tests_arg_cpp_parser_layout write: the
// module's own -- the face's records read as the C records they are.
const char LAYOUT[] =
    "parser: c=1 root=1 sub=1 option=1 defaults=1 validator=1 text=1 span=1\n";

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// TRAITS
//   constant: what the steps of d_tests_arg_cpp_parser_traits write: the
// module's own -- what the standard's traits tell of the face's types.
const char TRAITS[] =
    "trivially copyable: command=1 option=1 error=1 status=1 text=1 lists=1 "
    "standard layout: parser=1 copyable parser=0\n";

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

// DEPARTED
//   constant: what the steps of d_tests_arg_cpp_parser_departures write: the
// module's own, where the face's parse forms depart from CLI11's (ISSUES
// 58): a vector whose parse failed is left as it was given, where CLI11's
// is left consumed as far as the parse got.
const char DEPARTED[] =
    "ERR ExtrasError 109: prog: The following argument was not expected: "
    "--nope\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[-n]\n"
    "opt number n=1 res=[x]\n"
    "failed, left: [x,-n,--nope]\n"
    "cmd app n=1 miss=[--nope] subs=[] order=[-n]\n"
    "opt number n=1 res=[x]\n"
    "went on, left: [--nope]\n";

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

/**
 * @brief A list of texts as one, escaped, separated by commas.
 *
 * @param[in] _items  the texts.
 * @return the text.
 */
std::string
joined(
    const std::vector<std::string>& _items
)
{
    std::string out;

    // each item, after a comma when one comes before it
    for (std::size_t index = 0u; index < _items.size(); ++index)
    {
        out += (index > 0u) ? "," : "";
        out += escape(_items[index]);
    }

    return out;
}

/**
 * @brief Writes the root's name, as a parse leaves it, on one line.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line is.
 */
void
write_name(
    run&        _run,
    const char* _label
)
{
    _run.note(std::string(_label) +
              ": [" +
              escape(std::string(_run.root()->get_name())) +
              "]");

    return;
}

// 1.1    Endings
//------------------------------------------------------------------------------

/**
 * @brief EXITS 1: help, help of everything and a version asked for, each
 *        ended as main ends it.
 *
 * @param[in,out] _run  the step's run.
 */
void
exits_requests(
    run& _run
)
{
    static const char* const HELP[]    = { "-h" };
    static const char* const ALL[]     = { "--help-all" };
    static const char* const VERSION[] = { "--version" };
    cli::command             sub       = _run.app().add_subcommand(
                                             "sub",
                                             "a subcommand");

    (void)_run.app().add_option("-n,--number",
                                "a number");
    (void)sub->add_flag("--deep",
                        "deep down");
    _run.app().set_help_all_flag("--help-all",
                                 "everything");
    _run.app().set_version_flag("--version",
                                "2.0");
    _run.parse(HELP,
               1u);
    _run.exit();
    _run.parse(ALL,
               1u);
    _run.exit();
    _run.parse(VERSION,
               1u);
    _run.exit();

    return;
}

/**
 * @brief EXITS 2: a required option left out, an argument not expected, a
 *        value that does not convert and options that exclude each other,
 *        each ended as main ends it.
 *
 * @param[in,out] _run  the step's run.
 */
void
exits_failures(
    run& _run
)
{
    static const char* const EXTRA[]    = { "--needed", "1", "--nope" };
    static const char* const CONVERT[]  = { "--needed", "1", "--count", "x" };
    static const char* const EXCLUDED[] =
    {
        "--needed", "1", "--count", "2", "--other", "3"
    };
    int                      whole      = 0;
    cli::option              count      = _run.watch(
                                              "count",
                                              _run.app().add_option(
                                                  "--count",
                                                  whole));
    cli::option              other      = _run.make_option("other",
                                                           _run.root(),
                                                           "--other");

    (void)_run.watch("needed",
                     _run.app().add_option("--needed")->required());
    count->excludes(other);
    _run.parse(D_NULLPTR,
               0u);
    _run.exit();
    _run.parse(EXTRA,
               3u);
    _run.exit();
    _run.parse(CONVERT,
               4u);
    _run.exit();
    _run.parse(EXCLUDED,
               6u);
    _run.exit();

    return;
}

/**
 * @brief EXITS 3: help asked for in a subcommand, and a failure there,
 *        under CLI11's simple message and its help message.
 *
 * @param[in,out] _run  the step's run.
 */
void
exits_subcommand(
    run& _run
)
{
    static const char* const HELP[] = { "sub", "-h" };
    static const char* const BAD[]  = { "sub", "--nope" };
    cli::command             sub    = _run.make_subcommand(_run.root(),
                                                           "sub");

    sub->description("a subcommand");
    (void)sub->add_option("--deep",
                          "deep down");
    _run.parse(HELP,
               2u);
    _run.exit();
    _run.parse(BAD,
               2u);
    _run.exit();
    _run.app().failure_message(cli::failure_message::help);
    _run.parse(BAD,
               2u);
    _run.exit();

    return;
}

/**
 * @brief EXITS 4: a parse that goes on, ended as main ends it.
 *
 * @param[in,out] _run  the step's run.
 */
void
exits_success(
    run& _run
)
{
    static const char* const GIVEN[] = { "-v" };

    (void)_run.make_flag("verbose",
                         _run.root(),
                         "-v");
    _run.parse(GIVEN,
               1u);
    _run.exit();

    return;
}

// EXITS_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_exits.
const step_fn EXITS_STEPS[] =
{
    &exits_requests,
    &exits_failures,
    &exits_subcommand,
    &exits_success
};

// EXITS_COUNT
//   constant: how many steps EXITS_STEPS holds.
const std::size_t EXITS_COUNT = sizeof(EXITS_STEPS) / sizeof(EXITS_STEPS[0]);

/**
 * @brief MACRO 1: main through D_ARG_PARSE -- CLI11_PARSE -- going on,
 *        asked for help and a version, and failing.
 *
 * @param[in,out] _run  the step's run.
 */
void
macro_endings(
    run& _run
)
{
    static const char* const HELP[]    = { "-h" };
    static const char* const BAD[]     = { "--nope" };
    static const char* const VERSION[] = { "-V" };
    static const char* const GIVEN[]   = { "-v" };

    (void)_run.app().add_flag("-v",
                              "a flag");
    _run.app().set_version_flag("-V",
                                "3.1");
    _run.parse_main(GIVEN,
                    1u);
    _run.parse_main(HELP,
                    1u);
    _run.parse_main(BAD,
                    1u);
    _run.parse_main(VERSION,
                    1u);

    return;
}

// MACRO_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_macro.
const step_fn MACRO_STEPS[] =
{
    &macro_endings
};

// MACRO_COUNT
//   constant: how many steps MACRO_STEPS holds.
const std::size_t MACRO_COUNT = sizeof(MACRO_STEPS) / sizeof(MACRO_STEPS[0]);

// 1.2    Parses
//------------------------------------------------------------------------------

/**
 * @brief FORMS 1: command lines held in one string: without the program's
 *        name and with it, quoted values, `=`, spaces around, and a NUL
 *        byte kept in its word.
 *
 * @param[in,out] _run  the step's run.
 */
void
forms_lines(
    run& _run
)
{
    (void)_run.make_option("number",
                           _run.root(),
                           "-n,--number");
    (void)_run.make_option("text",
                           _run.root(),
                           "--text");
    (void)_run.make_flag("flag",
                         _run.root(),
                         "-v");
    _run.parse_line("-n 1 -v",
                    false);
    _run.parse_line("prog --text 'a b' --number=2",
                    true);
    _run.parse_line("  --text \"x y\"   -v  ",
                    false);
    _run.parse_line(std::string("--text a\0b",
                                10u),
                    false);

    return;
}

/**
 * @brief FORMS 2: arguments given last first, and the arguments the root
 *        did not use, which the vector holds after.
 *
 * @param[in,out] _run  the step's run.
 */
void
forms_vector(
    run& _run
)
{
    std::vector<std::string> args;

    (void)_run.make_option("number",
                           _run.root(),
                           "-n");
    _run.app().allow_extras();
    args.push_back("third");
    args.push_back("5");
    args.push_back("-n");
    args.push_back("first");
    _run.parse_vector(args);
    _run.note("left: [" + joined(args) + "]");

    return;
}

// FORMS_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_forms.
const step_fn FORMS_STEPS[] =
{
    &forms_lines,
    &forms_vector
};

// FORMS_COUNT
//   constant: how many steps FORMS_STEPS holds.
const std::size_t FORMS_COUNT = sizeof(FORMS_STEPS) / sizeof(FORMS_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief FORMS_MOVED 1: arguments given last first to the overload that
 *        consumes them.
 *
 * @param[in,out] _run  the step's run.
 */
void
forms_moved(
    run& _run
)
{
    std::vector<std::string> args;

    (void)_run.make_option("number",
                           _run.root(),
                           "-n");
    args.push_back("6");
    args.push_back("-n");
    _run.parse_moved(args);

    return;
}

// FORMS_MOVED_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_forms_moved.
const step_fn FORMS_MOVED_STEPS[] =
{
    &forms_moved
};

// FORMS_MOVED_COUNT
//   constant: how many steps FORMS_MOVED_STEPS holds.
const std::size_t FORMS_MOVED_COUNT =
    sizeof(FORMS_MOVED_STEPS) / sizeof(FORMS_MOVED_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/**
 * @brief FORMS_STREAM 1: configuration text read from a stream, as a
 *        configuration file would give it, a subcommand's section among it.
 *
 * @param[in,out] _run  the step's run.
 */
void
forms_stream(
    run& _run
)
{
    std::istringstream input("number = 5\nflag = true\n[sub]\ndeep = 1\n");
    cli::command       sub = _run.make_subcommand(_run.root(),
                                                  "sub");

    (void)_run.make_option("number",
                           _run.root(),
                           "--number");
    (void)_run.make_flag("flag",
                         _run.root(),
                         "--flag");
    (void)_run.make_option("deep",
                           sub,
                           "--deep");
    _run.parse_stream(input);

    return;
}

// FORMS_STREAM_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_forms_stream.
const step_fn FORMS_STREAM_STEPS[] =
{
    &forms_stream
};

// FORMS_STREAM_COUNT
//   constant: how many steps FORMS_STREAM_STEPS holds.
const std::size_t FORMS_STREAM_COUNT =
    sizeof(FORMS_STREAM_STEPS) / sizeof(FORMS_STREAM_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/**
 * @brief FORMS_WIDE 1: wide arguments, as wmain's, a value beyond ASCII
 *        among them; wide lines without the program's name and with it.
 *
 * @param[in,out] _run  the step's run.
 */
void
forms_wide(
    run& _run
)
{
    static const wchar_t* const GIVEN[] =
    {
        L"--text", L"\x00e9t\x00e9", L"-n", L"3"
    };

    (void)_run.make_option("text",
                           _run.root(),
                           "--text");
    (void)_run.make_option("number",
                           _run.root(),
                           "-n");
    _run.parse_wide(GIVEN,
                    4u);
    _run.parse_wide_line(std::wstring(L"--text \"w x\" -n 4"),
                         false);
    _run.parse_wide_line(std::wstring(L"tool -n 5"),
                         true);
    write_name(_run,
               "named");

    return;
}

// FORMS_WIDE_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_forms_wide.
const step_fn FORMS_WIDE_STEPS[] =
{
    &forms_wide
};

// FORMS_WIDE_COUNT
//   constant: how many steps FORMS_WIDE_STEPS holds.
const std::size_t FORMS_WIDE_COUNT =
    sizeof(FORMS_WIDE_STEPS) / sizeof(FORMS_WIDE_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

/**
 * @brief NAMES 1: a root without a name, named by a parse of main's
 *        arguments and by lines naming the program, quoted and not; then a
 *        name set, which parses keep.
 *
 * @param[in,out] _run  the step's run.
 */
void
names_given(
    run& _run
)
{
    (void)_run.app().add_flag("-x",
                              "a flag");
    _run.app().name("");
    write_name(_run,
               "unnamed");
    _run.parse(D_NULLPTR,
               0u);
    write_name(_run,
               "from main");
    _run.parse_line("tool -x",
                    true);
    write_name(_run,
               "from a line");
    _run.parse_line("'quoted tool' -x",
                    true);
    write_name(_run,
               "from quotes");
    _run.app().name("fixed");
    _run.parse_line("other -x",
                    true);
    write_name(_run,
               "set");
    _run.parse(D_NULLPTR,
               0u);
    write_name(_run,
               "kept");

    return;
}

// NAMES_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_names.
const step_fn NAMES_STEPS[] =
{
    &names_given
};

// NAMES_COUNT
//   constant: how many steps NAMES_STEPS holds.
const std::size_t NAMES_COUNT = sizeof(NAMES_STEPS) / sizeof(NAMES_STEPS[0]);

/**
 * @brief STATE 1: a parse forgotten by clear(), and a parse after one that
 *        used a subcommand and left arguments.
 *
 * @param[in,out] _run  the step's run.
 */
void
state_forgotten(
    run& _run
)
{
    static const char* const FIRST[]  = { "--n", "1", "sub", "left" };
    static const char* const SECOND[] = { "--n", "2" };
    cli::command             sub      = _run.make_subcommand(_run.root(),
                                                             "sub");

    (void)_run.make_option("n",
                           _run.root(),
                           "--n");
    sub->allow_extras();
    _run.parse(FIRST,
               4u);
    _run.app().clear();
    _run.report();
    _run.parse(SECOND,
               2u);

    return;
}

// STATE_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_state.
const step_fn STATE_STEPS[] =
{
    &state_forgotten
};

// STATE_COUNT
//   constant: how many steps STATE_STEPS holds.
const std::size_t STATE_COUNT = sizeof(STATE_STEPS) / sizeof(STATE_STEPS[0]);

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: errors as views, the parser's output, statuses
// ended that no parse gave, response files, suggestions, the completion
// flag, the records' layout and traits, and the departures. Each is a step
// written here, through the whole face.

namespace
{

/**
 * @brief Writes an error view on one line: its status's name, its exit
 *        code and its message.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the view by.
 * @param[in]     _error  the view.
 */
void
write_error(
    run&              _run,
    const char*       _label,
    const cli::error& _error
)
{
    std::ostringstream line;

    line << _label
         << ": status=" << _error.get_name()
         << " ok=" << _error.get_status().ok()
         << " exit=" << _error.get_exit_code()
         << " what=[" << escape(std::string(_error.what()))
         << "]";
    _run.note(line.str());

    return;
}

/**
 * @brief ERRORS 1: the views of what parses ended with: none, a failure,
 *        the same view after the next parse, a definition that failed and
 *        the parses reporting it until it is cleared, and a parse forgotten.
 *
 * @param[in,out] _run  the step's run.
 */
void
errors_views(
    run& _run
)
{
    static const char* const GOOD[] = { "--n", "1" };
    static const char* const BAD[]  = { "--nope" };

    (void)_run.app().add_option("--n");
    write_error(_run,
                "made",
                _run.app().get_error());
    (void)_run.parse_quiet(BAD,
                           1u);

    const cli::error failed = _run.app().get_error();

    write_error(_run,
                "failed",
                failed);
    (void)_run.parse_quiet(GOOD,
                           2u);
    write_error(_run,
                "the view after the next parse",
                failed);
    (void)_run.app().add_option("--n");
    write_error(_run,
                "definition",
                _run.app().get_definition_error());
    (void)_run.parse_quiet(GOOD,
                           2u);
    write_error(_run,
                "reported",
                _run.app().get_error());
    _run.app().clear_definition_error();
    write_error(_run,
                "cleared",
                _run.app().get_definition_error());
    (void)_run.parse_quiet(GOOD,
                           2u);
    write_error(_run,
                "parsed",
                _run.app().get_error());
    _run.app().clear();
    write_error(_run,
                "forgotten",
                _run.app().get_error());

    return;
}

// ERRORS_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_errors.
const step_fn ERRORS_STEPS[] =
{
    &errors_views
};

// ERRORS_COUNT
//   constant: how many steps ERRORS_STEPS holds.
const std::size_t ERRORS_COUNT =
    sizeof(ERRORS_STEPS) / sizeof(ERRORS_STEPS[0]);

/**
 * @brief OUTPUT 1: a parser's output and error output as strings, then as
 *        streams; exit() of a status and of an error view, to them and to
 *        streams given it.
 *
 * @param[in,out] _run  the step's run.
 */
void
output_ends(
    run& _run
)
{
    static const char* const BAD[] = { "tool", "--nope" };
    std::string              out;
    std::string              err;
    std::ostringstream       outs;
    std::ostringstream       errs;
    std::ostringstream       given_out;
    std::ostringstream       given_err;
    cli::arg_parser          app("output",
                                 "tool");

    (void)app.add_option("--n",
                         "a number");
    app.set_version_flag("--version",
                         "7.7");
    app.output(cli::string_sink(out),
               cli::string_sink(err));

    const cli::status failed  = app.parse(2,
                                          BAD);
    const int         first   = app.exit(failed);
    const int         helped  = app.exit(cli::status_code::call_for_help);

    _run.note("strings: codes=" + number(static_cast<long>(first)) + "," +
              number(static_cast<long>(helped)) +
              " out=[" + escape(out) + "] err=[" + escape(err) + "]");
    app.output(outs,
               errs);

    const int again   = app.exit(app.get_error());
    const int version = app.exit(cli::status_code::call_for_version,
                                 given_out);
    const int both    = app.exit(app.get_error(),
                                 given_out,
                                 given_err);

    _run.note("streams: codes=" + number(static_cast<long>(again)) + "," +
              number(static_cast<long>(version)) + "," +
              number(static_cast<long>(both)) +
              " out=[" + escape(outs.str()) + "] err=[" +
              escape(errs.str()) + "] given=[" + escape(given_out.str()) +
              "|" + escape(given_err.str()) + "]");

    return;
}

// OUTPUT_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_output.
const step_fn OUTPUT_STEPS[] =
{
    &output_ends
};

// OUTPUT_COUNT
//   constant: how many steps OUTPUT_STEPS holds.
const std::size_t OUTPUT_COUNT =
    sizeof(OUTPUT_STEPS) / sizeof(OUTPUT_STEPS[0]);

/**
 * @brief Ends a parser with a status, into streams of the step's own, and
 *        writes what came of it on one line.
 *
 * @param[in,out] _run     the run.
 * @param[in]     _label   what the line is.
 * @param[in]     _status  the status ended with.
 */
void
write_ended(
    run&               _run,
    const char*        _label,
    const cli::status& _status
)
{
    std::ostringstream out;
    std::ostringstream err;
    const int          code = _run.app().exit(_status,
                                              out,
                                              err);

    _run.note(std::string(_label) +
              ": status=" + _status.get_name() +
              " exit=" + number(static_cast<long>(code)) +
              " out=[" + escape(out.str()) +
              "] err=[" + escape(err.str()) + "]");

    return;
}

/**
 * @brief STATUSES 1: statuses ended that are not the last parse's: ok, a
 *        success, a runtime error, an error and a code that is no status,
 *        after a parse that went on; then the last parse's own.
 *
 * @param[in,out] _run  the step's run.
 */
void
statuses_ended(
    run& _run
)
{
    static const char* const BAD[] = { "--nope" };

    (void)_run.parse_quiet(D_NULLPTR,
                           0u);
    write_ended(_run,
                "ok",
                cli::status());
    write_ended(_run,
                "success",
                cli::status_code::success);
    write_ended(_run,
                "runtime",
                cli::status_code::runtime_error);
    write_ended(_run,
                "validation",
                cli::status_code::validation_error);
    write_ended(_run,
                "none",
                cli::status(9999));
    (void)_run.parse_quiet(BAD,
                           1u);
    write_ended(_run,
                "the last parse's",
                cli::status_code::extras_error);
    write_ended(_run,
                "another",
                cli::status_code::required_error);

    return;
}

// STATUSES_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_statuses.
const step_fn STATUSES_STEPS[] =
{
    &statuses_ended
};

// STATUSES_COUNT
//   constant: how many steps STATUSES_STEPS holds.
const std::size_t STATUSES_COUNT =
    sizeof(STATUSES_STEPS) / sizeof(STATUSES_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE_FILE
//   constant: the response file the response-file step reads.
const file_entry RESPONSE_FILE[] =
{
    { "args.rsp", "--n 5 -v 'a b'\n" },
    { D_NULLPTR, D_NULLPTR }
};

/**
 * @brief RESPONSE_FILES 1: response files off at first, then read by POSIX
 *        rules, then off again.
 *
 * @param[in,out] _run  the step's run.
 */
void
response_files_read(
    run& _run
)
{
    static const char* const GIVEN[] = { "@args.rsp" };
    scratch_directory        files(RESPONSE_FILE);

    (void)_run.make_option("n",
                           _run.root(),
                           "--n");
    (void)_run.make_flag("verbose",
                         _run.root(),
                         "-v");
    (void)_run.make_option("rest",
                           _run.root(),
                           "rest");
    _run.note(std::string("made: enabled=") +
              (_run.app().get_response_files() ? "1" : "0") +
              " native=" +
              ( (_run.app().get_response_quoting() ==
                 cli::quoting::native_response) ? "1" : "0" ));
    _run.app().response_files(true,
                              cli::quoting::posix);
    _run.note(std::string("set: enabled=") +
              (_run.app().get_response_files() ? "1" : "0") +
              " posix=" +
              ( (_run.app().get_response_quoting() ==
                 cli::quoting::posix) ? "1" : "0" ));
    _run.parse(GIVEN,
               1u);
    _run.app().response_files(false);
    _run.parse(GIVEN,
               1u);
    _run.note(std::string("files: ") + (files.ok() ? "made" : "failed"));

    return;
}

// RESPONSE_FILES_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_response_files.
const step_fn RESPONSE_FILES_STEPS[] =
{
    &response_files_read
};

// RESPONSE_FILES_COUNT
//   constant: how many steps RESPONSE_FILES_STEPS holds.
const std::size_t RESPONSE_FILES_COUNT =
    sizeof(RESPONSE_FILES_STEPS) / sizeof(RESPONSE_FILES_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

/**
 * @brief SUGGESTIONS 1: a name mistyped, without suggestions -- as the run
 *        makes its parser -- and with them.
 *
 * @param[in,out] _run  the step's run.
 */
void
suggestions_given(
    run& _run
)
{
    static const char* const MISTYPED[] = { "--numbr", "1" };

    (void)_run.app().add_option("--number");
    _run.note(std::string("off: ") +
              (_run.app().get_suggestions() ? "1" : "0"));
    _run.parse(MISTYPED,
               2u);
    _run.app().suggestions();
    _run.note(std::string("on: ") +
              (_run.app().get_suggestions() ? "1" : "0"));
    _run.parse(MISTYPED,
               2u);

    return;
}

// SUGGESTIONS_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_suggestions.
const step_fn SUGGESTIONS_STEPS[] =
{
    &suggestions_given
};

// SUGGESTIONS_COUNT
//   constant: how many steps SUGGESTIONS_STEPS holds.
const std::size_t SUGGESTIONS_COUNT =
    sizeof(SUGGESTIONS_STEPS) / sizeof(SUGGESTIONS_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

/**
 * @brief Writes the completion option a parser has, by name, and its
 *        description.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line is.
 */
void
write_completion(
    run&        _run,
    const char* _label
)
{
    const cli::option flag = _run.app().get_completion_ptr();

    _run.note(std::string(_label) +
              ": valid=" + (flag.valid() ? "1" : "0") +
              " name=[" + flag.get_name() +
              "] description=[" + flag.get_description() + "]");

    return;
}

/**
 * @brief COMPLETION_FLAG 1: the completion option set, asked for by a
 *        parse and ended -- the script, which the completion suite holds,
 *        written as its first line and whether it is the error's message
 *        -- then removed.
 *
 * @param[in,out] _run  the step's run.
 */
void
completion_asked(
    run& _run
)
{
    static const char* const ASKED[] = { "--completion", "bash" };
    std::ostringstream       out;
    std::ostringstream       err;

    write_completion(_run,
                     "made");
    (void)_run.app().set_completion_flag("--completion",
                                         "write a completion script");
    write_completion(_run,
                     "set");
    (void)_run.parse_quiet(ASKED,
                           2u);

    const cli::error  asked  = _run.app().get_error();
    const int         code   = _run.app().exit(asked,
                                               out,
                                               err);
    const std::string script = out.str();

    _run.note(std::string("asked: status=") + asked.get_name() +
              " exit=" + number(static_cast<long>(code)) +
              " first line=[" + script.substr(0,
                                              script.find('\n')) +
              "] the message=" +
              ( (script == std::string(asked.what())) ? "1" : "0" ) +
              " err=[" + escape(err.str()) + "]");
    (void)_run.app().set_completion_flag();
    write_completion(_run,
                     "removed");

    return;
}

// COMPLETION_FLAG_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_completion_flag.
const step_fn COMPLETION_FLAG_STEPS[] =
{
    &completion_asked
};

// COMPLETION_FLAG_COUNT
//   constant: how many steps COMPLETION_FLAG_STEPS holds.
const std::size_t COMPLETION_FLAG_COUNT =
    sizeof(COMPLETION_FLAG_STEPS) / sizeof(COMPLETION_FLAG_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

// the parser is its C struct and nothing more, so `&app` is the C parser
D_STATIC_ASSERT( (sizeof(cli::arg_parser) == sizeof(::d_arg_parser)),
                 "the parser adds no member to its C struct" );

// a handle is one pointer, as CLI11's Option* and App* are
D_STATIC_ASSERT( (sizeof(cli::command) == sizeof(::d_arg_command*)),
                 "a command handle is one pointer" );
D_STATIC_ASSERT( (sizeof(cli::option_group) == sizeof(::d_arg_command*)),
                 "an option group's handle is one pointer" );
D_STATIC_ASSERT( (sizeof(cli::option) == sizeof(::d_arg_option*)),
                 "an option handle is one pointer" );
D_STATIC_ASSERT( (sizeof(cli::option_defaults) ==
                  sizeof(::d_arg_option_defaults*)),
                 "an option defaults handle is one pointer" );
D_STATIC_ASSERT( (sizeof(cli::validator_handle) ==
                  sizeof(::d_arg_validator*)),
                 "a validator handle is one pointer" );

// a view is the pointer it reads through, a list a pointer and a count
D_STATIC_ASSERT( (sizeof(cli::error) == sizeof(const ::d_arg_error*)),
                 "an error is a view of one pointer" );
D_STATIC_ASSERT( (sizeof(cli::buffer) == sizeof(::d_arg_buffer*)),
                 "a buffer is a view of one pointer" );
D_STATIC_ASSERT( (sizeof(cli::c_string) == sizeof(const char*)),
                 "a text parameter is one pointer" );
D_STATIC_ASSERT( (sizeof(cli::text_list) ==
                  (sizeof(const ::d_pack_text*) + sizeof(std::size_t))),
                 "a list of texts is a pointer and a count" );
D_STATIC_ASSERT( (sizeof(cli::option_list) ==
                  (sizeof(::d_arg_option* const*) + sizeof(std::size_t))),
                 "a list of handles is a pointer and a count" );

// a value is its C struct, or its C value
D_STATIC_ASSERT( (sizeof(cli::status) == sizeof(int)),
                 "a status is its code" );
D_STATIC_ASSERT( (sizeof(cli::text) == sizeof(::d_pack_text)),
                 "a text is its span" );
D_STATIC_ASSERT( (sizeof(cli::validator) == sizeof(::d_arg_validator)),
                 "a validator adds no member to its C struct" );
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
D_STATIC_ASSERT( (sizeof(cli::filter) == sizeof(::d_arg_filter)),
                 "a filter adds no member to its C struct" );
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

// a formatter and a configuration format add their virtual functions' table
// alone, which the C tables call through
D_STATIC_ASSERT( (sizeof(cli::formatter) ==
                  (sizeof(::d_arg_formatter) + sizeof(void*))),
                 "a formatter adds its table of virtual functions alone" );
D_STATIC_ASSERT( (sizeof(cli::config_format) ==
                  (sizeof(::d_arg_config_format) + sizeof(void*))),
                 "a configuration format adds its table alone" );
D_STATIC_ASSERT( (sizeof(cli::config_ini) == sizeof(cli::config_format)),
                 "the INI format adds nothing to the format" );

/**
 * @brief LAYOUT 1: the face's records read as the C records they are: the
 *        parser as the C parser, handles as the C pointers the C lookups
 *        give, values at the address of their C structs.
 *
 * @param[in,out] _run  the step's run.
 */
void
layout_records(
    run& _run
)
{
    cli::arg_parser&       app       = _run.app();
    ::d_arg_parser* const  as_c      = &app;
    const cli::command     sub       = app.add_subcommand("sub");
    const cli::option      option    = app.add_option("--n");
    const cli::validator   checked   = cli::range(1,
                                                  5);
    const cli::text        word("word");
    std::ostringstream     line;

    line << "parser: c=" << (static_cast<const void*>(as_c) ==
                             static_cast<const void*>(&app))
         << " root=" << (cli::command(app).get() == &app.command)
         << " sub=" << (sub.get() == d_arg_command_subcommand(&app.command,
                                                              "sub"))
         << " option=" << (option.get() ==
                           d_arg_command_option(&app.command,
                                                "--n"))
         << " defaults=" << (app.option_defaults().get() ==
                             d_arg_command_option_defaults(&app.command))
         << " validator=" << (static_cast<const void*>(
                                  static_cast<const ::d_arg_validator*>(
                                      &checked)) ==
                              static_cast<const void*>(&checked))
         << " text=" << (static_cast<const void*>(
                             static_cast<const ::d_pack_text*>(&word)) ==
                         static_cast<const void*>(&word))
         << " span=" << (static_cast<const ::d_pack_text&>(word).length ==
                         4u);
    _run.note(line.str());

    return;
}

// LAYOUT_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_layout.
const step_fn LAYOUT_STEPS[] =
{
    &layout_records
};

// LAYOUT_COUNT
//   constant: how many steps LAYOUT_STEPS holds.
const std::size_t LAYOUT_COUNT =
    sizeof(LAYOUT_STEPS) / sizeof(LAYOUT_STEPS[0]);

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// a handle, a view and a status copy as the pointers and values they hold
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::command>::value),
                 "a command handle copies as a pointer" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::option_group>::value),
                 "an option group's handle copies as a pointer" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::option>::value),
                 "an option handle copies as a pointer" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::option_defaults>::value),
                 "an option defaults handle copies as a pointer" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::validator_handle>::value),
                 "a validator handle copies as a pointer" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::error>::value),
                 "an error view copies as a pointer" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::status>::value),
                 "a status copies as its code" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::text>::value),
                 "a text copies as its span" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::text_list>::value),
                 "a list of texts copies as a pointer and a count" );
D_STATIC_ASSERT( (std::is_trivially_copyable<cli::option_list>::value),
                 "a list of handles copies as a pointer and a count" );

// the parser is standard layout, so its address is its C struct's
D_STATIC_ASSERT( (std::is_standard_layout<cli::arg_parser>::value),
                 "the parser is standard layout" );

/**
 * @brief TRAITS 1: what the standard's traits tell of the face's types, as
 *        the assertions beside this hold at build time.
 *
 * @param[in,out] _run  the step's run.
 */
void
traits_told(
    run& _run
)
{
    std::ostringstream line;

    line << "trivially copyable: command="
         << std::is_trivially_copyable<cli::command>::value
         << " option=" << std::is_trivially_copyable<cli::option>::value
         << " error=" << std::is_trivially_copyable<cli::error>::value
         << " status=" << std::is_trivially_copyable<cli::status>::value
         << " text=" << std::is_trivially_copyable<cli::text>::value
         << " lists="
         << std::is_trivially_copyable<cli::text_list>::value
         << " standard layout: parser="
         << std::is_standard_layout<cli::arg_parser>::value
         << " copyable parser="
         << std::is_copy_constructible<cli::arg_parser>::value;
    _run.note(line.str());

    return;
}

// TRAITS_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_traits.
const step_fn TRAITS_STEPS[] =
{
    &traits_told
};

// TRAITS_COUNT
//   constant: how many steps TRAITS_STEPS holds.
const std::size_t TRAITS_COUNT =
    sizeof(TRAITS_STEPS) / sizeof(TRAITS_STEPS[0]);

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

/**
 * @brief DEPARTED 1: a vector whose parse failed, left as it was given;
 *        one whose parse went on, left with what the root did not use.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_vector(
    run& _run
)
{
    std::vector<std::string> args;

    (void)_run.make_option("number",
                           _run.root(),
                           "-n");
    args.push_back("x");
    args.push_back("-n");
    args.push_back("--nope");
    _run.parse_vector(args);
    _run.note("failed, left: [" + joined(args) + "]");
    _run.app().allow_extras();
    _run.parse_vector(args);
    _run.note("went on, left: [" + joined(args) + "]");

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_parser_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_vector
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

}  // namespace

//==============================================================================
// 3.  ENDINGS
//==============================================================================

/*
d_tests_arg_cpp_parser_exits
  The endings exit() writes for each kind of result, as CLI11's exit()
writes them.
  Tests the following:
  - help, help of everything and a version asked for
  - a required option left out, an argument not expected, a value that does
    not convert, options excluding each other
  - help asked for in a subcommand, and a failure there, under CLI11's
    simple message and its help message
  - a parse that goes on
*/
bool
d_tests_arg_cpp_parser_exits()
{
    return steps_hold("EXITS",
                      EXITS_STEPS,
                      EXITS_COUNT,
                      EXITS);
}

/*
d_tests_arg_cpp_parser_macro
  main through D_ARG_PARSE, as through CLI11_PARSE.
  Tests the following:
  - a parse that goes on: main goes on
  - help and a version asked for, and a failure: main returns exit()'s code,
    what it wrote written where the parser writes
*/
bool
d_tests_arg_cpp_parser_macro()
{
    return steps_hold("MACRO",
                      MACRO_STEPS,
                      MACRO_COUNT,
                      MACRO);
}

//==============================================================================
// 4.  PARSES
//==============================================================================

/*
d_tests_arg_cpp_parser_forms
  The parse's forms for a line and for a vector, as CLI11's.
  Tests the following:
  - lines without the program's name and with it, quoted values, `=`,
    spaces around, a NUL byte kept in its word
  - a vector given last first, holding after what the root did not use
*/
bool
d_tests_arg_cpp_parser_forms()
{
    return steps_hold("FORMS",
                      FORMS_STEPS,
                      FORMS_COUNT,
                      FORMS);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_parser_forms_moved
  The parse of a vector it consumes, from C++11, as CLI11's.
  Tests the following:
  - parse(std::vector<std::string>&&)
*/
bool
d_tests_arg_cpp_parser_forms_moved()
{
    return steps_hold("FORMS_MOVED",
                      FORMS_MOVED_STEPS,
                      FORMS_MOVED_COUNT,
                      FORMS_MOVED);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

/*
d_tests_arg_cpp_parser_forms_stream
  Configuration text parsed from a stream, as CLI11's parse_from_stream.
  Tests the following:
  - values for the root's options and for a subcommand's section
*/
bool
d_tests_arg_cpp_parser_forms_stream()
{
    return steps_hold("FORMS_STREAM",
                      FORMS_STREAM_STEPS,
                      FORMS_STREAM_COUNT,
                      FORMS_STREAM);
}

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/*
d_tests_arg_cpp_parser_forms_wide
  The parse's wide forms, as CLI11's.
  Tests the following:
  - wmain's arguments, a value beyond ASCII among them, made UTF-8
  - wide lines without the program's name and with it
*/
bool
d_tests_arg_cpp_parser_forms_wide()
{
    return steps_hold("FORMS_WIDE",
                      FORMS_WIDE_STEPS,
                      FORMS_WIDE_COUNT,
                      FORMS_WIDE);
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

/*
d_tests_arg_cpp_parser_names
  The name a parse gives a root without one, as CLI11's.
  Tests the following:
  - a root made without a name, named by main's arguments, by a line, and by
    a line naming the program in quotes
  - a name set, which parses keep
*/
bool
d_tests_arg_cpp_parser_names()
{
    return steps_hold("NAMES",
                      NAMES_STEPS,
                      NAMES_COUNT,
                      NAMES);
}

/*
d_tests_arg_cpp_parser_state
  What a parse forgets, as CLI11's.
  Tests the following:
  - clear() after a parse with values, a subcommand and arguments left
  - the parse after it
*/
bool
d_tests_arg_cpp_parser_state()
{
    return steps_hold("STATE",
                      STATE_STEPS,
                      STATE_COUNT,
                      STATE);
}

//==============================================================================
// 5.  THE PARSER'S OWN
//==============================================================================

/*
d_tests_arg_cpp_parser_errors
  Failures kept in the parser and read through views (ISSUES 49), where
CLI11 throws them.
  Tests the following:
  - get_error before any parse, after a failure, and a view kept past the
    next parse, which reads its report
  - get_definition_error after a definition failed, reported by the next
    parse; clear_definition_error, after which parses go on
  - get_error after clear()
*/
bool
d_tests_arg_cpp_parser_errors()
{
    return steps_hold("ERRORS",
                      ERRORS_STEPS,
                      ERRORS_COUNT,
                      ERRORS);
}

/*
d_tests_arg_cpp_parser_output
  The parser's own output and error output, which CLI11's App has not.
  Tests the following:
  - output to strings, then to streams
  - exit() of a status and of an error view to them; to an output given,
    and to an output and an error output given
*/
bool
d_tests_arg_cpp_parser_output()
{
    return steps_hold("OUTPUT",
                      OUTPUT_STEPS,
                      OUTPUT_COUNT,
                      OUTPUT);
}

/*
d_tests_arg_cpp_parser_statuses
  Statuses ended that are not the last parse's, which CLI11's exit(),
taking an Error, cannot be given.
  Tests the following:
  - ok, a success, a runtime error, an error, a code that is no status
  - the last parse's status, with its message, and another, with its own
*/
bool
d_tests_arg_cpp_parser_statuses()
{
    return steps_hold("STATUSES",
                      STATUSES_STEPS,
                      STATUSES_COUNT,
                      STATUSES);
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/*
d_tests_arg_cpp_parser_response_files
  Response files, which CLI11 has not.
  Tests the following:
  - off at first, with the platform's rules for response files
  - set on with POSIX rules: a file's arguments, quoted ones kept whole
  - set off: the argument as it is
*/
bool
d_tests_arg_cpp_parser_response_files()
{
    return steps_hold("RESPONSE_FILES",
                      RESPONSE_FILES_STEPS,
                      RESPONSE_FILES_COUNT,
                      RESPONSE_FILES);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

/*
d_tests_arg_cpp_parser_suggestions
  "Did you mean", which CLI11 has not.
  Tests the following:
  - a name mistyped with suggestions off, then on
*/
bool
d_tests_arg_cpp_parser_suggestions()
{
    return steps_hold("SUGGESTIONS",
                      SUGGESTIONS_STEPS,
                      SUGGESTIONS_COUNT,
                      SUGGESTIONS);
}

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

/*
d_tests_arg_cpp_parser_completion_flag
  The completion option, which CLI11 has not.
  Tests the following:
  - none at first; set, read back; asked for by a parse, which exit() ends
    with a script
  - removed
*/
bool
d_tests_arg_cpp_parser_completion_flag()
{
    return steps_hold("COMPLETION_FLAG",
                      COMPLETION_FLAG_STEPS,
                      COMPLETION_FLAG_COUNT,
                      COMPLETION_FLAG);
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

//==============================================================================
// 6.  ZERO OVERHEAD
//==============================================================================

/*
d_tests_arg_cpp_parser_layout
  What the face costs: nothing but the C records, which the build asserts
beside this section; and the records read as the C records they are.
  Tests the following:
  - the parser at its C struct's address, its root the handle on its own
    command record
  - handles holding the pointers the C lookups give
  - a validator and a text at their C structs' addresses
*/
bool
d_tests_arg_cpp_parser_layout()
{
    return steps_hold("LAYOUT",
                      LAYOUT_STEPS,
                      LAYOUT_COUNT,
                      LAYOUT);
}

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_arg_cpp_parser_traits
  What the standard's traits tell of the face's types, from C++11, which
the build asserts beside this section.
  Tests the following:
  - handles, views, statuses, texts and lists trivially copyable
  - the parser standard layout, and not copyable
*/
bool
d_tests_arg_cpp_parser_traits()
{
    return steps_hold("TRAITS",
                      TRAITS_STEPS,
                      TRAITS_COUNT,
                      TRAITS);
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

//==============================================================================
// 7.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_parser_departures
  Where the face's parse forms depart from CLI11's (ISSUES 58).
  Tests the following:
  - a vector whose parse failed, left as it was given
  - the same vector parsed with extras allowed, left with what the root did
    not use
*/
bool
d_tests_arg_cpp_parser_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

//==============================================================================
// 8.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_parser
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_parser[] =
{
    { "cpp_parser_exits",
      d_tests_arg_cpp_parser_exits },
    { "cpp_parser_macro",
      d_tests_arg_cpp_parser_macro },
    { "cpp_parser_forms",
      d_tests_arg_cpp_parser_forms },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_parser_forms_moved",
      d_tests_arg_cpp_parser_forms_moved },
#endif
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
    { "cpp_parser_forms_stream",
      d_tests_arg_cpp_parser_forms_stream },
#endif
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "cpp_parser_forms_wide",
      d_tests_arg_cpp_parser_forms_wide },
#endif
    { "cpp_parser_names",
      d_tests_arg_cpp_parser_names },
    { "cpp_parser_state",
      d_tests_arg_cpp_parser_state },
    { "cpp_parser_errors",
      d_tests_arg_cpp_parser_errors },
    { "cpp_parser_output",
      d_tests_arg_cpp_parser_output },
    { "cpp_parser_statuses",
      d_tests_arg_cpp_parser_statuses },
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "cpp_parser_response_files",
      d_tests_arg_cpp_parser_response_files },
#endif
#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
    { "cpp_parser_suggestions",
      d_tests_arg_cpp_parser_suggestions },
#endif
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    { "cpp_parser_completion_flag",
      d_tests_arg_cpp_parser_completion_flag },
#endif
    { "cpp_parser_layout",
      d_tests_arg_cpp_parser_layout },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { "cpp_parser_traits",
      d_tests_arg_cpp_parser_traits },
#endif
    { "cpp_parser_departures",
      d_tests_arg_cpp_parser_departures }
};

// d_tests_arg_cpp_parser_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_parser_count =
    sizeof(d_tests_arg_cpp_parser) / sizeof(d_tests_arg_cpp_parser[0]);
