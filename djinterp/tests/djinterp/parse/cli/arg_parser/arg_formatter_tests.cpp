/*******************************************************************************
* djinterp [parse]                                       arg_formatter_tests.cpp
*
* The argument parser's C++ formatter suite's sections.
*   The sections held to CLI11 run what the CASES section defines: a
* formatter's settings and labels, read back; one program's help, written
* by the command's own formatter and by formatters of each setting given to
* it; help a function writes; steps replaced by classes deriving from the
* formatter, one of them every step; and formatters copied.
* tools/cppref/cppref.py compiles that section against CLI11 2.7.2, through
* the face's names for CLI11's (cli11_shim.hpp, whose formatter is a
* Formatter shared with the App it is given to, its steps the formatter's
* virtual functions), and the transcripts below are what CLI11 wrote; so
* the CASES section is written in the part of the face CLI11 shares --
* each setting a statement of its own, as CLI11's return nothing, each
* formatter a static object, which a command here borrows, and each given
* to a command before the command's subcommands are made, which CLI11's
* copy when they are made. The sections that are the module's own are
* written apart, after it, each saying why CLI11 cannot write its
* transcript.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_formatter_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_formatter_tests.hpp"  // corresponding header
// std
#include <cstddef>   // std::size_t
#include <sstream>   // std::ostringstream
#include <string>    // std::string
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

// SETTINGS
//   constant: what the steps of d_tests_arg_cpp_formatter_settings write, as
// CLI11 writes them.
const char SETTINGS[] =
    "made: column=30 right=65 description=80 footer=80 ratio=0.333333 "
    "paragraphs=1,1 defaults=1 types=1 flag_values=1 REQUIRED=[REQUIRED] "
    "POSITIONALS=[POSITIONALS]\n"
    "set: column=40 right=50 description=60 footer=70 ratio=0.5 paragraphs=0,0 "
    "defaults=0 types=0 flag_values=0 REQUIRED=[NEEDED] "
    "POSITIONALS=[ARGUMENTS]\n"
    "set again: column=40 right=50 description=60 footer=70 ratio=0.5 "
    "paragraphs=1,0 defaults=1 types=0 flag_values=0 REQUIRED=[WANTED] "
    "POSITIONALS=[ARGUMENTS]\n";

// HELP
//   constant: what the steps of d_tests_arg_cpp_formatter_help write, as
// CLI11 writes them.
const char HELP[] =
    "own [A program whose description runs long enough to be wrapped where its "
    "paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name TEXT [joe]   a name\\n  -v,     --verbose     "
    "      say more\\n          --level{3}          a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "given [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name TEXT [joe]   a name\\n  -v,     --verbose     "
    "      say more\\n          --level{3}          a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "all [A program whose description runs long enough to be wrapped where its "
    "paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name TEXT [joe]   a name\\n  -v,     --verbose     "
    "      say more\\n          --level{3}          a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\nsub\\na subcommand\\n\\n\\nOPTIONS:\\n        "
    "  --inner             an option of the subcommand\\n\\n\\nA footer that "
    "runs long enough to be wrapped where its paragraph is narrow.\\n]\n"
    "prefixed [A program whose description runs long enough to be wrapped "
    "where its paragraph\\nis narrow.\\n\\n\\nouter prog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name TEXT [joe]   a name\\n  -v,     --verbose     "
    "      say more\\n          --level{3}          a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "sub [a subcommand\\n\\n\\nsub [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,     "
    "--help              Print this help message and exit\\n          --inner  "
    "           an option of the subcommand\\n\\nA footer that runs long "
    "enough to be wrapped where its paragraph is narrow.\\n]\n"
    "narrow [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file              the "
    "input\\n\\nOPTIONS:\\n  -h, --help        Print this help message and\\n  "
    "                  exit\\n  -n, --number REQUIRED \\n                    a "
    "number, which is required\\n      --name TEXT [joe]  \\n                  "
    "  a name\\n  -v, --verbose     say more\\n      --level{3}    a flag "
    "counted as three\\n\\nAdvanced:\\n      --deep        an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub               a subcommand\\n\\nA "
    "footer that runs long enough to be wrapped where its paragraph is "
    "narrow.\\n]\n"
    "narrow paragraphs [A program whose description\\nruns long enough to be "
    "wrapped\\nwhere its paragraph is narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name TEXT [joe]   a name\\n  -v,     --verbose     "
    "      say more\\n          --level{3}          a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long\\nenough to be wrapped\\nwhere "
    "its paragraph is\\nnarrow.\\n]\n"
    "unformatted [A program whose description runs long enough to be wrapped "
    "where its paragraph is narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name TEXT [joe]   a name\\n  -v,     --verbose     "
    "      say more\\n          --level{3}          a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "bare [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number REQUIRED   a number, which is "
    "required\\n          --name              a name\\n  -v,     --verbose     "
    "      say more\\n          --level             a flag counted as "
    "three\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "labelled [A program whose description runs long enough to be wrapped "
    "where its paragraph\\nis narrow.\\n\\n\\nprog [SETTINGS] [file] "
    "[COMMANDS]\\n\\n\\nARGUMENTS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number NEEDED     a number, which is "
    "required\\n          --name WORDS [joe]  a name\\n  -v,     --verbose "
    "Shuns: --first \\n                              say more\\n          "
    "--level{3}          a flag counted as three\\n          --count WHOLE "
    "(From:COUNT) \\n                              a count\\n          --first "
    "Wants: --count Shuns: --verbose \\n                              a flag "
    "with relations\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "unnamed [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nUse: [SETTINGS] [file] "
    "[COMMAND]\\n\\n\\nARGUMENTS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit\\n  -n,     --number NEEDED     a number, which is "
    "required\\n          --name WORDS [joe]  a name\\n  -v,     --verbose "
    "Shuns: --first \\n                              say more\\n          "
    "--level{3}          a flag counted as three\\n          --count WHOLE "
    "(From:COUNT) \\n                              a count\\n          --first "
    "Wants: --count Shuns: --verbose \\n                              a flag "
    "with relations\\n\\nAdvanced:\\n          --deep              an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "half [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,          --help         Print this help "
    "message and exit\\n  -n,          --number REQUIRED \\n                   "
    "           a number, which is required\\n               --name TEXT [joe] "
    " \\n                              a name\\n  -v,          --verbose      "
    "say more\\n               --level{3}     a flag counted as "
    "three\\n\\nAdvanced:\\n               --deep         an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n"
    "start [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h, --help                  Print this help "
    "message and exit\\n  -n, --number REQUIRED       a number, which is "
    "required\\n--name TEXT [joe]             a name\\n  -v, --verbose         "
    "      say more\\n--level{3}                    a flag counted as "
    "three\\n\\nAdvanced:\\n--deep                        an advanced "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a "
    "subcommand\\n\\nA footer that runs long enough to be wrapped where its "
    "paragraph is narrow.\\n]\n";

// LAMBDA
//   constant: what the steps of d_tests_arg_cpp_formatter_lambda write, as
// CLI11 writes them.
const char LAMBDA[] =
    "root [help of prog as [prog] normal\\n]\n"
    "all [help of prog as [prog] all\\n]\n"
    "prefixed [help of prog as [outer prog] normal\\n]\n"
    "sub [help of sub as [sub] normal\\n]\n";

// STEPS
//   constant: what the steps of d_tests_arg_cpp_formatter_steps write, as
// CLI11 writes them through subclasses of its Formatter replacing the same
// steps.
const char STEPS[] =
    "marked [A program whose description runs long enough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n(usage above)\\n\\nPOSITIONALS:\\n  file               "
    "         the input (!)\\n\\nOPTIONS:\\n  -h,     --help              "
    "Print this help message and exit (!)\\n  -n,     --number REQUIRED   a "
    "number, which is required (!)\\n          --name TEXT [joe]   a name "
    "(!)\\n  -v,     --verbose           say more (!)\\n          --level{3}   "
    "       a flag counted as three (!)\\n\\nAdvanced:\\n          --deep      "
    "        an advanced setting (!)\\n\\nSUBCOMMANDS:\\n  sub                 "
    "        a subcommand\\nmarked footer]\n"
    "sub [a subcommand\\n\\n\\nsub [OPTIONS]\\n\\n(usage "
    "above)\\n\\nOPTIONS:\\n  -h,     --help              Print this help "
    "message and exit (!)\\n          --inner             an option of the "
    "subcommand (!)\\nmarked footer]\n"
    "bracketed [<H:<D:A program whose description runs long enough to be "
    "wrapped where its\\nparagraph is narrow.\\n\\n:D><U:\\nprog [OPTIONS] "
    "<OU:[<ON:file:ON>]:OU> [SUBCOMMANDS]\\n\\n:U><P:<G:\\nPOSITIONALS:\\n<O:  "
    "<ON:file:ON><OO::OO>        <OD:the "
    "input:OD>\\n:O>:G>:P><G:\\nOPTIONS:\\n<O:  <ON:-h, --help:ON><OO::OO>  "
    "<OD:Print this help message and exit:OD>\\n:O><O:  <ON:-n, "
    "--number:ON><OO: REQUIRED:OO> \\n                              <OD:a "
    "number, which is required:OD>\\n:O><O:          <ON:--name:ON><OO: TEXT "
    "[joe] :OO> \\n                              <OD:a name:OD>\\n:O><O:  "
    "<ON:-v, --verbose:ON><OO::OO> \\n                              <OD:say "
    "more:OD>\\n:O><O:          <ON:--level{3}:ON><OO::OO> \\n                 "
    "             <OD:a flag counted as "
    "three:OD>\\n:O>:G><G:\\nAdvanced:\\n<O:          <ON:--deep:ON><OO::OO> "
    "\\n                              <OD:an advanced "
    "setting:OD>\\n:O>:G><SS:<E:[Option Group: Extra]\\n  <D:extra options\\n  "
    "\\n  :D>  <P::P>  <G:\\n  OPTIONS:\\n  <O:          "
    "<ON:--extra:ON><OO::OO> \\n                                <OD:an extra "
    "flag:OD>\\n  :O>:G>  <SS::SS><F::F>:E>\\nSUBCOMMANDS:\\n<S:  sub          "
    "               a subcommand\\n:S>:SS><F:\\nA footer that runs long enough "
    "to be wrapped where its paragraph is narrow.\\n:F>:H>]\n"
    "all [<H:<D:A program whose description runs long enough to be wrapped "
    "where its\\nparagraph is narrow.\\n\\n:D><U:\\nprog [OPTIONS] "
    "<OU:[<ON:file:ON>]:OU> [SUBCOMMANDS]\\n\\n:U><P:<G:\\nPOSITIONALS:\\n<O:  "
    "<ON:file:ON><OO::OO>        <OD:the "
    "input:OD>\\n:O>:G>:P><G:\\nOPTIONS:\\n<O:  <ON:-h, --help:ON><OO::OO>  "
    "<OD:Print this help message and exit:OD>\\n:O><O:  <ON:-n, "
    "--number:ON><OO: REQUIRED:OO> \\n                              <OD:a "
    "number, which is required:OD>\\n:O><O:          <ON:--name:ON><OO: TEXT "
    "[joe] :OO> \\n                              <OD:a name:OD>\\n:O><O:  "
    "<ON:-v, --verbose:ON><OO::OO> \\n                              <OD:say "
    "more:OD>\\n:O><O:          <ON:--level{3}:ON><OO::OO> \\n                 "
    "             <OD:a flag counted as "
    "three:OD>\\n:O>:G><G:\\nAdvanced:\\n<O:          <ON:--deep:ON><OO::OO> "
    "\\n                              <OD:an advanced "
    "setting:OD>\\n:O>:G><SS:<E:[Option Group: Extra]\\n  <D:extra options\\n  "
    "\\n  :D>  <P::P>  <G:\\n  OPTIONS:\\n  <O:          "
    "<ON:--extra:ON><OO::OO> \\n                                <OD:an extra "
    "flag:OD>\\n  :O>:G>  <SS::SS><F::F>:E>\\nSUBCOMMANDS:\\n<H:<E:sub\\n<D:a "
    "subcommand\\n\\n:D><P::P><G:\\nOPTIONS:\\n<O:          "
    "<ON:--inner:ON><OO::OO> \\n                              <OD:an option of "
    "the subcommand:OD>\\n:O>:G><SS::SS>:E>:H>\\n:SS><F:\\nA footer that runs "
    "long enough to be wrapped where its paragraph is narrow.\\n:F>:H>]\n"
    "sub [<H:<D:a subcommand\\n\\n:D><U:\\nsub "
    "[OPTIONS]\\n\\n:U><P::P><G:\\nOPTIONS:\\n<O:  <ON:-h, --help:ON><OO::OO>  "
    "<OD:Print this help message and exit:OD>\\n:O><O:          "
    "<ON:--inner:ON><OO::OO> \\n                              <OD:an option of "
    "the subcommand:OD>\\n:O>:G><SS::SS><F:\\nA footer that runs long enough "
    "to be wrapped where its paragraph is narrow.\\n:F>:H>]\n";

// COPIES
//   constant: what the steps of d_tests_arg_cpp_formatter_copies write, as
// CLI11 writes them through copies of its Formatter.
const char COPIES[] =
    "original: column=25 right=65 description=80 footer=80 ratio=0.333333 "
    "paragraphs=1,1 defaults=1 types=1 flag_values=1 REQUIRED=[NEEDED] "
    "POSITIONALS=[POSITIONALS]\n"
    "copied: column=35 right=65 description=80 footer=80 ratio=0.333333 "
    "paragraphs=1,1 defaults=1 types=1 flag_values=1 REQUIRED=[NEEDED] "
    "POSITIONALS=[POSITIONALS]\n"
    "assigned: column=25 right=65 description=80 footer=80 ratio=0.333333 "
    "paragraphs=1,1 defaults=1 types=1 flag_values=1 REQUIRED=[WANTED] "
    "POSITIONALS=[POSITIONALS]\n"
    "sliced [d\\n\\n\\nprog [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h, --help   Print "
    "this help message and exit\\n  -x           an option\\n]\n";

// FAILED
//   constant: what the steps of d_tests_arg_cpp_formatter_failed write: the
// module's own, as a step here that fails returns its status, which ends
// help, where CLI11's could only throw.
const char FAILED[] =
    "string=[] sink=ConversionError written=[]\n";

// LINES
//   constant: what the steps of d_tests_arg_cpp_formatter_lines write: the
// module's own, as CLI11's help has no longest line, and no terminal width.
const char LINES[] =
    "made: line=0 terminal=0\n"
    "set: line=40 terminal=1\n"
    "set again: line=40 terminal=0\n"
    "forty [A program whose description runs long\\nenough to be wrapped where "
    "its paragraph\\nis narrow.\\n\\n\\nprog [OPTIONS] [file] "
    "[SUBCOMMANDS]\\n\\n\\nPOSITIONALS:\\n  file                        the "
    "input\\n\\nOPTIONS:\\n  -h,     --help              Print this\\n         "
    "                     help\\n                              message\\n      "
    "                        and exit\\n  -n,     --number REQUIRED   a "
    "number,\\n                              which is\\n                       "
    "       required\\n          --name TEXT [joe]   a name\\n  -v,     "
    "--verbose           say more\\n          --level{3}          a flag\\n    "
    "                          counted as\\n                              "
    "three\\n\\nAdvanced:\\n          --deep              an\\n                "
    "              advanced\\n                              "
    "setting\\n\\nSUBCOMMANDS:\\n  sub                         a\\n            "
    "                  subcommand\\n\\nA footer that runs long enough to "
    "be\\nwrapped where its paragraph is narrow.\\n]\n";

// DEPARTED
//   constant: what the steps of d_tests_arg_cpp_formatter_departures write:
// the module's own, where the face departs from CLI11: CLI11 copies a
// command's formatter into a subcommand when the subcommand is made, so
// that one made before the root is given a formatter keeps the one it was
// made with -- its help in columns 30 wide, `  -h,     --help` and then the
// description at the thirtieth column -- where here a subcommand without a
// formatter of its own follows its parent's whenever help is made (ISSUES
// 18).
const char DEPARTED[] =
    "followed [a subcommand\\n\\n\\nsub [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h, "
    "--help \\n            Print this help message and exit\\n    --inner an "
    "option of the subcommand, described at length\\n]\n"
    "own [a subcommand\\n\\n\\nsub [OPTIONS]\\n\\n\\nOPTIONS:\\n  -h,        "
    "--help                     Print this help message and exit\\n            "
    " --inner                    an option of the subcommand, described at "
    "length\\n]\n";

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

// 1.1    The program and its writers
//------------------------------------------------------------------------------

/**
 * @brief Writes a formatter's settings, and two of its labels, on one
 *        line.
 *
 * @param[in,out] _run        the run.
 * @param[in]     _label      what the line names the formatter by.
 * @param[in]     _formatter  the formatter.
 */
