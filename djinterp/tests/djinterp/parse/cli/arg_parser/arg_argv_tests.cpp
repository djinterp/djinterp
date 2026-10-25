/*******************************************************************************
* djinterp [parse]                                            arg_argv_tests.cpp
*
* The argument parser's C++ argv suite's sections.
*   The sections held to CLI11 run what the CASES section defines: lines
* split by CLI11's rules, with and without Windows-style options, after a
* word already held; a program's name found at the start of a line among
* files made for it; and text made UTF-8 and made wide. tools/cppref/
* cppref.py compiles that section against CLI11 2.7.2, through the face's
* names for CLI11's (cli11_shim.hpp, whose split_line is App::parse's
* splitting of a string, and whose split_program_name, narrow and widen are
* CLI11's own), and the transcripts below are what CLI11 wrote; so the CASES
* section is written in the part of the face CLI11 shares -- no status
* taken from a call, a failure read from the description it writes. The
* sections that are the module's own are written apart, after it, each
* saying why CLI11 cannot write its transcript.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_argv_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
#include "./arg_argv_tests.hpp"  // corresponding header
// std
#include <cstddef>   // std::size_t
#include <sstream>   // std::ostringstream
#include <string>    // std::string, std::wstring
#include <utility>   // std::pair
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
using cli::testing::number;
using cli::testing::scratch_directory;
using cli::testing::steps_hold;

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// SPLIT
//   constant: what the steps of d_tests_arg_cpp_argv_split write, as CLI11
// writes them.
const char SPLIT[] =
    "[] 0: 1 [held]\n"
    "[  a   b  ] 0: 3 [held] [a] [b]\n"
    "[--name=\"a b\" c] 0: 4 [held] [--name] [a b] [c]\n"
    "[--name='a b'] 0: 3 [held] [--name] [a b]\n"
    "[--name=`a b`] 0: 3 [held] [--name] [a b]\n"
    "[-n=\"a b\"] 0: 3 [held] [-n] [a b]\n"
    "[x=\"a b\"] 0: 3 [held] [x=\"a] [b\"]\n"
    "[\"a b\" c] 0: 3 [held] [a b] [c]\n"
    "[\"a \\\\\"b\\\\\" c\"] 0: 2 [held] [a \"b\" c]\n"
    "[\"a\\\\nb\" 'a\\\\nb'] 0: 3 [held] [a\\nb] [a\\\\nb]\n"
    "[\"\\\\u0041\\\\U0001F600\\\\u00e9\"] 0: 2 [held] "
    "[A\xf0\x9f\x98\x80\xc3\xa9]\n"
    "[[a b] c] 0: 3 [held] [[a b]] [c]\n"
    "[(a (b) c) d] 0: 3 [held] [(a (b) c)] [d]\n"
    "[{a {b} c}] 0: 2 [held] [{a {b} c}]\n"
    "[\"a\"b 'c'd] 0: 5 [held] [a] [b] [c] [d]\n"
    "[\"unclosed words] 0: 2 [held] [\"unclosed words]\n"
    "[a \"\" b] 0: 4 [held] [a] [] [b]\n"
    "[--a=\"b\" --c=\"d e\"] 0: 5 [held] [--a] [b] [--c] [d e]\n"
    "[a\\nb\\tc] 0: 4 [held] [a] [b] [c]\n"
    "[\"\\\\q\"] 0: 1 [held] error=[unrecognized escape sequence \\\\q in "
    "\\\\q]\n"
    "[\"\\\\uD800\"] 0: 1 [held] error=[[0xD800, 0xDFFF] are not valid code "
    "points.]\n"
    "[/name:\"a b\" c] 0: 4 [held] [/name:\"a] [b\"] [c]\n"
    "[/name:\"a b\" c] 2: 4 [held] [/name] [a b] [c]\n"
    "[/n:'a b' -m=\"c d\"] 0: 5 [held] [/n:'a] [b'] [-m] [c d]\n"
    "[/n:'a b' -m=\"c d\"] 2: 5 [held] [/n] [a b] [-m] [c d]\n"
    "[-n:\"a b\"] 0: 3 [held] [-n:\"a] [b\"]\n"
    "[-n:\"a b\"] 2: 3 [held] [-n:\"a] [b\"]\n"
    "[\"my prog\" a] 1: 3 [held] [my prog] [a]\n";

// PROGRAM
//   constant: what the steps of d_tests_arg_cpp_argv_program write, as CLI11
// writes them.
const char PROGRAM[] =
    "[prog a b] -> [prog] [a b]\n"
    "[my prog a b] -> [my prog] [a b]\n"
    "[my prog] -> [my prog] []\n"
    "[\"quoted name\" rest] -> [quoted name] [rest]\n"
    "[\"escaped \\\\\" quote\" rest] -> [escaped \" quote] [rest]\n"
    "['unclosed name] -> ['unclosed] [name]\n"
    "[dir a] -> [dir] [a]\n"
    "[missing a b] -> [missing] [a b]\n"
    "[  prog   ] -> [prog] []\n"
    "[] -> [] []\n";

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// ENCODING
//   constant: what the steps of d_tests_arg_cpp_argv_encoding write, as
// CLI11 writes them.
const char ENCODING[] =
    "one byte [plain]\n"
    "two bytes [caf\xc3\xa9]\n"
    "three bytes [\xe2\x82\xac]\n"
    "four bytes [\xf0\x9f\x98\x80]\n"
    "empty []\n"
    "with a nul [a\\x00b]\n"
    "wstring [w\xc3\xa9]\n"
    "one byte [70 6c 61 69 6e]\n"
    "two bytes [63 61 66 e9]\n"
    "three bytes [20ac]\n"
    "four bytes [1f600]\n"
    "empty []\n"
    "with a nul [61 0 62]\n"
    "string [77 e9]\n";

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// RULES
//   constant: what the steps of d_tests_arg_cpp_argv_rules write: the
// module's own, as CLI11 splits by its own rules alone, and a split here
// returns a status.
const char RULES[] =
    "posix 0 [a 'b c' \"d \\\\\"e\\\\\"\" f\\\\ g]: OK 5 [held] [a] [b c] [d "
    "\"e\"] [f g] error=[before: ]\n"
    "posix 0 [x\\\\\\ny 'z\\\\n']: OK 3 [held] [xy] [z\\\\n] error=[before: ]\n"
    "posix 0 [a 'unclosed]: InvalidError 1 [held] error=[before: a single "
    "quote is not closed]\n"
    "windows 0 [a \"b c\" d\\\\\\\\\\\\\"e f\\\\g \"h\"\"i\"]: OK 6 [held] [a] "
    "[b c] [d\\\\\"e] [f\\\\g] [h\"i] error=[before: ]\n"
    "windows 1 [\"C:\\\\Program Files\\\\x.exe\" \"a\\\\\\\\\" b]: OK 4 [held] "
    "[C:\\\\Program Files\\\\x.exe] [a\\\\] [b] error=[before: ]\n"
    "windows 0 [a\\nb]: OK 2 [held] [a\\nb] error=[before: ]\n"
    "windows 4 [a\\nb]: OK 3 [held] [a] [b] error=[before: ]\n"
    "gnu 0 [a 'b c' \"d\" e\\\\ f]: OK 5 [held] [a] [b c] [d] [e f] "
    "error=[before: ]\n"
    "gnu 0 [a 'unclosed rest]: OK 3 [held] [a] [unclosed rest] error=[before: "
    "]\n"
    "native 0 [a \"b c\" d]: OK 4 [held] [a] [b c] [d] error=[before: ]\n"
    "native_response 0 [a \"b c\" d]: OK 4 [held] [a] [b c] [d] error=[before: "
    "]\n";

// QUOTED
//   constant: what the steps of d_tests_arg_cpp_argv_quoted write: the
// module's own, as CLI11 quotes no word for a command line.
const char QUOTED[] =
    "posix [plain] -> [plain] whole\n"
    "posix [two words] -> ['two words'] whole\n"
    "posix [it's] -> ['it'\\\\''s'] whole\n"
    "posix [say \"hi\"] -> ['say \"hi\"'] whole\n"
    "posix [back\\\\slash] -> ['back\\\\slash'] whole\n"
    "posix [] -> [''] whole\n"
    "posix [trailing\\\\] -> ['trailing\\\\'] whole\n"
    "posix [tab\\there] -> ['tab\\there'] whole\n"
    "posix [$HOME] -> ['$HOME'] whole\n"
    "windows [plain] -> [plain] whole\n"
    "windows [two words] -> [\"two words\"] whole\n"
    "windows [it's] -> [it's] whole\n"
    "windows [say \"hi\"] -> [\"say \\\\\"hi\\\\\"\"] whole\n"
    "windows [back\\\\slash] -> [back\\\\slash] whole\n"
    "windows [] -> [\"\"] whole\n"
    "windows [trailing\\\\] -> [trailing\\\\] whole\n"
    "windows [tab\\there] -> [\"tab\\there\"] whole\n"
    "windows [$HOME] -> [$HOME] whole\n"
    "cli11 [plain] -> [plain] whole\n"
    "cli11 [two words] -> [\"two words\"] whole\n"
    "cli11 [it's] -> [it's] whole\n"
    "cli11 [say \"hi\"] -> [\"say \\\\\"hi\\\\\"\"] whole\n"
    "cli11 [back\\\\slash] -> [back\\\\slash] whole\n"
    "cli11 [] -> [\"\"] whole\n"
    "cli11 [trailing\\\\] -> [trailing\\\\] whole\n"
    "cli11 [tab\\there] -> [\"tab\\\\there\"] whole\n"
    "cli11 [$HOME] -> [$HOME] whole\n"
    "gnu [plain] -> [plain] whole\n"
    "gnu [two words] -> [two\\\\ words] whole\n"
    "gnu [it's] -> [it\\\\'s] whole\n"
    "gnu [say \"hi\"] -> [say\\\\ \\\\\"hi\\\\\"] whole\n"
    "gnu [back\\\\slash] -> [back\\\\\\\\slash] whole\n"
    "gnu [] -> [''] whole\n"
    "gnu [trailing\\\\] -> [trailing\\\\\\\\] whole\n"
    "gnu [tab\\there] -> [tab\\\\\\there] whole\n"
    "gnu [$HOME] -> [$HOME] whole\n";

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// RESPONSE
//   constant: what the steps of d_tests_arg_cpp_argv_response write: the
// module's own, as CLI11 reads no response file.
const char RESPONSE[] =
    "posix args.rsp: OK 5 [held] [a] [b c] [d e] [f g] error=[]\n"
    "windows args.rsp: OK 7 [held] [a] ['b] [c'] [d e] [f\\\\] [g] error=[]\n"
    "cli11 args.rsp: OK 6 [held] [a] [b c] [d e] [f\\\\] [g] error=[]\n"
    "gnu args.rsp: OK 5 [held] [a] [b c] [d e] [f g] error=[]\n"
    "gnu bom.rsp: OK 3 [held] [x] [y z] error=[]\n"
    "native_response neutral.rsp: OK 4 [held] [a] [b c] [d] error=[]\n"
    "posix missing.rsp: FileError 1 [held] error=[missing.rsp was not readable "
    "(missing?)]\n";

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// WIDE
//   constant: what the steps of d_tests_arg_cpp_argv_wide write: the
// module's own, as widen() here has a form that returns a status (ISSUES
// 45), and narrow() takes a NULL C string as empty text.
const char WIDE[] =
    "converted: OK [63 61 66 e9]\n"
    "refused: ConversionError [68 65 6c 64]\n"
    "cut short [63 61 66]\n"
    "null []\n";

// DEPARTED
//   constant: what the steps of d_tests_arg_cpp_argv_departures write: the
// module's own, where the face departs from CLI11: CLI11's narrow() and
// widen() throw std::range_error from std::wstring_convert --
// `wstring_convert::to_bytes` for an unpaired surrogate,
// `wstring_convert::from_bytes` for UTF-8 that is not well-formed -- or
// std::runtime_error where the C++ library has no <codecvt>, where here an
// unpaired surrogate is U+FFFD and such UTF-8 is the empty text, which the
// status form of widen() tells from text that is empty (ISSUES 62).
const char DEPARTED[] =
    "unpaired [\xef\xbf\xbd" "a]\n"
    "stray byte []\n"
    "overlong []\n"
    "surrogate []\n";

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 1.  CASES
//==============================================================================
// What the sections held to CLI11 define, in the part of the face CLI11
// shares; tools/cppref/cppref.py compiles this section against CLI11.

// 1.1    Writers
//------------------------------------------------------------------------------

/**
 * @brief Words as a transcript writes them: how many, then each between
 *        brackets, escaped.
 *
 * @param[in] _words  the words.
 * @return the words, written.
 */