void
write_formatter(
    run&                  _run,
    const char*           _label,
    const cli::formatter& _formatter
)
{
    std::ostringstream line;

    line << _label
         << ": column=" << _formatter.get_column_width()
         << " right=" << _formatter.get_right_column_width()
         << " description=" << _formatter.get_description_paragraph_width()
         << " footer=" << _formatter.get_footer_paragraph_width()
         << " ratio=" << _formatter.get_long_option_alignment_ratio()
         << " paragraphs="
         << _formatter.is_description_paragraph_formatting_enabled()
         << "," << _formatter.is_footer_paragraph_formatting_enabled()
         << " defaults=" << _formatter.is_option_defaults_enabled()
         << " types=" << _formatter.is_option_type_names_enabled()
         << " flag_values=" << _formatter.is_default_flag_values_enabled()
         << " REQUIRED=[" << _formatter.get_label("REQUIRED")
         << "] POSITIONALS=[" << _formatter.get_label("POSITIONALS")
         << "]";
    _run.note(line.str());

    return;
}

/**
 * @brief Defines the program the help steps write: a description and a
 *        footer, a required option, one with a captured default, flags --
 *        one with a value of its own -- a positional, an option in a group
 *        of its own, and a subcommand with an option.
 *
 * @param[in,out] _run  the run.
 * @return the subcommand.
 */
cli::command
define_program(
    run& _run
)
{
    static std::string name("joe");

    _run.app().description("A program whose description runs long enough to "
                           "be wrapped where its paragraph is narrow.");
    _run.app().footer("A footer that runs long enough to be wrapped where "
                      "its paragraph is narrow.");
    _run.app().add_option("-n,--number",
                          "a number, which is required")
              ->required();
    _run.app().add_option("--name",
                          name,
                          "a name")
              ->capture_default_str();
    (void)_run.app().add_flag("-v,--verbose",
                              "say more");
    (void)_run.app().add_flag("--level{3}",
                              "a flag counted as three");
    (void)_run.app().add_option("file",
                                "the input");
    _run.app().add_option("--deep",
                          "an advanced setting")
              ->group("Advanced");

    cli::command sub = _run.app().add_subcommand("sub",
                                                 "a subcommand");

    (void)sub->add_option("--inner",
                          "an option of the subcommand");

    return sub;
}

/**
 * @brief Writes a command's help as one escaped line, under a label.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _label  what the line names the help by.
 * @param[in]     _help   the help.
 */
void
write_help(
    run&               _run,
    const char*        _label,
    const std::string& _help
)
{
    _run.note(std::string(_label) + " [" + escape(_help) + "]");

    return;
}

// 1.2    Settings
//------------------------------------------------------------------------------