std::string
written_words(
    const std::vector<std::string>& _words
)
{
    std::string written = number(static_cast<unsigned long>(_words.size()));

    for (std::size_t index = 0u; index < _words.size(); ++index)
    {
        written += " [" + escape(_words[index]) + "]";
    }

    return written;
}

/**
 * @brief Splits a line by CLI11's rules, after a word already held, and
 *        writes the words, and the description of a failure.
 *
 * @param[in,out] _run    the run.
 * @param[in]     _line   the line.
 * @param[in]     _flags  split_flags values, combined.
 */
void
write_split(
    run&         _run,
    const char*  _line,
    unsigned int _flags
)
{
    std::vector<std::string> words(1u,
                                   std::string("held"));
    std::string              error;

    (void)cli::split_line(_line,
                          words,
                          cli::quoting::cli11,
                          _flags,
                          &error);
    _run.note("[" + escape(_line) + "] " +
              number(static_cast<unsigned long>(_flags)) + ": " +
              written_words(words) +
              ((error.empty()) ? std::string()
                               : " error=[" + escape(error) + "]"));

    return;
}

// 1.2    Lines
//------------------------------------------------------------------------------

/**
 * @brief SPLIT 1: lines split by CLI11's rules: spaces, quotes of each
 *        kind around a value after `=`, escapes, brackets, quotes
 *        unclosed and empty, other whitespace; escapes CLI11 refuses.
 *
 * @param[in,out] _run  the step's run.
 */