/**
 * @brief SETTINGS 1: a formatter's settings as made, then each set and
 *        read back, and labels given and replaced.
 *
 * @param[in,out] _run  the step's run.
 */
void
settings_read(
    run& _run
)
{
    cli::formatter made;

    write_formatter(_run,
                    "made",
                    made);
    made.column_width(40u);
    made.right_column_width(50u);
    made.description_paragraph_width(60u);
    made.footer_paragraph_width(70u);
    made.long_option_alignment_ratio(0.5f);
    made.enable_description_formatting(false);
    made.enable_footer_formatting(false);
    made.enable_option_defaults(false);
    made.enable_option_type_names(false);
    made.enable_default_flag_values(false);
    made.label("REQUIRED",
               "NEEDED");
    made.label("POSITIONALS",
               "ARGUMENTS");
    write_formatter(_run,
                    "set",
                    made);
    made.enable_description_formatting();
    made.enable_option_defaults();
    made.label("REQUIRED",
               "WANTED");
    write_formatter(_run,
                    "set again",
                    made);

    return;
}

// SETTINGS_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_settings.
const step_fn SETTINGS_STEPS[] =
{
    &settings_read
};

// SETTINGS_COUNT
//   constant: how many steps SETTINGS_STEPS holds.
const std::size_t SETTINGS_COUNT =
    sizeof(SETTINGS_STEPS) / sizeof(SETTINGS_STEPS[0]);

// 1.3    Help
//------------------------------------------------------------------------------

/**
 * @brief HELP 1: the program's help by the command's own formatter, by a
 *        new one given to it, which writes the same, and expanded, prefixed
 *        and a subcommand's.
 *
 * @param[in,out] _run  the step's run.
 */
void
help_default(
    run& _run
)
{
    static cli::formatter given;
    const cli::command    sub = define_program(_run);

    write_help(_run,
               "own",
               _run.app().help());
    _run.app().formatter(given);
    write_help(_run,
               "given",
               _run.app().help());
    write_help(_run,
               "all",
               _run.app().help("",
                               cli::help_mode::all));
    write_help(_run,
               "prefixed",
               _run.app().help("outer"));
    write_help(_run,
               "sub",
               sub->help());

    return;
}

/**
 * @brief HELP 2: the program's help with the left and right columns made
 *        narrow.
 *
 * @param[in,out] _run  the step's run.
 */
void
help_widths(
    run& _run
)
{
    static cli::formatter narrow;

    narrow.column_width(20u);
    narrow.right_column_width(30u);
    _run.app().formatter(narrow);
    (void)define_program(_run);
    write_help(_run,
               "narrow",
               _run.app().help());

    return;
}

/**
 * @brief HELP 3: the program's help with the description's and the
 *        footer's paragraphs made narrow, then not formatted.
 *
 * @param[in,out] _run  the step's run.
 */
void
help_paragraphs(
    run& _run
)
{
    static cli::formatter paragraphs;

    paragraphs.description_paragraph_width(30u);
    paragraphs.footer_paragraph_width(25u);
    _run.app().formatter(paragraphs);
    (void)define_program(_run);
    write_help(_run,
               "narrow paragraphs",
               _run.app().help());
    paragraphs.enable_description_formatting(false);
    paragraphs.enable_footer_formatting(false);
    write_help(_run,
               "unformatted",
               _run.app().help());

    return;
}

/**
 * @brief HELP 4: the program's help without defaults, type names, and
 *        flags' values.
 *
 * @param[in,out] _run  the step's run.
 */
void
help_switches(
    run& _run
)
{
    static cli::formatter bare;

    bare.enable_option_defaults(false);
    bare.enable_option_type_names(false);
    bare.enable_default_flag_values(false);
    _run.app().formatter(bare);
    (void)define_program(_run);
    write_help(_run,
               "bare",
               _run.app().help());

    return;
}

/**
 * @brief HELP 5: the program's help with every fixed word and type name
 *        help writes translated by labels -- with an option read from the
 *        environment and one that needs and excludes others -- then of the
 *        program unnamed, at most one subcommand required.
 *
 * @param[in,out] _run  the step's run.
 */
void
help_labels(
    run& _run
)
{
    static cli::formatter labelled;
    static int            count = 0;

    labelled.label("REQUIRED",
                   "NEEDED");
    labelled.label("OPTIONS",
                   "SETTINGS");
    labelled.label("SUBCOMMANDS",
                   "COMMANDS");
    labelled.label("SUBCOMMAND",
                   "COMMAND");
    labelled.label("POSITIONALS",
                   "ARGUMENTS");
    labelled.label("Usage",
                   "Use");
    labelled.label("TEXT",
                   "WORDS");
    labelled.label("INT",
                   "WHOLE");
    labelled.label("Env",
                   "From");
    labelled.label("Needs",
                   "Wants");
    labelled.label("Excludes",
                   "Shuns");
    _run.app().formatter(labelled);
    (void)define_program(_run);

    const cli::option counted = _run.app().add_option("--count",
                                                      count,
                                                      "a count")
                                          ->envname("COUNT");

    _run.app().add_flag("--first",
                        "a flag with relations")
              ->needs(counted)
              ->excludes("--verbose");
    write_help(_run,
               "labelled",
               _run.app().help());
    _run.app().require_subcommand(0u,
                                  1u);
    _run.app().name("");
    write_help(_run,
               "unnamed",
               _run.app().help());

    return;
}

/**
 * @brief HELP 6: the program's help with its long options aligned at half
 *        the left column, and at its start.
 *
 * @param[in,out] _run  the step's run.
 */
void
help_alignment(
    run& _run
)
{
    static cli::formatter aligned;

    aligned.long_option_alignment_ratio(0.5f);
    _run.app().formatter(aligned);
    (void)define_program(_run);
    write_help(_run,
               "half",
               _run.app().help());
    aligned.long_option_alignment_ratio(0.0f);
    write_help(_run,
               "start",
               _run.app().help());

    return;
}

// HELP_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_help.
const step_fn HELP_STEPS[] =
{
    &help_default,
    &help_widths,
    &help_paragraphs,
    &help_switches,
    &help_labels,
    &help_alignment
};

// HELP_COUNT
//   constant: how many steps HELP_STEPS holds.
const std::size_t HELP_COUNT = sizeof(HELP_STEPS) / sizeof(HELP_STEPS[0]);

// 1.4    Help a function writes
//------------------------------------------------------------------------------

// tagged_help
//   struct: help written by a function, as CLI11's FormatterLambda takes one:
// the command's name, the name help shows, and the mode.
struct tagged_help
{
    /**
     * @brief Writes help.
     *
     * @param[in] _command  the command whose help it is.
     * @param[in] _name     the name help shows.
     * @param[in] _mode     the mode.
     * @return the help.
     */
    template<typename Command,
             typename Name,
             typename Mode>
    std::string
    operator()(
        const Command& _command,
        const Name&    _name,
        Mode           _mode
    ) const
    {
        return "help of " + std::string(_command->get_name()) +
               " as [" + std::string(_name) + "] " +
               ((_mode == cli::help_mode::all) ? "all" : "normal") + "\n";
    }
};

/**
 * @brief LAMBDA 1: help a function writes, given to the root before its
 *        subcommand is made: the root's, expanded and prefixed, and the
 *        subcommand's.
 *
 * @param[in,out] _run  the step's run.
 */
void
lambda_help(
    run& _run
)
{
    static const tagged_help                  writer = tagged_help();
    static cli::formatter_lambda<tagged_help> lambda(writer);

    _run.app().formatter(lambda);

    const cli::command sub = _run.app().add_subcommand("sub",
                                                       "a subcommand");

    write_help(_run,
               "root",
               _run.app().help());
    write_help(_run,
               "all",
               _run.app().help("",
                               cli::help_mode::all));
    write_help(_run,
               "prefixed",
               _run.app().help("outer"));
    write_help(_run,
               "sub",
               sub->help());

    return;
}

// LAMBDA_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_lambda.
const step_fn LAMBDA_STEPS[] =
{
    &lambda_help
};

// LAMBDA_COUNT
//   constant: how many steps LAMBDA_STEPS holds.
const std::size_t LAMBDA_COUNT =
    sizeof(LAMBDA_STEPS) / sizeof(LAMBDA_STEPS[0]);

// 1.5    Steps replaced
//------------------------------------------------------------------------------

/**
 * @brief Opens a step's piece of help with its mark: `<MARK:`.
 *
 * @param[in]     _mark  the step's mark.
 * @param[in,out] _out   the help being built.
 * @return the append's status.
 */
cli::status
mark_open(
    const char*  _mark,
    cli::buffer& _out
)
{
    const std::string opening = std::string("<") + _mark + ":";

    return _out.append(cli::text(opening));
}

/**
 * @brief Closes a step's piece of help with its mark, `:MARK>`, when what
 *        came before was written.
 *
 * @param[in]     _written  the status of what came before.
 * @param[in]     _mark     the step's mark.
 * @param[in,out] _out      the help being built.
 * @return `_written` when it is not ok, else the append's status.
 */
cli::status
mark_close(
    const cli::status& _written,
    const char*        _mark,
    cli::buffer&       _out
)
{
    const std::string closing = std::string(":") + _mark + ">";

    return (_written.ok())
           ? _out.append(cli::text(closing))
           : _written;
}

// marked
//   class: a formatter that replaces three steps, as a subclass of CLI11's
// Formatter replaces its make_* functions: each option's description marked
// after the formatter's own, the usage followed by a note, and a footer of
// its own.
class marked : public cli::formatter
{
public:
    /**
     * @brief Writes an option's description, then a mark.
     *
     * @param[in]     _option  the option.
     * @param[in,out] _out     the help being built.
     * @return the formatter's status, or the append's.
     */
    virtual cli::status
    make_option_desc(
        const cli::option& _option,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        const cli::status written = cli::formatter::make_option_desc(_option,
                                                                     _out);

        return (written.ok())
               ? _out.append(cli::text(" (!)"))
               : written;
    }