void
split_lines(
    run& _run
)
{
    static const char* const LINES[] =
    {
        "",
        "  a   b  ",
        "--name=\"a b\" c",
        "--name='a b'",
        "--name=`a b`",
        "-n=\"a b\"",
        "x=\"a b\"",
        "\"a b\" c",
        "\"a \\\"b\\\" c\"",
        "\"a\\nb\" 'a\\nb'",
        "\"\\u0041\\U0001F600\\u00e9\"",
        "[a b] c",
        "(a (b) c) d",
        "{a {b} c}",
        "\"a\"b 'c'd",
        "\"unclosed words",
        "a \"\" b",
        "--a=\"b\" --c=\"d e\"",
        "a\nb\tc",
        "\"\\q\"",
        "\"\\uD800\""
    };

    for (std::size_t index = 0u;
         index < (sizeof(LINES) / sizeof(LINES[0]));
         ++index)
    {
        write_split(_run,
                    LINES[index],
                    0u);
    }

    return;
}

/**
 * @brief SPLIT 2: lines split with Windows-style options and without: `:`
 *        before a quote after a `/name` ends a word only with them; and a
 *        line with a program's name, which CLI11's rules read as any
 *        word.
 *
 * @param[in,out] _run  the step's run.
 */
void
split_switches(
    run& _run
)
{
    static const char* const LINES[] =
    {
        "/name:\"a b\" c",
        "/n:'a b' -m=\"c d\"",
        "-n:\"a b\""
    };

    for (std::size_t index = 0u;
         index < (sizeof(LINES) / sizeof(LINES[0]));
         ++index)
    {
        write_split(_run,
                    LINES[index],
                    0u);
        write_split(_run,
                    LINES[index],
                    cli::split_flags::windows_style);
    }

    write_split(_run,
                "\"my prog\" a",
                cli::split_flags::program_name);

    return;
}