    /**
     * @brief Writes the usage, then a note.
     *
     * @param[in]     _command  the command.
     * @param[in]     _name     the name help shows.
     * @param[in,out] _out      the help being built.
     * @return the formatter's status, or the append's.
     */
    virtual cli::status
    make_usage(
        const cli::command& _command,
        const cli::text&    _name,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        const cli::status written = cli::formatter::make_usage(_command,
                                                               _name,
                                                               _out);

        return (written.ok())
               ? _out.append(cli::text("(usage above)\n"))
               : written;
    }

    /**
     * @brief Writes a footer of its own.
     *
     * @param[in]     _command  the command; unused.
     * @param[in,out] _out      the help being built.
     * @return the append's status.
     */
    virtual cli::status
    make_footer(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        (void)_command;

        return _out.append(cli::text("marked footer"));
    }
};

// bracketed
//   class: a formatter that replaces every step it can, each writing the
// formatter's own piece of help between marks naming the step -- H for
// make_help, G make_group, P make_positionals, SS make_subcommands, S
// make_subcommand, E make_expanded, F make_footer, D make_description, U
// make_usage, O make_option, ON make_option_name, OO make_option_opts, OD
// make_option_desc and OU make_option_usage -- so that its help shows which
// step wrote each piece, and which steps each calls.
class bracketed : public cli::formatter
{
public:
    /**
     * @brief The formatter's make_help, between marks.
     *
     * @param[in]     _command  the command.
     * @param[in]     _name     the name help shows.
     * @param[in]     _mode     the mode.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_help(
        const cli::command&   _command,
        const cli::text&      _name,
        cli::help_mode::value _mode,
        cli::buffer&          _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("H",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_help(_command,
                                                      _name,
                                                      _mode,
                                                      _out)
                          : opened,
                          "H",
                          _out);
    }

    /**
     * @brief The formatter's make_group, between marks.
     *
     * @param[in]     _group       the group's name.
     * @param[in]     _positional  whether the group's options are
     *                             positionals.
     * @param[in]     _options     the group's options.
     * @param[in,out] _out         the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_group(
        const cli::text&        _group,
        bool                    _positional,
        const cli::option_list& _options,
        cli::buffer&            _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("G",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_group(_group,
                                                       _positional,
                                                       _options,
                                                       _out)
                          : opened,
                          "G",
                          _out);
    }

    /**
     * @brief The formatter's make_positionals, between marks.
     *
     * @param[in]     _command  the command.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_positionals(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("P",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_positionals(_command,
                                                             _out)
                          : opened,
                          "P",
                          _out);
    }

    /**
     * @brief The formatter's make_subcommands, between marks.
     *
     * @param[in]     _command  the command.
     * @param[in]     _mode     the mode.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_subcommands(
        const cli::command&   _command,
        cli::help_mode::value _mode,
        cli::buffer&          _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("SS",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_subcommands(_command,
                                                             _mode,
                                                             _out)
                          : opened,
                          "SS",
                          _out);
    }

    /**
     * @brief The formatter's make_subcommand, between marks.
     *
     * @param[in]     _command  the subcommand.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_subcommand(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("S",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_subcommand(_command,
                                                            _out)
                          : opened,
                          "S",
                          _out);
    }

    /**
     * @brief The formatter's make_expanded, between marks.
     *
     * @param[in]     _command  the subcommand or option group.
     * @param[in]     _mode     the mode.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_expanded(
        const cli::command&   _command,
        cli::help_mode::value _mode,
        cli::buffer&          _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("E",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_expanded(_command,
                                                          _mode,
                                                          _out)
                          : opened,
                          "E",
                          _out);
    }

    /**
     * @brief The formatter's make_footer, between marks.
     *
     * @param[in]     _command  the command.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_footer(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("F",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_footer(_command,
                                                        _out)
                          : opened,
                          "F",
                          _out);
    }

    /**
     * @brief The formatter's make_description, between marks.
     *
     * @param[in]     _command  the command.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_description(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("D",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_description(_command,
                                                             _out)
                          : opened,
                          "D",
                          _out);
    }

    /**
     * @brief The formatter's make_usage, between marks.
     *
     * @param[in]     _command  the command.
     * @param[in]     _name     the name help shows.
     * @param[in,out] _out      the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_usage(
        const cli::command& _command,
        const cli::text&    _name,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("U",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_usage(_command,
                                                       _name,
                                                       _out)
                          : opened,
                          "U",
                          _out);
    }

    /**
     * @brief The formatter's make_option, between marks.
     *
     * @param[in]     _option      the option.
     * @param[in]     _positional  whether the line is a positional's.
     * @param[in,out] _out         the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_option(
        const cli::option& _option,
        bool               _positional,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("O",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_option(_option,
                                                        _positional,
                                                        _out)
                          : opened,
                          "O",
                          _out);
    }

    /**
     * @brief The formatter's make_option_name, between marks.
     *
     * @param[in]     _option      the option.
     * @param[in]     _positional  whether the name is a positional's.
     * @param[in,out] _out         the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_option_name(
        const cli::option& _option,
        bool               _positional,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("ON",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_option_name(_option,
                                                             _positional,
                                                             _out)
                          : opened,
                          "ON",
                          _out);
    }

    /**
     * @brief The formatter's make_option_opts, between marks.
     *
     * @param[in]     _option  the option.
     * @param[in,out] _out     the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_option_opts(
        const cli::option& _option,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("OO",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_option_opts(_option,
                                                             _out)
                          : opened,
                          "OO",
                          _out);
    }

    /**
     * @brief The formatter's make_option_desc, between marks.
     *
     * @param[in]     _option  the option.
     * @param[in,out] _out     the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_option_desc(
        const cli::option& _option,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("OD",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_option_desc(_option,
                                                             _out)
                          : opened,
                          "OD",
                          _out);
    }

    /**
     * @brief The formatter's make_option_usage, between marks.
     *
     * @param[in]     _option  the option.
     * @param[in,out] _out     the help being built.
     * @return the status of the first part that failed, or ok.
     */
    virtual cli::status
    make_option_usage(
        const cli::option& _option,
        cli::buffer&       _out
    ) const D_OVERRIDE
    {
        const cli::status opened = mark_open("OU",
                                             _out);

        return mark_close((opened.ok())
                          ? cli::formatter::make_option_usage(_option,
                                                              _out)
                          : opened,
                          "OU",
                          _out);
    }
};