// SPLIT_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_split.
const step_fn SPLIT_STEPS[] =
{
    &split_lines,
    &split_switches
};

// SPLIT_COUNT
//   constant: how many steps SPLIT_STEPS holds.
const std::size_t SPLIT_COUNT = sizeof(SPLIT_STEPS) / sizeof(SPLIT_STEPS[0]);

// 1.3    Program names
//------------------------------------------------------------------------------

/**
 * @brief PROGRAM 1: a program's name found at the start of a line among
 *        files made for it -- one whose name holds a space, and a folder,
 *        which names no program -- or quoted, or the first word.
 *
 * @param[in,out] _run  the step's run.
 */
void
program_names(
    run& _run
)
{
    static const file_entry  FILES[] =
    {
        { "prog",      "" },
        { "my prog",   "" },
        { D_NULLPTR,   D_NULLPTR }
    };
    static const char* const LINES[] =
    {
        "prog a b",
        "my prog a b",
        "my prog",
        "\"quoted name\" rest",
        "\"escaped \\\" quote\" rest",
        "'unclosed name",
        "dir a",
        "missing a b",
        "  prog   ",
        ""
    };
    scratch_directory        files(FILES);

    // the folder beside the files
    if ( (!files.make_folder("dir")) ||
         (!files.ok()) )
    {
        _run.note("the scratch directory was not made");
    }

    for (std::size_t index = 0u;
         index < (sizeof(LINES) / sizeof(LINES[0]));
         ++index)
    {
        const std::pair<std::string, std::string> split =
            cli::split_program_name(LINES[index]);

        _run.note("[" + escape(LINES[index]) + "] -> [" +
                  escape(split.first) + "] [" + escape(split.second) + "]");
    }

    return;
}

// PROGRAM_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_program.
const step_fn PROGRAM_STEPS[] =
{
    &program_names
};