/**
 * @brief STEPS 1: the program's help through a formatter replacing three
 *        steps, the root's and a subcommand's.
 *
 * @param[in,out] _run  the step's run.
 */
void
steps_marked(
    run& _run
)
{
    static marked replaced;

    _run.app().formatter(replaced);

    const cli::command sub = define_program(_run);

    write_help(_run,
               "marked",
               _run.app().help());
    write_help(_run,
               "sub",
               sub->help());

    return;
}

/**
 * @brief STEPS 2: the program's help, with an option group, through a
 *        formatter replacing every step: the root's, expanded and a
 *        subcommand's.
 *
 * @param[in,out] _run  the step's run.
 */
void
steps_bracketed(
    run& _run
)
{
    static bracketed replaced;

    _run.app().formatter(replaced);

    const cli::command sub   = define_program(_run);
    cli::option_group  group = _run.app().add_option_group("Extra",
                                                           "extra options");

    (void)group->add_flag("--extra",
                          "an extra flag");
    write_help(_run,
               "bracketed",
               _run.app().help());
    write_help(_run,
               "all",
               _run.app().help("",
                               cli::help_mode::all));
    write_help(_run,
               "sub",
               sub->help());

    return;
}

// STEPS_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_steps.
const step_fn STEPS_STEPS[] =
{
    &steps_marked,
    &steps_bracketed
};

// STEPS_COUNT
//   constant: how many steps STEPS_STEPS holds.
const std::size_t STEPS_COUNT = sizeof(STEPS_STEPS) / sizeof(STEPS_STEPS[0]);

// 1.6    Copies
//------------------------------------------------------------------------------

/**
 * @brief COPIES 1: formatters copied by construction and assignment, with
 *        their settings and labels, each changed apart from the other; a
 *        copy of a formatter whose steps are replaced, which is a
 *        formatter, its steps the formatter's own.
 *
 * @param[in,out] _run  the step's run.
 */
void
copies_made(
    run& _run
)
{
    static cli::formatter original;
    static cli::formatter copied;
    static cli::formatter assigned;
    static marked         replaced;
    static cli::formatter sliced;

    original.column_width(25u);
    original.label("REQUIRED",
                   "NEEDED");
    copied = cli::formatter(original);
    assigned = original;
    copied.column_width(35u);
    assigned.label("REQUIRED",
                   "WANTED");
    write_formatter(_run,
                    "original",
                    original);
    write_formatter(_run,
                    "copied",
                    copied);
    write_formatter(_run,
                    "assigned",
                    assigned);
    replaced.column_width(15u);
    sliced = replaced;
    _run.app().formatter(sliced);
    (void)_run.app().add_option("-x",
                                "an option");
    write_help(_run,
               "sliced",
               _run.app().help());

    return;
}

// COPIES_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_copies.
const step_fn COPIES_STEPS[] =
{
    &copies_made
};

// COPIES_COUNT
//   constant: how many steps COPIES_STEPS holds.
const std::size_t COPIES_COUNT =
    sizeof(COPIES_STEPS) / sizeof(COPIES_STEPS[0]);

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: a step that fails, which returns its status;
// help's longest line and the terminal's width; and the departures. Each is
// a step written here, through the whole face.