// PROGRAM_COUNT
//   constant: how many steps PROGRAM_STEPS holds.
const std::size_t PROGRAM_COUNT =
    sizeof(PROGRAM_STEPS) / sizeof(PROGRAM_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 1.4    Encoding
//------------------------------------------------------------------------------

/**
 * @brief Wide text as code points in hexadecimal, a surrogate pair read
 *        as the one it encodes where `wchar_t` holds UTF-16, so that the
 *        text reads the same whatever the width of `wchar_t`.
 *
 * @param[in] _text  the wide text.
 * @return the code points, separated by spaces.
 */
std::string
code_points(
    const std::wstring& _text
)
{
    const bool         utf16 = (sizeof(wchar_t) == 2u);
    std::ostringstream written;

    written << std::hex;

    for (std::size_t index = 0u; index < _text.size(); ++index)
    {
        const bool    first = (index == 0u);
        unsigned long point = static_cast<unsigned long>(_text[index]);

        // a surrogate pair, where wchar_t holds UTF-16
        if ( (utf16)                          &&
             (point >= 0xD800ul)              &&
             (point <= 0xDBFFul)              &&
             ((index + 1u) < _text.size()) )
        {
            const unsigned long low =
                static_cast<unsigned long>(_text[index + 1u]);

            // a low surrogate after the high one
            if ( (low >= 0xDC00ul) &&
                 (low <= 0xDFFFul) )
            {
                point = 0x10000ul + ((point - 0xD800ul) << 10) +
                        (low - 0xDC00ul);
                ++index;
            }
        }

        written << (first ? "" : " ") << point;
    }

    return written.str();
}

/**
 * @brief ENCODING 1: wide text made UTF-8: characters of each length, the
 *        empty text, a NUL inside text of a length given, and a
 *        std::wstring.
 *
 * @param[in,out] _run  the step's run.
 */
void
encoding_narrow(
    run& _run
)
{
    static const wchar_t WITH_NUL[] = { L'a', L'\0', L'b' };

    _run.note("one byte [" + escape(cli::narrow(L"plain")) + "]");
    _run.note("two bytes [" + escape(cli::narrow(L"caf\u00e9")) + "]");
    _run.note("three bytes [" + escape(cli::narrow(L"\u20ac")) + "]");
    _run.note("four bytes [" + escape(cli::narrow(L"\U0001F600")) + "]");
    _run.note("empty [" + escape(cli::narrow(L"")) + "]");
    _run.note("with a nul [" +
              escape(cli::narrow(WITH_NUL,
                                 3u)) + "]");
    _run.note("wstring [" +
              escape(cli::narrow(std::wstring(L"w\u00e9"))) + "]");

    return;
}

/**
 * @brief ENCODING 2: UTF-8 made wide: characters of each length, the empty
 *        text, a NUL inside text of a length given, and a std::string.
 *
 * @param[in,out] _run  the step's run.
 */
void
encoding_widen(
    run& _run
)
{
    _run.note("one byte [" + code_points(cli::widen("plain")) + "]");
    _run.note("two bytes [" + code_points(cli::widen("caf\xc3\xa9")) + "]");
    _run.note("three bytes [" + code_points(cli::widen("\xe2\x82\xac")) +
              "]");
    _run.note("four bytes [" +
              code_points(cli::widen("\xf0\x9f\x98\x80")) + "]");
    _run.note("empty [" + code_points(cli::widen("")) + "]");
    _run.note("with a nul [" +
              code_points(cli::widen("a\0b",
                                     3u)) + "]");
    _run.note("string [" +
              code_points(cli::widen(std::string("w\xc3\xa9"))) + "]");

    return;
}

// ENCODING_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_encoding.
const step_fn ENCODING_STEPS[] =
{
    &encoding_narrow,
    &encoding_widen
};

// ENCODING_COUNT
//   constant: how many steps ENCODING_STEPS holds.
const std::size_t ENCODING_COUNT =
    sizeof(ENCODING_STEPS) / sizeof(ENCODING_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

}  // namespace

//==============================================================================
// 2.  THE MODULE'S OWN
//==============================================================================
// What CLI11 cannot write: the rules beside its own, words quoted for them,
// response files, the status form of widen(), and the departures. Each is a
// step written here, through the whole face.

namespace
{

// 2.1    Rules
//------------------------------------------------------------------------------

/**
 * @brief Splits a line by a set of rules, after a word already held and
 *        into a description already begun, and writes the status, the
 *        words and the description.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what the line names the rules by.
 * @param[in]     _quoting  the rules.
 * @param[in]     _flags    split_flags values, combined.
 * @param[in]     _line     the line.
 */
void
write_rule(
    run&                _run,
    const char*         _label,
    cli::quoting::value _quoting,
    unsigned int        _flags,
    const char*         _line
)
{
    std::vector<std::string> words(1u,
                                   std::string("held"));
    std::string              error("before: ");
    const cli::status        split = cli::split_line(_line,
                                                     words,
                                                     _quoting,
                                                     _flags,
                                                     &error);

    _run.note(std::string(_label) + " " +
              number(static_cast<unsigned long>(_flags)) + " [" +
              escape(_line) + "]: " + split.get_name() + " " +
              written_words(words) + " error=[" + escape(error) + "]");

    return;
}

// split_row
//   struct: a line rules_split splits: what the line names its rules by, the
// rules, split_flags values combined, and the line.
struct split_row
{
    const char*         label;
    cli::quoting::value quoting;
    unsigned int        flags;
    const char*         line;
};

// SPLIT_ROWS
//   constant: the lines of rules_split, in the order they are written.
const split_row SPLIT_ROWS[] =
{
    { "posix", cli::quoting::posix, 0u,
      "a 'b c' \"d \\\"e\\\"\" f\\ g" },
    { "posix", cli::quoting::posix, 0u,
      "x\\\ny 'z\\n'" },
    { "posix", cli::quoting::posix, 0u,
      "a 'unclosed" },
    { "windows", cli::quoting::windows, 0u,
      "a \"b c\" d\\\\\\\"e f\\g \"h\"\"i\"" },
    { "windows", cli::quoting::windows, cli::split_flags::program_name,
      "\"C:\\Program Files\\x.exe\" \"a\\\\\" b" },
    { "windows", cli::quoting::windows, 0u,
      "a\nb" },
    { "windows", cli::quoting::windows, cli::split_flags::line_breaks,
      "a\nb" },
    { "gnu", cli::quoting::gnu, 0u,
      "a 'b c' \"d\" e\\ f" },
    { "gnu", cli::quoting::gnu, 0u,
      "a 'unclosed rest" },
    { "native", cli::quoting::native, 0u,
      "a \"b c\" d" },
    { "native_response", cli::quoting::native_response, 0u,
      "a \"b c\" d" }
};

// SPLIT_ROW_COUNT
//   constant: how many lines SPLIT_ROWS holds.
const std::size_t SPLIT_ROW_COUNT = sizeof(SPLIT_ROWS) / sizeof(SPLIT_ROWS[0]);

/**
 * @brief RULES 1: lines split by POSIX's rules, the C runtime's -- with a
 *        program's name and with line breaks -- and GCC's; the platform's
 *        two, by lines all of them split alike; text each refuses.
 *
 * @param[in,out] _run  the step's run.
 */
void
rules_split(
    run& _run
)
{
    // each line by its rules
    for (std::size_t index = 0u; index < SPLIT_ROW_COUNT; ++index)
    {
        write_rule(_run,
                   SPLIT_ROWS[index].label,
                   SPLIT_ROWS[index].quoting,
                   SPLIT_ROWS[index].flags,
                   SPLIT_ROWS[index].line);
    }

    return;
}

// RULES_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_rules.
const step_fn RULES_STEPS[] =
{
    &rules_split
};

// RULES_COUNT
//   constant: how many steps RULES_STEPS holds.
const std::size_t RULES_COUNT = sizeof(RULES_STEPS) / sizeof(RULES_STEPS[0]);

// 2.2    Words quoted
//------------------------------------------------------------------------------

/**
 * @brief QUOTED 1: words quoted by each set of rules, and split back by the
 *        same rules, each one argument again.
 *
 * @param[in,out] _run  the step's run.
 */
void
quoted_words(
    run& _run
)
{
    static const cli::quoting::value QUOTINGS[] =
    {
        cli::quoting::posix,
        cli::quoting::windows,
        cli::quoting::cli11,
        cli::quoting::gnu
    };
    static const char* const         NAMES[] =
    {
        "posix", "windows", "cli11", "gnu"
    };
    static const char* const         WORDS[] =
    {
        "plain", "two words", "it's", "say \"hi\"", "back\\slash", "",
        "trailing\\", "tab\there", "$HOME"
    };

    for (std::size_t rule = 0u;
         rule < (sizeof(QUOTINGS) / sizeof(QUOTINGS[0]));
         ++rule)
    {
        for (std::size_t word = 0u;
             word < (sizeof(WORDS) / sizeof(WORDS[0]));
             ++word)
        {
            const std::string        quoted = cli::quote_word(WORDS[word],
                                                              QUOTINGS[rule]);
            std::vector<std::string> back;
            const cli::status        split  = cli::split_line(quoted,
                                                              back,
                                                              QUOTINGS[rule]);
            const bool               whole  = ( (split.ok())          &&
                                                (back.size() == 1u)   &&
                                                (back[0] == WORDS[word]) );

            _run.note(std::string(NAMES[rule]) + " [" + escape(WORDS[word]) +
                      "] -> [" + escape(quoted) + "] " +
                      (whole ? "whole" : "split: " + written_words(back)));
        }
    }

    return;
}

// QUOTED_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_quoted.
const step_fn QUOTED_STEPS[] =
{
    &quoted_words
};

// QUOTED_COUNT
//   constant: how many steps QUOTED_STEPS holds.
const std::size_t QUOTED_COUNT =
    sizeof(QUOTED_STEPS) / sizeof(QUOTED_STEPS[0]);

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

// 2.3    Response files
//------------------------------------------------------------------------------

/**
 * @brief Reads a response file by a set of rules, after a word already
 *        held, and writes the status, the arguments and the description.
 *
 * @param[in,out] _run      the run.
 * @param[in]     _label    what the line names the rules by.
 * @param[in]     _quoting  the rules.
 * @param[in]     _path     the file's path.
 */
void
write_response(
    run&                _run,
    const char*         _label,
    cli::quoting::value _quoting,
    const char*         _path
)
{
    std::vector<std::string> words(1u,
                                   std::string("held"));
    std::string              error;
    const cli::status        read = cli::read_response_file(_path,
                                                            words,
                                                            _quoting,
                                                            &error);

    _run.note(std::string(_label) + " " + _path + ": " + read.get_name() +
              " " + written_words(words) + " error=[" + escape(error) + "]");

    return;
}

/**
 * @brief RESPONSE 1: response files read by each set of rules, a
 *        byte-order mark skipped, by the platform's rules a file all of
 *        them read alike, and a file that is not there.
 *
 * @param[in,out] _run  the step's run.
 */
void
response_read(
    run& _run
)
{
    static const file_entry FILES[] =
    {
        { "args.rsp",    "a 'b c'\n\"d e\" f\\ g\n" },
        { "bom.rsp",     "\xef\xbb\xbfx \"y z\"\n" },
        { "neutral.rsp", "a \"b c\"\nd\n" },
        { D_NULLPTR,     D_NULLPTR }
    };
    scratch_directory       files(FILES);

    // the files the step reads
    if (!files.ok())
    {
        _run.note("the scratch directory was not made");
    }

    write_response(_run,
                   "posix",
                   cli::quoting::posix,
                   "args.rsp");
    write_response(_run,
                   "windows",
                   cli::quoting::windows,
                   "args.rsp");
    write_response(_run,
                   "cli11",
                   cli::quoting::cli11,
                   "args.rsp");
    write_response(_run,
                   "gnu",
                   cli::quoting::gnu,
                   "args.rsp");
    write_response(_run,
                   "gnu",
                   cli::quoting::gnu,
                   "bom.rsp");
    write_response(_run,
                   "native_response",
                   cli::quoting::native_response,
                   "neutral.rsp");
    write_response(_run,
                   "posix",
                   cli::quoting::posix,
                   "missing.rsp");

    return;
}

// RESPONSE_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_response.
const step_fn RESPONSE_STEPS[] =
{
    &response_read
};

// RESPONSE_COUNT
//   constant: how many steps RESPONSE_STEPS holds.
const std::size_t RESPONSE_COUNT =
    sizeof(RESPONSE_STEPS) / sizeof(RESPONSE_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

// 2.4    Wide text
//------------------------------------------------------------------------------

/**
 * @brief WIDE 1: widen() with a status, for text that converts and for
 *        text that does not, the string given left as it was; a run of
 *        bytes cut short; narrow() of a NULL C string.
 *
 * @param[in,out] _run  the step's run.
 */
void
wide_forms(
    run& _run
)
{
    std::wstring      out(L"held");
    const cli::status converted = cli::widen("caf\xc3\xa9",
                                             out);

    _run.note(std::string("converted: ") + converted.get_name() + " [" +
              code_points(out) + "]");
    out = L"held";

    const cli::status refused = cli::widen("a\xff" "b",
                                           out);

    _run.note(std::string("refused: ") + refused.get_name() + " [" +
              code_points(out) + "]");
    _run.note("cut short [" +
              code_points(cli::widen("caf\xc3\xa9",
                                     3u)) + "]");
    _run.note("null [" +
              escape(cli::narrow(static_cast<const wchar_t*>(D_NULLPTR))) +
              "]");

    return;
}

// WIDE_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_wide.
const step_fn WIDE_STEPS[] =
{
    &wide_forms
};

// WIDE_COUNT
//   constant: how many steps WIDE_STEPS holds.
const std::size_t WIDE_COUNT = sizeof(WIDE_STEPS) / sizeof(WIDE_STEPS[0]);

// 2.5    Departures
//------------------------------------------------------------------------------

/**
 * @brief DEPARTED 1: wide text with an unpaired surrogate made UTF-8, the
 *        surrogate U+FFFD; UTF-8 that is not well-formed made wide, the
 *        empty text.
 *
 * @param[in,out] _run  the step's run.
 */
void
departed_wide(
    run& _run
)
{
    std::wstring lone;

    lone.push_back(static_cast<wchar_t>(0xD800));
    lone += L"a";
    _run.note("unpaired [" + escape(cli::narrow(lone)) + "]");
    _run.note("stray byte [" + code_points(cli::widen("a\xff" "b")) + "]");
    _run.note("overlong [" + code_points(cli::widen("\xc0\xaf")) + "]");
    _run.note("surrogate [" + code_points(cli::widen("\xed\xa0\x80")) +
              "]");

    return;
}

// DEPARTED_STEPS
//   constant: the steps of d_tests_arg_cpp_argv_departures.
const step_fn DEPARTED_STEPS[] =
{
    &departed_wide
};

// DEPARTED_COUNT
//   constant: how many steps DEPARTED_STEPS holds.
const std::size_t DEPARTED_COUNT =
    sizeof(DEPARTED_STEPS) / sizeof(DEPARTED_STEPS[0]);

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

}  // namespace

//==============================================================================
// 3.  CLI11'S SPLITTING AND ENCODING
//==============================================================================

/*
d_tests_arg_cpp_argv_split
  Lines split by CLI11's rules, as its parse of a string splits them.
  Tests the following:
  - spaces; quotes of each kind, after `=` and around words; escapes;
    brackets; quotes unclosed and empty; other whitespace
  - escapes refused, the words left as they were and the failure described
  - Windows-style options; a program's name, any word to CLI11's rules
*/
bool
d_tests_arg_cpp_argv_split()
{
    return steps_hold("SPLIT",
                      SPLIT_STEPS,
                      SPLIT_COUNT,
                      SPLIT);
}

/*
d_tests_arg_cpp_argv_program
  A program's name at the start of a line, as CLI11's split_program_name
  finds it.
  Tests the following:
  - the shortest run of words naming a file, one holding a space; a folder
    passed over
  - a quoted name, with an escaped quote and unclosed; the first word
    otherwise; spaces around, and nothing
*/
bool
d_tests_arg_cpp_argv_program()
{
    return steps_hold("PROGRAM",
                      PROGRAM_STEPS,
                      PROGRAM_COUNT,
                      PROGRAM);
}

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/*
d_tests_arg_cpp_argv_encoding
  Wide text and UTF-8 converted, as CLI11's narrow() and widen() convert
  them.
  Tests the following:
  - characters of one to four bytes; the empty text; a NUL in text of a
    length given; the standard strings
*/
bool
d_tests_arg_cpp_argv_encoding()
{
    return steps_hold("ENCODING",
                      ENCODING_STEPS,
                      ENCODING_COUNT,
                      ENCODING);
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 4.  THE FACE'S OWN
//==============================================================================

/*
d_tests_arg_cpp_argv_rules
  Lines split by the rules beside CLI11's, with a status.
  Tests the following:
  - POSIX's, the C runtime's with a program's name and with line breaks,
    GCC's, and the platform's two
  - text refused: the words left as they were, the description appended
*/
bool
d_tests_arg_cpp_argv_rules()
{
    return steps_hold("RULES",
                      RULES_STEPS,
                      RULES_COUNT,
                      RULES);
}

/*
d_tests_arg_cpp_argv_quoted
  Words quoted so that they split back whole.
  Tests the following:
  - each set of rules over words plain, spaced, quoted, escaped and empty,
    each split back by the same rules
*/
bool
d_tests_arg_cpp_argv_quoted()
{
    return steps_hold("QUOTED",
                      QUOTED_STEPS,
                      QUOTED_COUNT,
                      QUOTED);
}

#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)

/*
d_tests_arg_cpp_argv_response
  Response files read.
  Tests the following:
  - a file by each set of rules; a byte-order mark; the platform's rules
  - a file that is not there: file_error, the arguments as they were
*/
bool
d_tests_arg_cpp_argv_response()
{
    return steps_hold("RESPONSE",
                      RESPONSE_STEPS,
                      RESPONSE_COUNT,
                      RESPONSE);
}

#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES

#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)

/*
d_tests_arg_cpp_argv_wide
  The face's own forms of the conversions.
  Tests the following:
  - widen() with a status: converted, and refused with the string as it
    was; a run of bytes cut short
  - narrow() of a NULL C string
*/
bool
d_tests_arg_cpp_argv_wide()
{
    return steps_hold("WIDE",
                      WIDE_STEPS,
                      WIDE_COUNT,
                      WIDE);
}

//==============================================================================
// 5.  DEPARTURES FROM CLI11
//==============================================================================

/*
d_tests_arg_cpp_argv_departures
  Where the face's conversions depart from CLI11's (ISSUES 62).
  Tests the following:
  - an unpaired surrogate made U+FFFD
  - UTF-8 that is not well-formed made the empty text
*/
bool
d_tests_arg_cpp_argv_departures()
{
    return steps_hold("DEPARTED",
                      DEPARTED_STEPS,
                      DEPARTED_COUNT,
                      DEPARTED);
}

#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

//==============================================================================
// 6.  THE SUITE
//==============================================================================

// d_tests_arg_cpp_argv
//   constant: the suite, in the header's order.
const d_tests_section d_tests_arg_cpp_argv[] =
{
    { "cpp_argv_split",
      d_tests_arg_cpp_argv_split },
    { "cpp_argv_program",
      d_tests_arg_cpp_argv_program },
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "cpp_argv_encoding",
      d_tests_arg_cpp_argv_encoding },
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
    { "cpp_argv_rules",
      d_tests_arg_cpp_argv_rules },
    { "cpp_argv_quoted",
      d_tests_arg_cpp_argv_quoted },
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
    { "cpp_argv_response",
      d_tests_arg_cpp_argv_response },
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
    { "cpp_argv_wide",
      d_tests_arg_cpp_argv_wide },
    { "cpp_argv_departures",
      d_tests_arg_cpp_argv_departures }
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV
};

// d_tests_arg_cpp_argv_count
//   constant: how many sections the table above holds.
const std::size_t d_tests_arg_cpp_argv_count =
    sizeof(d_tests_arg_cpp_argv) / sizeof(d_tests_arg_cpp_argv[0]);