namespace
{

// 2.1    A step that fails
//------------------------------------------------------------------------------

// failing
//   class: a formatter whose description step writes part of its text and
// then fails.
class failing : public cli::formatter
{
public:
    /**
     * @brief Writes part of a description, then fails.
     *
     * @param[in]     _command  the command; unused.
     * @param[in,out] _out      the help being built.
     * @return status_code::conversion_error.
     */
    virtual cli::status
    make_description(
        const cli::command& _command,
        cli::buffer&        _out
    ) const D_OVERRIDE
    {
        (void)_command;
        (void)_out.append(cli::text("partial"));

        return cli::status_code::conversion_error;
    }
};

/**
 * @brief FAILED 1: help through a formatter whose step fails: as a string,
 *        empty; into a sink, the step's status, and nothing written.
 *
 * @param[in,out] _run  the step's run.
 */
void
failed_step(
    run& _run
)
{
    static failing broken;
    std::string    written;

    _run.app().formatter(broken);
    (void)define_program(_run);

    const std::string as_string = _run.app().help();
    const cli::status into_sink = _run.app().help(cli::string_sink(written));

    _run.note("string=[" + escape(as_string) + "] sink=" +
              into_sink.get_name() + " written=[" + escape(written) + "]");

    return;
}

// FAILED_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_failed.
const step_fn FAILED_STEPS[] =
{
    &failed_step
};

// FAILED_COUNT
//   constant: how many steps FAILED_STEPS holds.
const std::size_t FAILED_COUNT =
    sizeof(FAILED_STEPS) / sizeof(FAILED_STEPS[0]);

// 2.2    Lines
//------------------------------------------------------------------------------

/**
 * @brief Writes a formatter's line settings on one line.
 *
 * @param[in,out] _run        the run.
 * @param[in]     _label      what the line names the formatter by.
 * @param[in]     _formatter  the formatter.
 */
void
write_lines(
    run&                  _run,
    const char*           _label,
    const cli::formatter& _formatter
)
{
    _run.note(std::string(_label) +
              ": line=" +
              number(static_cast<unsigned long>(_formatter.get_line_width())) +
              " terminal=" +
              (_formatter.is_terminal_width_enabled() ? "1" : "0"));

    return;
}

/**
 * @brief LINES 1: help's longest line, set and read back, and the program's
 *        help within it; the terminal's width, asked for and read back.
 *
 * @param[in,out] _run  the step's run.
 */
void
lines_limited(
    run& _run
)
{
    static cli::formatter limited;

    write_lines(_run,
                "made",
                limited);
    limited.line_width(40u)
           .terminal_width();
    write_lines(_run,
                "set",
                limited);
    limited.terminal_width(false);
    write_lines(_run,
                "set again",
                limited);
    _run.app().formatter(limited);
    (void)define_program(_run);
    write_help(_run,
               "forty",
               _run.app().help());

    return;
}

// LINES_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_lines.
const step_fn LINES_STEPS[] =
{
    &lines_limited
};

// LINES_COUNT
//   constant: how many steps LINES_STEPS holds.
const std::size_t LINES_COUNT = sizeof(LINES_STEPS) / sizeof(LINES_STEPS[0]);

// 2.3    Departures
//------------------------------------------------------------------------------

/**
 * @brief DEPARTED 1: a subcommand made before the root is given a
 *        formatter, which follows the root's; then one of its own.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_followed(
    run& _run
)
{
    static cli::formatter narrow;
    static cli::formatter own;
    cli::command          sub = _run.app().add_subcommand("sub",
                                                          "a subcommand");

    (void)sub->add_option("--inner",
                          "an option of the subcommand, described at length");
    narrow.column_width(12u);
    own.column_width(40u);
    _run.app().formatter(narrow);
    write_help(_run,
               "followed",
               sub->help());
    sub->formatter(own);
    write_help(_run,
               "own",
               sub->help());

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_formatter_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_followed
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

}  // namespace

//==============================================================================
// 3.  CLI11'S FORMATTER
//==============================================================================

/*
d_tests_arg_cpp_formatter_settings
  A formatter's settings and labels, as CLI11's Formatter has them.
  Tests the following:
  - the settings a formatter is made with
  - each set and read back; labels given, replaced and read, an unlabelled
    key its own label
*/
bool
d_tests_arg_cpp_formatter_settings()
{
    return steps_hold("SETTINGS",
                      SETTINGS_STEPS,
                      SETTINGS_COUNT,
                      SETTINGS);
}

/*
d_tests_arg_cpp_formatter_help
  One program's help under formatters of each setting, as CLI11 writes it.
  Tests the following:
  - the command's own formatter and a new one given to it; expanded,
    prefixed and a subcommand's help
  - narrow columns; narrow and unformatted paragraphs
  - no defaults, type names or flags' values; long options aligned
  - every label help writes, the program named and unnamed
*/
bool
d_tests_arg_cpp_formatter_help()
{
    return steps_hold("HELP",
                      HELP_STEPS,
                      HELP_COUNT,
                      HELP);
}

/*
d_tests_arg_cpp_formatter_lambda
  Help a function writes, as CLI11's FormatterLambda writes it.
  Tests the following:
  - the root's help, expanded and prefixed, and a subcommand's
*/
bool
d_tests_arg_cpp_formatter_lambda()
{
    return steps_hold("LAMBDA",
                      LAMBDA_STEPS,
                      LAMBDA_COUNT,
                      LAMBDA);
}

/*
d_tests_arg_cpp_formatter_steps
  Steps replaced by classes deriving from cli::formatter, as CLI11's
  subclasses of its Formatter replace them.
  Tests the following:
  - three steps replaced, two calling the formatter's own first, the root's
    help and a subcommand's
  - every step replaced, its piece between marks: the root's help, an
    option group's within it, expanded help and a subcommand's
*/
bool
d_tests_arg_cpp_formatter_steps()
{
    return steps_hold("STEPS",
                      STEPS_STEPS,
                      STEPS_COUNT,
                      STEPS);
}

/*
d_tests_arg_cpp_formatter_copies
  Formatters copied, as CLI11's Formatter is.
  Tests the following:
  - by construction and assignment, settings and labels copied, each
    changed apart
  - a copy of a formatter whose steps are replaced: a formatter, with the
    formatter's own steps
*/
bool
d_tests_arg_cpp_formatter_copies()
{
    return steps_hold("COPIES",
                      COPIES_STEPS,
                      COPIES_COUNT,
                      COPIES);
}

//==============================================================================
// 4.  THE FACE'S OWN
//==============================================================================

/*
d_tests_arg_cpp_formatter_failed
  A step that fails, which ends help with its status.
  Tests the following:
  - help as a string empty; into a sink, the step's status, and nothing
    written
*/
bool
d_tests_arg_cpp_formatter_failed()
{
    return steps_hold("FAILED",
                      FAILED_STEPS,
                      FAILED_COUNT,
                      FAILED);
}

/*
d_tests_arg_cpp_formatter_lines
  Help's longest line and the terminal's width, the module's own settings.
  Tests the following:
  - as made, and set in a chain and read back
  - the program's help within forty columns
*/
bool
d_tests_arg_cpp_formatter_lines()
{
    return steps_hold("LINES",
                      LINES_STEPS,
                      LINES_COUNT,
                      LINES);
}

//==============================================================================
// 5.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_formatter_departures
  Where the face's formatters depart from CLI11's (ISSUES 18).
  Tests the following:
  - a subcommand made before the root's formatter is given follows it; one
    of its own then replaces it
*/
bool
d_tests_arg_cpp_formatter_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_formatter
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_formatter[] =
{
    { "cpp_formatter_settings",
      d_tests_arg_cpp_formatter_settings },
    { "cpp_formatter_help",
      d_tests_arg_cpp_formatter_help },
    { "cpp_formatter_lambda",
      d_tests_arg_cpp_formatter_lambda },
    { "cpp_formatter_steps",
      d_tests_arg_cpp_formatter_steps },
    { "cpp_formatter_copies",
      d_tests_arg_cpp_formatter_copies },
    { "cpp_formatter_failed",
      d_tests_arg_cpp_formatter_failed },
    { "cpp_formatter_lines",
      d_tests_arg_cpp_formatter_lines },
    { "cpp_formatter_departures",
      d_tests_arg_cpp_formatter_departures }
};

// d_tests_arg_cpp_formatter_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_formatter_count =
    sizeof(d_tests_arg_cpp_formatter) / sizeof(d_tests_arg_cpp_formatter[0]);
