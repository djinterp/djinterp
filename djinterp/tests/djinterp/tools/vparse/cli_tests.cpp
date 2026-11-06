/*******************************************************************************
* vparse [tests]                                                   cli_tests.cpp
*
* The command line's suite: `vparse`, run in-process, its every subcommand
* and its every way of refusing, the measures over many inputs among them.
*   cli::run() takes its streams, so each case runs the whole command line --
* the kit, the argument parser, the subcommand, the model -- over arguments
* made here, with a file standing in for the standard input, and reads back
* what it wrote and how it ended. Nothing runs a process, and nothing needs a
* display or uxoxo.
*   Where an answer could come from the code under test, it does not: what
* compile writes is held to the session's own disassembly; a part a run
* prints, to the model's listing of the same run; a refusal, to the words the
* session refuses the grammar in, said as the kit says an error; a failure's
* place, to the headline of the same run made by the model; bench's counts,
* to the model's profiles of the same runs; and each difference compare
* names, to match's verdicts on the input it names.
*   The files a case reads and writes are made in a folder of the suite's own
* under the working directory the build runs it in, and removed at the end.
*   LEVELS. The command line's floor is the model's, C++17, so this file's
* is: below it the file is empty rather than an error.
*
*
* path:      /tests/djinterp/tools/vparse/cli_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "cli/cli.hpp"     // the unit under test
#include "cli/report.hpp"  // json_quote
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor its header has

// std
#include <cctype>            // std::tolower
#include <cerrno>            // errno, ENOSPC
#include <cstddef>           // std::size_t
#include <cstdio>            // std::FILE, std::fopen, std::fclose
#include <cstring>           // std::strerror
#include <filesystem>        // std::filesystem
#include <fstream>           // std::ofstream, std::ifstream
#include <initializer_list>  // std::initializer_list
#include <iterator>          // std::istreambuf_iterator
#include <map>               // std::map
#include <string>            // std::string, std::to_string, std::stol
#include <system_error>      // std::error_code
#include <utility>           // std::pair
#include <vector>            // std::vector
// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // buffers, sinks
#include "djinterp/cpp/modules/parsegen/vparse/session.hpp"          // session
// vparse
#include "./tool_tests.hpp"  // the suite's harness and grammars
#include "../../../../inc/djinterp/tools/vparse/run.hpp"    // execute,
                                                            // part_listing
#include "../../../../inc/djinterp/tools/vparse/store.hpp"  // listing_text


namespace
{

namespace fs  = std::filesystem;
namespace cli = ::vparse::cli;
namespace vp  = ::djinterp::parsegen::vparse;
namespace wb  = ::vparse::workbench;

using ::vparse::tests::ARITHMETIC;
using ::vparse::tests::NESTED;
using ::vparse::tests::SUMS;
using ::vparse::tests::chain;
using ::vparse::tests::has;
using ::vparse::tests::request;
using ::vparse::tests::section;

// SCRATCH
//   constant: the folder, under the working directory, the suite's files are
// made in.
const char* const SCRATCH = "cli_tests_files";

// VPARSE_README
//   macro: vparse's README, whose JSON tables tests_cli_json holds the
// command line to; the build defines it. Without it the section reads
// nothing, and fails.
#ifndef VPARSE_README
    #define VPARSE_README "README.md"
#endif  // VPARSE_README

// INPUT
//   constant: the window's example input, which SUMS matches.
const char* const INPUT = "12+3-45";

// UNREAD
//   constant: a grammar the notation's reader stops in, at its end.
const char* const UNREAD = "S = [0-9]+";

// FOREIGN
//   constant: a folder's name no Windows code page can spell: a u with a
// diaeresis and two Japanese characters, which only the Simplified Chinese
// code page has together, and a Thai letter, which only the Thai one has.
const char* const FOREIGN =
    "runs_\xC3\xBC_\xE6\x97\xA5\xE6\x9C\xAC_\xE0\xB8\x81";

// FACTORED
//   constant: NESTED's language, each rule's shared beginning factored out.
const char* const FACTORED =
    "Expr = Sum ;\n"
    "Sum  = Prod More* ;\n"
    "More = '+' Prod | '-' Prod ;\n"
    "Prod = Atom Then* ;\n"
    "Then = '*' Atom | '/' Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// DROPPED
//   constant: NESTED with one alternative dropped, `Prod '-' Sum`.
const char* const DROPPED =
    "Expr = Sum ;\n"
    "Sum  = Prod '+' Sum | Prod ;\n"
    "Prod = Atom '*' Prod | Atom '/' Prod | Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// ODD
//   constant: an input JSON has to escape: a tab, a quote, a backslash, a
// byte that begins no UTF-8 sequence, a character beyond ASCII, and a control
// character with no short escape.
const char* const ODD = "1\t\"\\\xFF\xC3\xA9\x01";

// ODD_JSON
//   constant: ODD as JSON writes it: the tab, the quote and the backslash
// escaped, the stray byte as the text `\xFF`, the character as its own
// bytes, and the control character as `\u0001`.
const char* const ODD_JSON = "\"1\\t\\\"\\\\\\\\xFF\xC3\xA9\\u0001\"";

// captured
//   struct: what one run of the command line wrote, and how it ended.
struct captured
{
    int         status = -1;
    std::string out;
    std::string err;
};

/**
 * @brief Names a file in the suite's folder.
 *
 * @param[in] _name  the file's name.
 * @return its path, as the command line is given it.
 */
std::string at(
    const std::string& _name
)
{
    return std::string(SCRATCH) + "/" + _name;
}

/**
 * @brief Writes a file in the suite's folder, byte for byte.
 *
 * @param[in] _name  the file's name.
 * @param[in] _text  what it holds.
 * @return whether it was written.
 */
bool write(
    const std::string& _name,
    const std::string& _text
)
{
    std::ofstream out(at(_name),
                      std::ios::binary | std::ios::trunc);

    out.write(_text.data(),
              static_cast<std::streamsize>(_text.size()));

    return static_cast<bool>(out);
}

/**
 * @brief Reads a file in the suite's folder whole.
 *
 * @param[in] _name  the file's name.
 * @return what it holds; empty when it cannot be read.
 */
std::string read(
    const std::string& _name
)
{
    std::ifstream in(at(_name),
                     std::ios::binary);

    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

/**
 * @brief Reads a file anywhere, whole.
 *
 * @param[in] _path  the file's path.
 * @return what it holds; empty when it cannot be read.
 */
std::string read_whole(
    const std::string& _path
)
{
    std::ifstream in(_path,
                     std::ios::binary);

    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

/**
 * @brief Takes a buffer's content, and frees it.
 *
 * @param[in,out] _buffer  the buffer, empty after.
 * @return what it held.
 */
std::string taken(
    struct dj_arg_buffer& _buffer
)
{
    const std::string text = (_buffer.data != nullptr)
                                 ? std::string(_buffer.data,
                                               _buffer.length)
                                 : std::string();

    d_arg_buffer_release(&_buffer);

    return text;
}

/**
 * @brief Takes nothing, as a full disk takes nothing, and says why as the
 *        system says it; a dj_pack_sink_write_fn.
 *
 * @param[in] _context  unused.
 * @param[in] _data     unused.
 * @param[in] _size     unused.
 * @return 0, errno ENOSPC.
 */
std::size_t full_disk(
    void*       _context,
    const void* _data,
    std::size_t _size
)
{
    (void)_context;
    (void)_data;
    (void)_size;

    errno = ENOSPC;

    return 0u;
}

/**
 * @brief Runs the command line in-process.
 *
 * @param[in] _arguments  the arguments after the program's name.
 * @param[in] _input      what the standard input holds.
 * @param[in] _full       whether the output is a full disk (full_disk),
 *                        which takes nothing, rather than a buffer.
 * @return what the run wrote, and its exit status.
 *
 * `--color=never` comes first, so that what the run says is the same whether
 * or not the suite's own standard error is a terminal: the kit asks the
 * process's streams, not the sinks a suite gives it, whether to colour.
 */
captured run_vparse(
    const std::vector<std::string>& _arguments,
    const std::string&              _input = std::string(),
    bool                            _full = false
)
{
    std::vector<const char*> argv = { "vparse",
                                      "--color=never" };

    // the arguments after the program's name
    for (const std::string& each : _arguments)
    {
        argv.push_back(each.c_str());
    }

    argv.push_back(nullptr);

    (void)write("stdin.txt",
                _input);

    std::FILE* const    in  = std::fopen(at("stdin.txt").c_str(),
                                         "rb");
    struct dj_arg_buffer out = D_ARG_BUFFER_INIT;
    struct dj_arg_buffer err = D_ARG_BUFFER_INIT;
    cli::context        io;

    io.in  = in;
    io.out = _full ? dj_pack_sink{ &full_disk,
                                  nullptr }
                   : d_arg_buffer_sink(&out);
    io.err = d_arg_buffer_sink(&err);

    captured result;

    result.status = cli::run(static_cast<int>(argv.size() - 1u),
                             argv.data(),
                             io);
    result.out    = taken(out);
    result.err    = taken(err);

    // the stand-in for the standard input
    if (in != nullptr)
    {
        (void)std::fclose(in);
    }

    return result;
}

/**
 * @brief Gives the error the kit writes: `vparse: error: MESSAGE` and a
 *        newline.
 *
 * @param[in] _message  the message.
 * @return the line.
 */
std::string error_line(
    const std::string& _message
)
{
    return "vparse: error: " + _message + "\n";
}

/**
 * @brief Gives the error a refused grammar is said in: the model's headline
 *        for the run that refused it, written as a diagnostic -- its first
 *        letter in lower case, and without its full stop -- after the
 *        grammar's name; and where the headline names the place the reader
 *        stopped, that place first, as `NAME:LINE:COLUMN: error: `, and
 *        not again in the sentence.
 *
 * @param[in] _name  the grammar's name, as the command line gave it.
 * @param[in] _run   the model's run of the grammar.
 * @return the line.
 */
std::string refusal_line(
    const std::string&    _name,
    const wb::run_record& _run
)
{
    std::string sentence = wb::headline(_run);
    std::string first    = "vparse: error: " + _name + ": ";
    const int   far      = _run.reading.far.offset;

    // the place, as the headline words it, moved to the front
    if (far >= 0)
    {
        const std::pair<int, int> at    = wb::line_col(_run.request.grammar,
                                                       far);
        const std::string         place = " at line " +
                                          std::to_string(at.first) +
                                          ", column " +
                                          std::to_string(at.second) +
                                          " (character " +
                                          std::to_string(far) + ")";
        const std::size_t         found = sentence.find(place);

        if (found != std::string::npos)
        {
            sentence.erase(found,
                           place.size());
            first = _name + ":" + std::to_string(at.first) + ":" +
                    std::to_string(at.second) + ": error: ";
        }
    }

    // the full stop, and the capital
    if ( (!sentence.empty()) &&
         (sentence.back() == '.') )
    {
        sentence.pop_back();
    }

    if (!sentence.empty())
    {
        sentence[0] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(sentence[0])));
    }

    return first + sentence + "\n";
}

/**
 * @brief Makes the suite's folder afresh, with the grammars every section
 *        reads in it.
 *
 * @return whether every file was written.
 */
bool prepare()
{
    std::error_code ignored;

    fs::remove_all(SCRATCH,
                   ignored);
    fs::create_directories(SCRATCH,
                           ignored);

    // each file the sections read, and what it holds
    const std::pair<std::string, std::string> files[] =
    {
        { "sums.txt",        SUMS                   },
        { "arithmetic.ebnf", ARITHMETIC             },
        { "undefined.txt",   "S = A ;"              },
        { "twice.txt",       "S = 'a' ;\nS = 'b' ;" },
        { "unread.txt",      UNREAD                 },
        { "loops.txt",       "S = S 'a' | 'a' ;"    },
        { "long.txt",        chain(500u)            },
        { "in.txt",          INPUT                  },
        { "a.txt",           "12+3"                 },
        { "b.txt",           "1+"                   },
        { "c.txt",           "7"                    },
        { "nested.txt",      NESTED                 },
        { "factored.txt",    FACTORED               },
        { "dropped.txt",     DROPPED                },
        { "noway.txt",       "S = S 'a' ;"          }
    };

    // each written, in order, until one cannot be
    for (const std::pair<std::string, std::string>& each : files)
    {
        // a file that could not be written
        if (!write(each.first,
                   each.second))
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Gives the session's verdict on a grammar.
 *
 * @param[in] _grammar  the grammar.
 * @param[in] _ebnf     whether it is EBNF.
 * @return what the session said.
 */
vp::compile_result judged(
    const std::string& _grammar,
    bool               _ebnf
)
{
    vp::session judge;

    return judge.load(_grammar,
                      _ebnf ? vp::frontend::ebnf
                            : vp::frontend::metacircular);
}

/**
 * @brief Gives the session's listing of a grammar's program.
 *
 * @param[in] _grammar  the grammar.
 * @param[in] _ebnf     whether it is EBNF.
 * @return the disassembly; empty when the session refuses the grammar.
 */
std::string listing_of(
    const std::string& _grammar,
    bool               _ebnf
)
{
    vp::session judge;

    (void)judge.load(_grammar,
                     _ebnf ? vp::frontend::ebnf
                           : vp::frontend::metacircular);

    return judge.disassembly();
}

/**
 * @brief Drops the lines of a summary that belong to a run rather than to
 *        its request: when it ran, how long it took, where it came from.
 *
 * @param[in] _text  a summary, as listing_text lays it out.
 * @return the other lines.
 */
std::string without_moments(
    const std::string& _text
)
{
    std::string out;
    std::size_t start = 0u;

    // a line at a time, its newline kept
    while (start < _text.size())
    {
        const std::size_t end  = _text.find('\n',
                                            start);
        const std::size_t stop = (end == std::string::npos) ? _text.size()
                                                            : end + 1u;
        const std::string line = _text.substr(start,
                                              stop - start);

        // a run's own rows
        if ( (line.rfind("time ", 0u) != 0u)  &&
             (line.rfind("when ", 0u) != 0u)  &&
             (line.rfind("origin ", 0u) != 0u) )
        {
            out += line;
        }

        start = stop;
    }

    return out;
}

/**
 * @brief Reads a top-level member of a JSON document the command line
 *        printed.
 *
 * @param[in] _json  the document.
 * @param[in] _key   the member's key.
 * @return the member's value as printed, to the end of its line, without
 *         the comma after it; empty when there is no such member.
 */
std::string member_of(
    const std::string& _json,
    const std::string& _key
)
{
    const std::string key   = "\n  \"" + _key + "\": ";
    const std::size_t found = _json.find(key);

    // no such member
    if (found == std::string::npos)
    {
        return std::string();
    }

    const std::size_t from = found + key.size();
    const std::size_t end  = _json.find('\n',
                                        from);
    std::string       out  = _json.substr(from,
                                          end - from);

    // a member before another ends in a comma
    if ( (!out.empty()) &&
         (out.back() == ',') )
    {
        out.pop_back();
    }

    return out;
}

/**
 * @brief Counts the times a text holds another.
 *
 * @param[in] _text  the text.
 * @param[in] _part  what to count.
 * @return how many times `_part` begins in `_text`.
 */
std::size_t times_in(
    const std::string& _text,
    const std::string& _part
)
{
    std::size_t count = 0u;

    // each place it begins
    for (std::size_t at = _text.find(_part);
         at != std::string::npos;
         at = _text.find(_part,
                         at + 1u))
    {
        count++;
    }

    return count;
}

/**
 * @brief Gives the inputs a compare in JSON names as differences.
 *
 * @param[in] _json  compare's JSON document.
 * @return each difference's input, in order; inputs with no escape in them
 *         are read as they are, which a corpus of expressions has none of.
 */
std::vector<std::string> differences_in(
    const std::string& _json
)
{
    const std::string        head = "{\"input\": \"";
    std::vector<std::string> out;

    // each difference, a record a line
    for (std::size_t at = _json.find(head);
         at != std::string::npos;
         at = _json.find(head,
                         at + 1u))
    {
        const std::size_t from = at + head.size();
        const std::size_t end  = _json.find('"',
                                            from);

        out.push_back(_json.substr(from,
                                   end - from));
    }

    return out;
}

/**
 * @brief Says whether a measure's JSON memoized some rules and not another.
 *
 * @param[in] _json     the document.
 * @param[in] _rules    rules it must have memoized.
 * @param[in] _not      a rule it must not have.
 * @return whether its "memoized" member names each of `_rules` and not
 *         `_not`.
 */
bool memoizes(
    const std::string&                 _json,
    std::initializer_list<const char*> _rules,
    const std::string&                 _not
)
{
    const std::string named = member_of(_json,
                                        "memoized");

    // each rule asked for
    for (const char* rule : _rules)
    {
        // a rule not named
        if (!has(named,
                 std::string("\"") + rule + "\""))
        {
            return false;
        }
    }

    return !has(named,
                "\"" + _not + "\"");
}

/*
tests_cli_help
  The command line describes itself, in the kit's layout, and refuses to run
without a subcommand.
  Tests the following:
  - --help lists every subcommand, says @FILE is read, and gives the exit
    statuses, and ends with 0
  - a subcommand's help lists what it takes -- run's, every part by name and
    all; check's, its operand by its name -- and ends with what its exit
    status answers
  - --version prints the version and ends with 0
  - no subcommand is the kit's usage error, and trouble
*/
bool tests_cli_help()
{
    const captured help = run_vparse({ "--help" });

    // every subcommand
    for (const char* name :
         { "check", "compile", "match", "run", "replay", "saved", "bench",
           "corpus", "compare" })
    {
        // a subcommand the help does not list
        if (!has(help.out,
                 std::string("  ") + name + " "))
        {
            return false;
        }
    }

    const captured run_help = run_vparse({ "run", "--help" });

    // every part, by name
    for (const std::string& part : wb::part_names())
    {
        // a part run's help does not name
        if (!has(run_help.out,
                 part))
        {
            return false;
        }
    }

    const captured version    = run_vparse({ "--version" });
    const captured nothing    = run_vparse({});
    const captured check_help = run_vparse({ "check", "--help" });

    return ( (help.status == cli::exit_status::PASSED)             &&
             (has(help.out, "Usage: vparse [OPTION]... COMMAND"))  &&
             (has(help.out, "\n  @FILE "))                         &&
             (has(help.out, "An @FILE is split as GCC splits"))    &&
             (has(help.out, "\nExit status:\n"))                   &&
             (run_help.status == cli::exit_status::PASSED)         &&
             (has(run_help.out, "or trace; all"))                  &&
             (has(run_help.out, "Exit status: 0 when the input "
                                "matched; 1 when it did not"))    &&
             (version.status == cli::exit_status::PASSED)          &&
             (version.out == std::string(cli::version()) + "\n")   &&
             (has(version.out, "vparse "))                         &&
             (nothing.status == cli::exit_status::REFUSED)         &&
             (nothing.out.empty())                                 &&
             (has(nothing.err,
                  error_line("a command is required")))            &&
             (has(check_help.out,
                  "Usage: vparse check [OPTION]... GRAMMAR\n"))    &&
             (has(check_help.out,
                  "Exit status: 0 when the grammar is accepted; 1 "
                  "when it is refused")) );
}

/*
tests_cli_check
  check judges a grammar as the session does, and says so.
  Tests the following:
  - an accepted grammar is named with its rules and the size of the program
    the session built, and ends with PASSED; an EBNF file is read as EBNF by
    its name, and not when told otherwise
  - every fault the session refuses is refused, in its words, in either
    notation, said as an error, and ends with NOT_ACCEPTED, check's answer;
    so does a grammar the reader cannot read -- in a file, from standard
    input, or EBNF read as the notation -- the place it stopped first, as
    `NAME:LINE:COLUMN: error: `
  - a grammar that loops on an empty input is accepted, and said to loop
  - a grammar whose reading takes more steps than --step-limit gives is read
    under the reader's own limit, and accepted, and a long grammar's rules
    are counted past the first ten rather than named
  - --format json carries the verdict; -q prints nothing but the error
*/
bool tests_cli_check()
{
    const std::size_t    sums       = judged(SUMS,
                                             false).instructions;
    const wb::run_record unreadable = wb::execute(1,
                                                  request("notation",
                                                          UNREAD,
                                                          ""));
    const wb::run_record notation   = wb::execute(1,
                                                  request("notation",
                                                          ARITHMETIC,
                                                          ""));

    const captured    accepted  = run_vparse({ "check", at("sums.txt") });
    const captured    ebnf      = run_vparse({ "check",
                                               at("arithmetic.ebnf") });
    const captured    misread   = run_vparse({ "check",
                                               at("arithmetic.ebnf"),
                                               "-f",
                                               "notation" });
    const captured    unread    = run_vparse({ "check", at("unread.txt") });
    const captured    piped     = run_vparse({ "check", "-" },
                                             UNREAD);
    const captured    loops     = run_vparse({ "check", at("loops.txt") });
    const captured    outrun    = run_vparse({ "check",
                                               at("long.txt"),
                                               "--step-limit",
                                               "1000" });
    const captured    json      = run_vparse({ "check",
                                               at("sums.txt"),
                                               "--format",
                                               "json" });
    const captured    quiet     = run_vparse({ "check",
                                               at("undefined.txt"),
                                               "-q" });

    // each verdict, in its words and with its status
    if ( (accepted.status != cli::exit_status::PASSED)              ||
         (accepted.out != "The grammar was accepted: 3 rules (Sum, "
                          "More, Term), " + std::to_string(sums) +
                          " instructions.\n")                       ||
         (ebnf.status != cli::exit_status::PASSED)                  ||
         (!has(ebnf.out, "4 rules (Expr, Term, Factor, Digit)"))    ||
         (misread.status != cli::exit_status::NOT_ACCEPTED)         ||
         (misread.err != refusal_line(at("arithmetic.ebnf"),
                                      notation))                    ||
         (unread.status != cli::exit_status::NOT_ACCEPTED)          ||
         (!unread.out.empty())                                      ||
         (unread.err != refusal_line(at("unread.txt"),
                                     unreadable))                   ||
         (unread.err.rfind(at("unread.txt") + ":1:", 0u) != 0u)    ||
         (piped.err != refusal_line("standard input",
                                    unreadable))                    ||
         (loops.status != cli::exit_status::PASSED)                 ||
         (!has(loops.out, "step limit"))                            ||
         (outrun.status != cli::exit_status::PASSED)                ||
         (!has(outrun.out, "accepted: 500 rules (R0, R1, R2, R3, R4, "
                           "R5, R6, R7, R8, R9 and 490 more)"))     ||
         (!has(json.out, "\"accepted\": true"))                     ||
         (!has(json.out, "\"instructions\": " +
                         std::to_string(sums)))                     ||
         (quiet.status != cli::exit_status::NOT_ACCEPTED)           ||
         (!quiet.out.empty())                                       ||
         (!has(quiet.err, "rule not defined: A")) )
    {
        return false;
    }

    // fault
    //   struct: a grammar the session refuses, and which notation it is in.
    struct fault
    {
        bool        ebnf;
        const char* grammar;
    };

    const fault faults[] = {
        { false, "S = A ;"                  },
        { false, "S = 'a' ;\nS = 'b' ;"     },
        { false, ""                         },
        { true,  "S = A ;"                  },
        { true,  "S = \"a\" ;\nS = \"b\" ;" },
        { true,  "# no rule\n"              } };

    // the same three faults, in the same words, in either notation
    for (const fault& each : faults)
    {
        const std::string name = each.ebnf ? "fault.ebnf" : "fault.txt";

        (void)write(name,
                    each.grammar);

        const captured           refused = run_vparse({ "check",
                                                        at(name) });
        const vp::compile_result verdict = judged(each.grammar,
                                                  each.ebnf);

        // refused as the session refuses it, said as an error
        if ( (verdict.ok)                                         ||
             (refused.status != cli::exit_status::NOT_ACCEPTED)   ||
             (!refused.out.empty())                               ||
             (refused.err.rfind("vparse: error: " + at(name) +
                                ": ",
                                0u) != 0u)                        ||
             (!has(refused.err, verdict.error)) )
        {
            return false;
        }
    }

    return true;
}

/*
tests_cli_compile
  compile writes the session's own listing of the program it builds.
  Tests the following:
  - the listing is the session's disassembly, byte for byte, in either
    notation, on the standard output or in the file -o names
  - a refused grammar writes no program, and says why, as an error
  - a file that cannot be written is trouble, said with the system's reason;
    -o - is the standard output
*/
bool tests_cli_compile()
{
    const captured notation = run_vparse({ "compile", at("sums.txt") });
    const captured ebnf     = run_vparse({ "compile", at("arithmetic.ebnf") });
    const captured into     = run_vparse({ "compile",
                                           at("sums.txt"),
                                           "-o",
                                           at("program.txt") });
    const captured refused  = run_vparse({ "compile", at("undefined.txt") });
    const captured nowhere  = run_vparse({ "compile",
                                           at("sums.txt"),
                                           "-o",
                                           at("no/such/folder/program.txt") });
    const captured dashed   = run_vparse({ "compile",
                                           at("sums.txt"),
                                           "-o",
                                           "-" });

    return ( (notation.status == cli::exit_status::PASSED)       &&
             (notation.out == listing_of(SUMS, false))           &&
             (!notation.out.empty())                             &&
             (ebnf.status == cli::exit_status::PASSED)           &&
             (ebnf.out == listing_of(ARITHMETIC, true))          &&
             (into.status == cli::exit_status::PASSED)           &&
             (into.out.empty())                                  &&
             (read("program.txt") == notation.out)               &&
             (refused.status == cli::exit_status::REFUSED)       &&
             (refused.out.empty())                               &&
             (has(refused.err, error_line(at("undefined.txt") +
                                          ": the program could not "
                                          "be built: rule not "
                                          "defined: A")))       &&
             (nowhere.status == cli::exit_status::REFUSED)       &&
             (nowhere.out.empty())                               &&
             (has(nowhere.err,
                  error_line("cannot write '" +
                             at("no/such/folder/program.txt") +
                             "': No such file or directory")))  &&
             (dashed.status == cli::exit_status::PASSED)         &&
             (dashed.out == notation.out) );
}

/*
tests_cli_match
  match runs a grammar over each input, and says how each went.
  Tests the following:
  - a line an input, then a tally; NO_MATCH when one did not match, PASSED
    when all did, REFUSED when the grammar was refused, said as an error
  - the line of an input that did not match says where it stopped, as the
    model's traced run of it does, though match ran it untraced first; with
    --no-trace it cannot say
  - inputs given as text, and from the standard input, which may be named
    once: twice is the kit's usage error
  - -q prints nothing; --format json lists every input, each as it was
    given, escaped as JSON escapes it and kept as UTF-8
*/
bool tests_cli_match()
{
    wb::run_request traced = request("notation",
                                     SUMS,
                                     "1+");

    const std::string where  = wb::headline(wb::execute(1,
                                                        traced));
    const captured    mixed  = run_vparse({ "match",
                                            at("sums.txt"),
                                            at("a.txt"),
                                            at("b.txt"),
                                            at("c.txt") });
    const captured    all    = run_vparse({ "match",
                                            at("sums.txt"),
                                            at("a.txt"),
                                            at("c.txt") });
    const captured    texts  = run_vparse({ "match",
                                            at("sums.txt"),
                                            "--text",
                                            "12",
                                            "1+" });
    const captured    piped  = run_vparse({ "match", at("sums.txt"), "-" },
                                          "99-1");
    const captured    twice  = run_vparse({ "match", "-", "-" },
                                          SUMS);
    const captured    quiet  = run_vparse({ "match",
                                            at("sums.txt"),
                                            at("b.txt"),
                                            "-q" });
    const captured    blind  = run_vparse({ "match",
                                            at("sums.txt"),
                                            at("b.txt"),
                                            "--no-trace" });
    const captured    bad    = run_vparse({ "match",
                                            at("undefined.txt"),
                                            at("a.txt") });
    const captured    json   = run_vparse({ "match",
                                            at("sums.txt"),
                                            "--text",
                                            "12",
                                            "1+",
                                            "--format",
                                            "json" });
    const captured    odd    = run_vparse({ "match",
                                            at("sums.txt"),
                                            "--text",
                                            ODD,
                                            "--format",
                                            "json" });

    return ( (mixed.status == cli::exit_status::NO_MATCH)           &&
             (has(mixed.out, at("a.txt") + ": Matched all 4"))       &&
             (has(mixed.out, at("b.txt") + ": " + where + "\n"))     &&
             (has(mixed.out, "2 of 3 inputs matched.\n"))           &&
             (all.status == cli::exit_status::PASSED)               &&
             (texts.status == cli::exit_status::NO_MATCH)           &&
             (has(texts.out, "'12': Matched all 2 characters"))     &&
             (has(texts.out, "'1+': " + where))                     &&
             (piped.status == cli::exit_status::PASSED)             &&
             (has(piped.out, "standard input: Matched all 4"))      &&
             (twice.status == cli::exit_status::REFUSED)            &&
             (twice.out.empty())                                    &&
             (twice.err == error_line("match: standard input can be "
                                      "read once: - may stand for "
                                      "the grammar or for one "
                                      "input") +
                           "Try 'vparse match --help' for more "
                           "information.\n")                        &&
             (quiet.status == cli::exit_status::NO_MATCH)           &&
             (quiet.out.empty())                                    &&
             (quiet.err.empty())                                    &&
             (blind.status == cli::exit_status::NO_MATCH)           &&
             (has(blind.out, "Record the trace"))                   &&
             (bad.status == cli::exit_status::REFUSED)              &&
             (bad.out.empty())                                      &&
             (has(bad.err, error_line(at("undefined.txt") +
                                      ": the program could not be "
                                      "built: rule not defined: "
                                      "A")))                       &&
             (json.status == cli::exit_status::NO_MATCH)            &&
             (has(json.out, "\"input\": \"1+\", \"as_text\": true")) &&
             (has(json.out, "\"matched\": 1"))                      &&
             (has(json.out, "\"total\": 2"))                        &&
             (odd.status == cli::exit_status::NO_MATCH)             &&
             (has(odd.out, std::string("\"input\": ") + ODD_JSON)) );
}

/*
tests_cli_run
  run prints the parts of a run, each as the model lists it.
  Tests the following:
  - every part, printed alone and quiet, is the model's listing of the same
    run, but for the summary's rows that belong to a run rather than to its
    request
  - several parts are printed in the order asked, each under its title, and
    all prints every part
  - the status says how the run went: PASSED, NO_MATCH, REFUSED -- the run
    printed all the same, and the refusal said as an error -- and NO_MATCH
    where left recursion runs to the step limit given
  - --format json carries the outcome and the parts asked for
*/
bool tests_cli_run()
{
    const wb::run_record same = wb::execute(1,
                                            request("notation",
                                                    SUMS,
                                                    INPUT));

    // every part, printed alone
    for (const std::string& part : wb::part_names())
    {
        const captured    shown    = run_vparse({ "run",
                                                  at("sums.txt"),
                                                  at("in.txt"),
                                                  "-p",
                                                  part,
                                                  "-q" });
        const std::string expected =
            wb::listing_text(wb::part_listing(same,
                                              part));

        // the part as the model lists it, but for the run's moments
        if ( (shown.status != cli::exit_status::PASSED) ||
             (without_moments(shown.out) != without_moments(expected)) )
        {
            return false;
        }
    }

    const captured    two    = run_vparse({ "run",
                                            at("sums.txt"),
                                            at("in.txt"),
                                            "-p",
                                            "program,rules" });
    const captured    every  = run_vparse({ "run",
                                            at("sums.txt"),
                                            at("in.txt"),
                                            "-p",
                                            "all" });
    const captured    failed = run_vparse({ "run",
                                            at("sums.txt"),
                                            at("b.txt") });
    const captured    bad    = run_vparse({ "run", at("undefined.txt") });
    const captured    loops  = run_vparse({ "run",
                                            at("loops.txt"),
                                            "aa",
                                            "--text",
                                            "--step-limit",
                                            "500" });
    const captured    json   = run_vparse({ "run",
                                            at("sums.txt"),
                                            at("in.txt"),
                                            "-p",
                                            "summary,program",
                                            "--format",
                                            "json" });
    const std::size_t first  = two.out.find("Program\n-------\n");
    const std::size_t second = two.out.find("Rules\n-----\n");

    // every part, under its title
    for (const std::string& part : wb::part_names())
    {
        // a part all left out
        if (!has(every.out,
                 wb::part_title(part) + "\n"))
        {
            return false;
        }
    }

    return ( (two.status == cli::exit_status::PASSED)              &&
             (first != std::string::npos)                          &&
             (second != std::string::npos)                         &&
             (first < second)                                      &&
             (has(two.out, wb::headline(same)))                    &&
             (failed.status == cli::exit_status::NO_MATCH)         &&
             (bad.status == cli::exit_status::REFUSED)             &&
             (bad.out.rfind("The program could not be built", 0u) == 0u) &&
             (bad.err == error_line(at("undefined.txt") +
                                    ": the program could not be built: "
                                    "rule not defined: A"))        &&
             (loops.status == cli::exit_status::NO_MATCH)          &&
             (has(loops.out, "step limit of 500"))                 &&
             (has(json.out, "\"matched\": true"))                  &&
             (has(json.out, "\"summary\": {"))                     &&
             (has(json.out, "\"program\": {"))                     &&
             (!has(json.out, "\"trace\": {")) );
}

/*
tests_cli_store
  Runs saved from the command line are run folders the window loads, and
replay runs one again to the same parts.
  Tests the following:
  - run --save writes the run's folder and says where; a second run in the
    same second takes the next number in the same session
  - saved lists both, newest first, as text and as JSON
  - replay prints the saved run's parts as the run printed them
  - a folder that is not a saved run is trouble, and says so
  - match --save saves every input's run in one session
*/
bool tests_cli_store()
{
    const std::string root   = at("runs");
    const captured    first  = run_vparse({ "run",
                                            at("sums.txt"),
                                            at("in.txt"),
                                            "--save",
                                            "--root",
                                            root,
                                            "-p",
                                            "program" });
    const captured    second = run_vparse({ "run",
                                            at("sums.txt"),
                                            at("b.txt"),
                                            "--save",
                                            "--root",
                                            root,
                                            "-p",
                                            "program" });
    const captured    listed = run_vparse({ "saved", "--root", root });
    const captured    json   = run_vparse({ "saved",
                                            "--root",
                                            root,
                                            "--format",
                                            "json" });

    const std::vector<wb::saved_run> runs = wb::list_saved(root);

    // both runs saved, and listed
    if ( (first.status != cli::exit_status::PASSED)     ||
         (!has(first.out, "Saved to "))                 ||
         (second.status != cli::exit_status::NO_MATCH)  ||
         (runs.size() != 2u)                            ||
         (listed.status != cli::exit_status::PASSED)    ||
         (!has(json.out, "\"runs\": [")) )
    {
        return false;
    }

    const std::string newest  = wb::utf8_from_path(runs[0].folder);
    const std::string oldest  = wb::utf8_from_path(runs[1].folder);
    const captured    again   = run_vparse({ "replay",
                                             newest,
                                             "-p",
                                             "program" });
    const std::size_t listing = again.out.find("addr");
    const captured    nothing = run_vparse({ "replay", at("") });
    const captured    many    = run_vparse({ "match",
                                             at("sums.txt"),
                                             at("a.txt"),
                                             at("b.txt"),
                                             "--save",
                                             "--root",
                                             at("matches") });
    const std::vector<wb::saved_run> matched = wb::list_saved(at("matches"));

    return ( (listed.out.find(newest) < listed.out.find(oldest))    &&
             (again.status == cli::exit_status::NO_MATCH)           &&
             (listing != std::string::npos)                         &&
             (has(second.out, again.out.substr(listing)))           &&
             (nothing.status == cli::exit_status::REFUSED)          &&
             (nothing.out.empty())                                  &&
             (has(nothing.err, "vparse: error: "))                  &&
             (has(nothing.err, " is not a saved run\n"))            &&
             (many.status == cli::exit_status::NO_MATCH)            &&
             (matched.size() == 2u)                                 &&
             (matched[0].folder.parent_path() ==
              matched[1].folder.parent_path()) );
}

/*
tests_cli_store_names
  A run is saved, listed and replayed under a folder named in any script.
  Tests the following:
  - run --save under a root whose name is beyond ASCII, and beyond any one
    code page, saves there, and says where in UTF-8
  - saved lists the run by its folder, in UTF-8, as text and as JSON
  - replay finds the folder by that name, and prints the parts the run
    printed
*/
bool tests_cli_store_names()
{
    const std::string root  = at(FOREIGN);
    const captured    saved = run_vparse({ "run",
                                           at("sums.txt"),
                                           at("in.txt"),
                                           "--save",
                                           "--root",
                                           root,
                                           "-p",
                                           "program" });

    const std::vector<wb::saved_run> runs =
        wb::list_saved(wb::path_from_utf8(root));

    // saved where it was asked to be, and said so
    if ( (saved.status != cli::exit_status::PASSED)  ||
         (!has(saved.out,
               "Saved to " + root))                  ||
         (runs.size() != 1u) )
    {
        return false;
    }

    const std::string folder  = wb::utf8_from_path(runs[0].folder);
    const captured    listed  = run_vparse({ "saved", "--root", root });
    const captured    json    = run_vparse({ "saved",
                                             "--root",
                                             root,
                                             "--format",
                                             "json" });
    const captured    again   = run_vparse({ "replay",
                                             folder,
                                             "-p",
                                             "program" });
    const std::size_t listing = again.out.find("addr");

    return ( (folder.rfind(root,
                           0u) == 0u)                       &&
             (listed.status == cli::exit_status::PASSED)    &&
             (has(listed.out,
                  folder))                                  &&
             (json.status == cli::exit_status::PASSED)      &&
             (has(json.out,
                  FOREIGN))                                 &&
             (again.status == cli::exit_status::PASSED)     &&
             (listing != std::string::npos)                 &&
             (has(saved.out,
                  again.out.substr(listing))) );
}

/*
tests_cli_refusals
  A command line that cannot be carried out ends as the kit ends one: an
error, said once, and trouble.
  Tests the following:
  - a grammar that is not there, or is a folder, is trouble, said as the kit
    says a file it cannot read, with why
  - a part, a frontend or a step limit that is not one, --ebnf with
    --frontend, match with no input, and a command that is not one are
    usage errors: the error, the command's, and a pointer to its help
  - a grammar read from the standard input runs as a file's does
*/
bool tests_cli_refusals()
{
    const captured missing  = run_vparse({ "check", at("no_such.txt") });
    const captured folder   = run_vparse({ "check", SCRATCH });
    const captured part     = run_vparse({ "run",
                                           at("sums.txt"),
                                           "-p",
                                           "nope" });
    const captured frontend = run_vparse({ "run",
                                           at("sums.txt"),
                                           "-f",
                                           "abnf" });
    const captured limit    = run_vparse({ "run",
                                           at("sums.txt"),
                                           "--step-limit",
                                           "0" });
    const captured both     = run_vparse({ "run",
                                           at("sums.txt"),
                                           "--ebnf",
                                           "-f",
                                           "notation" });
    const captured none     = run_vparse({ "match", at("sums.txt") });
    const captured unknown  = run_vparse({ "frobnicate" });
    const captured piped    = run_vparse({ "run", "-", "12", "--text" },
                                         SUMS);

    // each usage error: its command's, and a pointer to that command's help
    for (const captured* each : { &part, &frontend, &limit, &both, &none })
    {
        const std::string name = (each == &none) ? "match" : "run";

        // not trouble, or not said as the kit says a usage error
        if ( (each->status != cli::exit_status::REFUSED)                 ||
             (!each->out.empty())                                        ||
             (each->err.rfind("vparse: error: " + name + ": ",
                              0u) != 0u)                                 ||
             (!has(each->err,
                   "\nTry 'vparse " + name + " --help' for more "
                   "information.\n")) )
        {
            return false;
        }
    }

    return ( (missing.status == cli::exit_status::REFUSED)       &&
             (missing.err == error_line("cannot read '" +
                                        at("no_such.txt") +
                                        "': No such file or "
                                        "directory"))            &&
             (folder.status == cli::exit_status::REFUSED)        &&
             (folder.err.rfind("vparse: error: cannot read '" +
                               std::string(SCRATCH) + "': ",
                               0u) == 0u)                        &&
             (has(part.err, "nope not in"))                      &&
             (unknown.status == cli::exit_status::REFUSED)       &&
             (has(unknown.err, "'frobnicate' is not a command")) &&
             (piped.status == cli::exit_status::PASSED)          &&
             (has(piped.out, "Matched all 2 characters")) );
}

/*
tests_cli_kit
  What the kit gives the command line beyond help and its failures: -q
wherever it is given, @FILE, and an output that takes nothing.
  Tests the following:
  - -q before the command is -q after it: match prints nothing, and says
    nothing, its status saying it all
  - @FILE stands for the arguments the file holds, the command among them;
    after --, it is an input's name
  - an output that takes nothing, as a full disk, is trouble, said with the
    system's reason, for a command's result and for help alike
*/
bool tests_cli_kit()
{
    const captured    before   = run_vparse({ "-q",
                                              "match",
                                              at("sums.txt"),
                                              at("b.txt") });
    const bool        filed    = write("match.rsp",
                                       "match\n" + at("sums.txt") + "\n" +
                                       at("a.txt") + "\n" + at("c.txt") +
                                       "\n");
    const captured    direct   = run_vparse({ "match",
                                              at("sums.txt"),
                                              at("a.txt"),
                                              at("c.txt") });
    const captured    held     = run_vparse({ "@" + at("match.rsp") });
    const captured    after    = run_vparse({ "match",
                                              at("sums.txt"),
                                              "--",
                                              "@" + at("match.rsp") });
    const captured    full     = run_vparse({ "match",
                                              at("sums.txt"),
                                              at("a.txt") },
                                            std::string(),
                                            true);
    const captured    helpless = run_vparse({ "--help" },
                                            std::string(),
                                            true);
    const std::string no_room  = std::strerror(ENOSPC);

    return ( (before.status == cli::exit_status::NO_MATCH)           &&
             (before.out.empty())                                    &&
             (before.err.empty())                                    &&
             (filed)                                                 &&
             (direct.status == cli::exit_status::PASSED)             &&
             (held.status == direct.status)                          &&
             (held.out == direct.out)                                &&
             (after.status == cli::exit_status::REFUSED)             &&
             (after.err == error_line("cannot read '@" +
                                      at("match.rsp") +
                                      "': No such file or "
                                      "directory"))                  &&
             (full.status == cli::exit_status::REFUSED)              &&
             (full.err == error_line("cannot write standard "
                                     "output: " + no_room))          &&
             (helpless.status == cli::exit_status::REFUSED)          &&
             (helpless.err == full.err) );
}

/*
tests_cli_bench
  bench runs a grammar over many inputs, each counted as the model counts a
run, and timed apart.
  Tests the following:
  - over the inputs given, the steps are the sum of the model's runs', and
    each rule's counts the sum of the model's profiles of those runs; the
    time is printed, in JSON as a number
  - with no input, the grammar's corpus is run: the inputs corpus makes with
    the same settings
  - --memoize auto memoizes Sum, Prod and Atom of NESTED over its corpus,
    and not Expr, which no input calls twice; it says the steps the corpus
    took without the memo -- the plain bench's -- and takes fewer
  - a memo name no rule has, and a grammar the session refuses, are REFUSED,
    said as errors
*/
bool tests_cli_bench()
{
    const captured       json = run_vparse({ "bench",
                                             at("sums.txt"),
                                             at("a.txt"),
                                             at("b.txt"),
                                             "--times",
                                             "1",
                                             "--format",
                                             "json" });
    const wb::run_record one  = wb::execute(1,
                                            request("notation",
                                                    SUMS,
                                                    "12+3"));
    const wb::run_record two  = wb::execute(1,
                                            request("notation",
                                                    SUMS,
                                                    "1+"));
    const long           sum  = one.recognition.steps +
                                two.recognition.steps;

    // the totals, the model's sums
    if ( (json.status != cli::exit_status::PASSED)              ||
         (member_of(json.out,
                    "inputs") != "2")                           ||
         (member_of(json.out,
                    "matched") != "1")                          ||
         (member_of(json.out,
                    "steps") != std::to_string(sum))            ||
         (member_of(json.out,
                    "ms").empty())                              ||
         (member_of(json.out,
                    "memoized") != "[]") )
    {
        return false;
    }

    // rule by rule, the model's profiles added
    for (std::size_t index = 0u;
         index < one.recognition.counts.rules.size();
         index++)
    {
        const vp::peg::profile::DRuleCounts& a =
            one.recognition.counts.rules[index];
        const vp::peg::profile::DRuleCounts& b =
            two.recognition.counts.rules[index];
        const std::string                    line =
            "{\"rule\": \"" + one.entry[static_cast<std::size_t>(a.start)] +
            "\", \"calls\": " + std::to_string(a.calls + b.calls) +
            ", \"repeats\": " + std::to_string(a.repeats + b.repeats) +
            ", \"repeat_steps\": " +
            std::to_string(a.repeat_steps + b.repeat_steps) +
            ", \"hits\": 0, \"steps\": " +
            std::to_string(a.steps + b.steps) + ", \"arrivals\": " +
            std::to_string(a.arrivals + b.arrivals) + ", \"revisits\": " +
            std::to_string(a.revisits + b.revisits) + "}";

        // a rule counted otherwise
        if (!has(json.out,
                 line))
        {
            return false;
        }
    }

    const captured made   = run_vparse({ "corpus",
                                         at("nested.txt"),
                                         "--walks",
                                         "6",
                                         "--format",
                                         "json" });
    const captured corpus = run_vparse({ "bench",
                                         at("nested.txt"),
                                         "--walks",
                                         "6",
                                         "--times",
                                         "1" });
    const captured plain  = run_vparse({ "bench",
                                         at("nested.txt"),
                                         "--walks",
                                         "10",
                                         "--times",
                                         "1",
                                         "--format",
                                         "json" });
    const captured memo   = run_vparse({ "bench",
                                         at("nested.txt"),
                                         "--walks",
                                         "10",
                                         "--times",
                                         "1",
                                         "--memoize",
                                         "auto",
                                         "--format",
                                         "json" });
    const captured nope   = run_vparse({ "bench",
                                         at("nested.txt"),
                                         at("a.txt"),
                                         "--memoize",
                                         "Nope" });
    const captured bad    = run_vparse({ "bench",
                                         at("undefined.txt"),
                                         at("a.txt") });
    const std::size_t inputs = times_in(made.out,
                                        "\"kind\": ");

    return ( (corpus.status == cli::exit_status::PASSED)                   &&
             (inputs > 0u)                                                 &&
             (has(corpus.out, "over " + std::to_string(inputs) +
                              " inputs ("))                                &&
             (memo.status == cli::exit_status::PASSED)                     &&
             (memoizes(memo.out,
                       { "Sum", "Prod", "Atom" },
                       "Expr"))                                            &&
             (member_of(memo.out,
                        "unmemoized_steps") ==
              member_of(plain.out,
                        "steps"))                                          &&
             (std::stol(member_of(memo.out,
                                  "steps")) <
              std::stol(member_of(plain.out,
                                  "steps")))                               &&
             (member_of(plain.out,
                        "unmemoized_steps") == "null")                     &&
             (nope.status == cli::exit_status::REFUSED)                    &&
             (nope.out.empty())                                            &&
             (nope.err == error_line(at("nested.txt") +
                                     ": no rule is named Nope, to be "
                                     "called through the memo"))         &&
             (bad.status == cli::exit_status::REFUSED)                     &&
             (bad.out.empty())                                             &&
             (has(bad.err,
                  "rule not defined: A")) );
}

/*
tests_cli_corpus
  corpus makes a grammar's inputs, and lists them or writes them a file each.
  Tests the following:
  - one seed lists one corpus, twice over; another seed lists another
  - -o makes the folder and writes each input to a file named for its place
    and kind, holding the input byte for byte, as JSON lists it
  - every walk of NESTED, so written, is one match says NESTED matches
  - a grammar no walk can end has no corpus, and says so; a grammar the
    session refuses is REFUSED, said as an error; a folder that cannot be
    made is trouble, said
*/
bool tests_cli_corpus()
{
    const captured once  = run_vparse({ "corpus",
                                        at("nested.txt"),
                                        "--walks",
                                        "8" });
    const captured twice = run_vparse({ "corpus",
                                        at("nested.txt"),
                                        "--walks",
                                        "8" });
    const captured other = run_vparse({ "corpus",
                                        at("nested.txt"),
                                        "--walks",
                                        "8",
                                        "--seed",
                                        "7" });
    const captured json  = run_vparse({ "corpus",
                                        at("nested.txt"),
                                        "--walks",
                                        "8",
                                        "--format",
                                        "json" });
    const captured wrote = run_vparse({ "corpus",
                                        at("nested.txt"),
                                        "--walks",
                                        "8",
                                        "-o",
                                        at("made/corpus") });

    // one seed, one corpus
    if ( (once.status != cli::exit_status::PASSED)  ||
         (once.out != twice.out)                    ||
         (once.out == other.out)                    ||
         (wrote.status != cli::exit_status::PASSED) ||
         (!has(wrote.out,
               "Wrote ")) )
    {
        return false;
    }

    std::vector<std::string> walks;
    std::size_t              files = 0u;
    std::error_code          code;

    // every file written, each held to the input JSON lists
    for (fs::directory_iterator each(at("made/corpus"),
                                     code), end;
         ( (!code) &&
           (each != end) );
         each.increment(code))
    {
        const std::string name = wb::utf8_from_path(each->path().filename());
        const std::string held = read("made/corpus/" + name);

        files++;

        // a file whose input JSON does not list, by its kind
        if (!has(json.out,
                 "{\"text\": \"" + held + "\", \"kind\": \"" +
                     ((name.find("-walk") != std::string::npos) ? "walk"
                                                               : "miss") +
                     "\""))
        {
            return false;
        }

        // the walks, for match
        if (name.find("-walk.txt") != std::string::npos)
        {
            walks.push_back(at("made/corpus/" + name));
        }
    }

    std::vector<std::string> arguments = { "match", at("nested.txt") };

    arguments.insert(arguments.end(),
                     walks.begin(),
                     walks.end());

    const captured matched = run_vparse(arguments);
    const captured none    = run_vparse({ "corpus", at("noway.txt") });
    const captured bad     = run_vparse({ "corpus", at("undefined.txt") });
    const captured blocked = run_vparse({ "corpus",
                                          at("nested.txt"),
                                          "-o",
                                          at("in.txt/corpus") });

    return ( (files == times_in(json.out,
                                "\"kind\": "))                     &&
             (fs::exists(at("made/corpus/0001-walk.txt")))        &&
             (!walks.empty())                                     &&
             (matched.status == cli::exit_status::PASSED)         &&
             (none.status == cli::exit_status::PASSED)            &&
             (has(none.out,
                  "No walk of the grammar ends"))                 &&
             (bad.status == cli::exit_status::REFUSED)            &&
             (bad.out.empty())                                    &&
             (has(bad.err, "rule not defined: A"))                &&
             (blocked.status == cli::exit_status::REFUSED)        &&
             (blocked.out.empty())                                &&
             (blocked.err.rfind("vparse: error: cannot make '" +
                                at("in.txt/corpus") + "': ",
                                0u) == 0u) );
}

/*
tests_cli_compare
  compare runs two grammars over the same inputs, and names every input they
say different things of.
  Tests the following:
  - NESTED and FACTORED agree over NESTED's corpus, which both match the
    same inputs of; so do NESTED and ARITHMETIC, the second read as EBNF by
    its name
  - with `Prod '-' Sum` dropped, they differ, and each input named is one
    match says NESTED matches and the other does not
  - NESTED against itself memoized agrees, the memoized taking fewer steps
  - -q prints nothing; '-' for both grammars is a usage error; a grammar
    the session refuses is REFUSED
*/
bool tests_cli_compare()
{
    const captured same    = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("factored.txt"),
                                          "--walks",
                                          "20" });
    const captured json    = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("dropped.txt"),
                                          "--walks",
                                          "20",
                                          "--format",
                                          "json" });
    const captured text    = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("dropped.txt"),
                                          "--walks",
                                          "20" });
    const captured self    = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("nested.txt"),
                                          "--walks",
                                          "20",
                                          "--memoize",
                                          "auto",
                                          "--format",
                                          "json" });
    const captured quiet   = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("dropped.txt"),
                                          "--walks",
                                          "20",
                                          "-q" });
    const captured piped   = run_vparse({ "compare", "-", "-" },
                                        NESTED);
    const captured refused = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("undefined.txt") });
    const captured ebnf    = run_vparse({ "compare",
                                          at("nested.txt"),
                                          at("arithmetic.ebnf"),
                                          "--walks",
                                          "20" });

    const std::vector<std::string> named = differences_in(json.out);

    // agreement, and a difference found
    if ( (same.status != cli::exit_status::PASSED)     ||
         (!has(same.out,
               " agree over "))                       ||
         (json.status != cli::exit_status::NO_MATCH)   ||
         (member_of(json.out,
                    "agreed") != "false")              ||
         (named.empty())                               ||
         (text.status != cli::exit_status::NO_MATCH)   ||
         (!has(text.out,
               " differ on ")) )
    {
        return false;
    }

    // each input named, held to match's verdicts
    for (const std::string& input : named)
    {
        const captured full  = run_vparse({ "match",
                                            at("nested.txt"),
                                            "--text",
                                            input });
        const captured short_of = run_vparse({ "match",
                                               at("dropped.txt"),
                                               "--text",
                                               input });

        // an input the two do not disagree on
        if ( (full.status != cli::exit_status::PASSED)        ||
             (short_of.status != cli::exit_status::NO_MATCH) )
        {
            return false;
        }
    }

    return ( (self.status == cli::exit_status::PASSED)                 &&
             (member_of(self.out,
                        "agreed") == "true")                           &&
             (memoizes(self.out,
                       { "Sum", "Prod", "Atom" },
                       "Expr"))                                          &&
             (std::stol(member_of(self.out,
                                  "second_steps")) <
              std::stol(member_of(self.out,
                                  "first_steps")))                     &&
             (quiet.status == cli::exit_status::NO_MATCH)              &&
             (quiet.out.empty())                                       &&
             (quiet.err.empty())                                       &&
             (piped.status == cli::exit_status::REFUSED)               &&
             (has(piped.err,
                  "compare: standard input can be read once"))         &&
             (refused.status == cli::exit_status::REFUSED)             &&
             (refused.out.empty())                                     &&
             (has(refused.err,
                  error_line(at("undefined.txt") +
                             ": the program could not be built: "
                             "rule not defined: A")))                 &&
             (ebnf.status == cli::exit_status::PASSED) );
}

/*
tests_cli_memoize
  run and match call rules through the memo as --memoize says, and a saved
run keeps the setting.
  Tests the following:
  - run --memoize all matches as the run without it, with the same matches,
    and its profile counts the memo's answers; its summary names the rules
  - --memoize with a name no rule has is REFUSED, in the model's words,
    said as an error and printed with the run
  - match --memoize auto gives every input the verdict match gives it
    without the memo
  - a run saved with --memoize replays with it: the summary's memo row is
    the same
*/
bool tests_cli_memoize()
{
    const captured plain   = run_vparse({ "run",
                                          at("nested.txt"),
                                          "((1+2)*3)",
                                          "--text",
                                          "-p",
                                          "matches",
                                          "-q" });
    const captured memo    = run_vparse({ "run",
                                          at("nested.txt"),
                                          "((1+2)*3)",
                                          "--text",
                                          "-p",
                                          "matches",
                                          "-q",
                                          "--memoize",
                                          "all" });
    const captured counted = run_vparse({ "run",
                                          at("nested.txt"),
                                          "((1+2)*3)",
                                          "--text",
                                          "-p",
                                          "summary,profile",
                                          "--memoize",
                                          "all" });
    const captured nope    = run_vparse({ "run",
                                          at("nested.txt"),
                                          "1",
                                          "--text",
                                          "--memoize",
                                          "Sum,Nope" });
    const captured bare    = run_vparse({ "match",
                                          at("nested.txt"),
                                          "--text",
                                          "(1)",
                                          "(1",
                                          "1-2" });
    const captured chosen  = run_vparse({ "match",
                                          at("nested.txt"),
                                          "--text",
                                          "(1)",
                                          "(1",
                                          "1-2",
                                          "--memoize",
                                          "auto" });
    const captured saved   = run_vparse({ "run",
                                          at("nested.txt"),
                                          "((1))",
                                          "--text",
                                          "--memoize",
                                          "Sum,Prod",
                                          "--save",
                                          "--root",
                                          at("memo_runs") });

    const std::vector<wb::saved_run> runs = wb::list_saved(at("memo_runs"));

    // the memo's run, as the run without it
    if ( (memo.status != cli::exit_status::PASSED)              ||
         (memo.out != plain.out)                                ||
         (!has(counted.out,
               "Expr, Sum, Prod, Atom, Num; "))                 ||
         (!has(counted.out,
               "memo hits"))                                    ||
         (nope.status != cli::exit_status::REFUSED)             ||
         (!has(nope.out,
               "no rule is named Nope"))                        ||
         (!has(nope.err,
               "no rule is named Nope"))                        ||
         (chosen.status != bare.status)                         ||
         (chosen.out.size() == 0u)                              ||
         (runs.size() != 1u) )
    {
        return false;
    }

    const captured again = run_vparse({ "replay",
                                        wb::utf8_from_path(runs[0].folder) });

    return ( (has(saved.out,
                  "memo           Sum, Prod; "))              &&
             (has(again.out,
                  "memo           Sum, Prod; ")) );
}

/**
 * @brief Gives the summary's memo row of a run over NESTED with --memoize
 *        given the values given, in order.
 *
 * @param[in] _values  each value, given to --memoize of its own.
 * @return what the run printed; its summary has the memo row.
 */
captured memoized_by(
    const std::vector<std::string>& _values
)
{
    std::vector<std::string> arguments = { "run",
                                           at("nested.txt"),
                                           "(1)",
                                           "--text",
                                           "-q" };

    // each value, an option of its own
    for (const std::string& value : _values)
    {
        arguments.push_back("--memoize");
        arguments.push_back(value);
    }

    return run_vparse(arguments);
}

/*
tests_cli_memoize_values
  --memoize collects its values, as an option that collects them does
(GRAMMAR.md, R8), and folds them in order.
  Tests the following:
  - names given one an option are the names given separated by commas
  - a word given after names replaces them, and names after a word replace
    it, so that a word appended overrides
*/
bool tests_cli_memoize_values()
{
    const captured apart    = memoized_by({ "Sum", "Prod" });
    const captured together = memoized_by({ "Sum,Prod" });
    const captured mixed    = memoized_by({ "Sum", "Prod,Atom" });
    const captured after    = memoized_by({ "all", "Sum" });
    const captured off      = memoized_by({ "Sum", "Prod", "none" });
    const captured chosen   = memoized_by({ "Sum", "auto" });

    return ( (apart.status == cli::exit_status::PASSED)            &&
             (has(apart.out, "memo           Sum, Prod; "))         &&
             (without_moments(together.out) ==
              without_moments(apart.out))                           &&
             (has(mixed.out, "memo           Sum, Prod, Atom; "))   &&
             (has(after.out, "memo           Sum; "))               &&
             (has(off.out, "memo           none\n"))                &&
             (has(chosen.out, "memo           auto"))               &&
             (chosen.status == cli::exit_status::PASSED) );
}

// json_node
//   struct: a JSON value as tests_cli_json reads one: an object's keys, in
// order, with their values; an array's elements; nothing more of any other
// value.
struct json_node
{
    bool                     object = false;
    std::vector<std::string> keys;
    std::vector<json_node>   values;
};

/**
 * @brief Steps past white space.
 *
 * @param[in]     _text  the document.
 * @param[in,out] _at    where the reading is.
 */
void skip_space(
    const std::string& _text,
    std::size_t&       _at
)
{
    // space, tab, and the line ends
    while ( (_at < _text.size()) &&
            ( (_text[_at] == ' ')  ||
              (_text[_at] == '\t') ||
              (_text[_at] == '\n') ||
              (_text[_at] == '\r') ) )
    {
        _at++;
    }

    return;
}

/**
 * @brief Reads a JSON string, its escapes kept as they are written but for
 *        `\"`, which a key never holds.
 *
 * @param[in]     _text  the document.
 * @param[in,out] _at    where the string's quote is; past its close after.
 * @param[out]    _out   receives the string's characters.
 * @return whether a string was read whole.
 */
bool read_string(
    const std::string& _text,
    std::size_t&       _at,
    std::string&       _out
)
{
    // not a string
    if ( (_at >= _text.size()) ||
         (_text[_at] != '"') )
    {
        return false;
    }

    _out.clear();

    // a character at a time, an escape two
    for (_at++; _at < _text.size(); _at++)
    {
        if (_text[_at] == '"')
        {
            _at++;

            return true;
        }

        _out += _text[_at];
        _at  += (_text[_at] == '\\') ? 1u : 0u;
    }

    return false;
}

bool read_node(
    const std::string& _text,
    std::size_t&       _at,
    json_node&         _out
);

/**
 * @brief Reads an object member's key, and the colon after it.
 *
 * @param[in]     _text  the document.
 * @param[in,out] _at    where the key's quote is; past the colon after.
 * @param[out]    _key   receives the key.
 * @return whether a key and its colon were read.
 */
bool read_key(
    const std::string& _text,
    std::size_t&       _at,
    std::string&       _key
)
{
    // the key
    if (!read_string(_text,
                     _at,
                     _key))
    {
        return false;
    }

    skip_space(_text,
               _at);

    // its colon
    if ( (_at >= _text.size()) ||
         (_text[_at] != ':') )
    {
        return false;
    }

    _at++;

    return true;
}

/**
 * @brief Reads an object's members or an array's elements, after its
 *        opening bracket.
 *
 * @param[in]     _text  the document.
 * @param[in,out] _at    where the first member or element is; past the
 *                       closing bracket after.
 * @param[in,out] _out   receives the members or the elements.
 * @return whether it was read whole.
 */
bool read_inside(
    const std::string& _text,
    std::size_t&       _at,
    json_node&         _out
)
{
    const char close = _out.object ? '}' : ']';

    skip_space(_text,
               _at);

    // a member or an element at a time, until the close
    while ( (_at < _text.size()) &&
            (_text[_at] != close) )
    {
        std::string key;
        json_node   value;

        // an object's key, and its colon
        if ( (_out.object) &&
             (!read_key(_text,
                        _at,
                        key)) )
        {
            return false;
        }

        // the value, then a comma or the close
        if (!read_node(_text,
                       _at,
                       value))
        {
            return false;
        }

        _out.keys.push_back(key);
        _out.values.push_back(std::move(value));
        skip_space(_text,
                   _at);
        _at += ( (_at < _text.size()) &&
                 (_text[_at] == ',') ) ? 1u : 0u;
        skip_space(_text,
                   _at);
    }

    _at++;

    return (_at <= _text.size());
}

/**
 * @brief Reads one JSON value.
 *
 * @param[in]     _text  the document.
 * @param[in,out] _at    where the value is; past it after.
 * @param[out]    _out   receives the value.
 * @return whether a value was read whole.
 */
bool read_node(
    const std::string& _text,
    std::size_t&       _at,
    json_node&         _out
)
{
    std::string ignored;

    skip_space(_text,
               _at);

    // nothing left
    if (_at >= _text.size())
    {
        return false;
    }

    // an object or an array
    if ( (_text[_at] == '{') ||
         (_text[_at] == '[') )
    {
        _out.object = (_text[_at++] == '{');

        return read_inside(_text,
                           _at,
                           _out);
    }

    // a string
    if (_text[_at] == '"')
    {
        return read_string(_text,
                           _at,
                           ignored);
    }

    const std::size_t start = _at;

    // a number, true, false or null: its letters and digits
    while ( (_at < _text.size()) &&
            ( (std::isalnum(static_cast<unsigned char>(_text[_at])) != 0) ||
              (_text[_at] == '-') ||
              (_text[_at] == '+') ||
              (_text[_at] == '.') ) )
    {
        _at++;
    }

    return (_at > start);
}

/**
 * @brief Finds a value in a document by its path: members' keys joined by
 *        `.`, `[]` after a key for the array's first element, and `PART`
 *        for an object's first member, whatever its key.
 *
 * @param[in] _root  the document.
 * @param[in] _path  the path: `inputs[]`, `parts.PART`, `differences[].first`;
 *                   empty for the document itself.
 * @return the value; NULL where the path leads nowhere.
 */
const json_node* node_at(
    const json_node&   _root,
    const std::string& _path
)
{
    const json_node* node  = &_root;
    std::size_t      start = 0u;

    // a step at a time
    while ( (node != nullptr) &&
            (start < _path.size()) )
    {
        const std::size_t dot   = _path.find('.',
                                             start);
        std::string       step  = _path.substr(start,
                                               dot - start);
        const bool        first = ( (step.size() > 2u) &&
                                    (step.compare(step.size() - 2u,
                                                  2u,
                                                  "[]") == 0) );
        const json_node*  next  = nullptr;

        step = first ? step.substr(0u, step.size() - 2u) : step;

        // the member of that key, or the first
        for (std::size_t at = 0u; at < node->keys.size(); at++)
        {
            next = ( (next == nullptr) &&
                     ( (step == "PART") ||
                       (node->keys[at] == step) ) ) ? &node->values[at]
                                                     : next;
        }

        node  = ( (first) &&
                  (next != nullptr) ) ? ( next->values.empty()
                                          ? nullptr
                                          : &next->values[0] )
                                      : next;
        start = (dot == std::string::npos) ? _path.size() : dot + 1u;
    }

    return node;
}

/**
 * @brief Reads the README's JSON tables: each `#### `ID`` heading and the
 *        keys its table's rows give, in order.
 *
 * @param[in] _readme  the README.
 * @return each table's keys, by its ID.
 */
std::map<std::string, std::vector<std::string>> documented(
    const std::string& _readme
)
{
    std::map<std::string, std::vector<std::string>> tables;
    std::string                                     id;
    std::size_t                                     start = 0u;

    // a line at a time
    while (start < _readme.size())
    {
        std::size_t       end  = _readme.find('\n',
                                              start);
        end                    = (end == std::string::npos) ? _readme.size()
                                                            : end;
        const std::string line = _readme.substr(start,
                                                end - start);
        const std::size_t tick = line.find('`',
                                           6u);

        // a table's heading, or a row of the table under it
        if (line.rfind("#### `", 0u) == 0u)
        {
            id         = line.substr(6u,
                                     tick - 6u);
            tables[id] = std::vector<std::string>();
        }
        else if ( (!id.empty()) &&
                  (line.rfind("| `", 0u) == 0u) )
        {
            tables[id].push_back(line.substr(3u,
                                             line.find('`', 3u) - 3u));
        }

        id    = (line.rfind("## ", 0u) == 0u) ? std::string() : id;
        start = end + 1u;
    }

    return tables;
}

// json_case
//   struct: one of tests_cli_json's runs: the table it is held to, the
// command line, and where in the document the table's object is.
struct json_case
{
    std::string              table;
    std::vector<std::string> arguments;
    std::string              path;
};

/**
 * @brief The runs over one grammar and over saved runs.
 *
 * @param[in] _root  the folder a run is saved under, which replay and saved
 *                   read.
 * @return the cases, each a table and the run held to it.
 */
std::vector<json_case> json_runs(
    const std::string& _root
)
{
    const std::vector<std::string> checked = { "check", at("sums.txt"),
                                               "--format", "json" };
    const std::vector<std::string> matched = { "match", at("sums.txt"),
                                               at("a.txt"), at("b.txt"),
                                               "--format", "json" };
    const std::vector<std::string> ran     = { "run", at("sums.txt"),
                                               at("in.txt"), "-p",
                                               "summary,rules", "--format",
                                               "json" };
    const std::vector<std::string> listed  = { "saved", "--root", _root,
                                               "--format", "json" };
    const std::vector<std::string> folder  = wb::list_saved(_root).empty()
        ? std::vector<std::string>()
        : std::vector<std::string>{ "replay",
                                    wb::utf8_from_path(
                                        wb::list_saved(_root)[0].folder),
                                    "--format", "json" };

    return { { "check", checked, "" },
             { "check", { "check", at("undefined.txt"), "--format",
                          "json" }, "" },
             { "match", matched, "" },
             { "match", { "match", at("undefined.txt"), at("a.txt"),
                          "--format", "json" }, "" },
             { "match.inputs[]", matched, "inputs[]" },
             { "run", ran, "" },
             { "run", { "run", at("undefined.txt"), "--format", "json" },
               "" },
             { "run.parts.PART", ran, "parts.PART" },
             { "replay", folder, "" },
             { "saved", listed, "" },
             { "saved.runs[]", listed, "runs[]" } };
}

/**
 * @brief The runs of the measures, and their refusals.
 *
 * @return the cases, each a table and the run held to it.
 */
std::vector<json_case> json_measures()
{
    const std::vector<std::string> bench   = { "bench", at("sums.txt"),
                                               at("a.txt"), "--times", "1",
                                               "--format", "json" };
    const std::vector<std::string> corpus  = { "corpus", at("nested.txt"),
                                               "--walks", "2", "--format",
                                               "json" };
    const std::vector<std::string> compare = { "compare", at("nested.txt"),
                                               at("dropped.txt"), "--walks",
                                               "20", "--format", "json" };

    return { { "bench", bench, "" },
             { "bench.peaks", bench, "peaks" },
             { "bench.memo", bench, "memo" },
             { "bench.rules[]", bench, "rules[]" },
             { "bench.runs[]", bench, "runs[]" },
             { "corpus", corpus, "" },
             { "corpus.inputs[]", corpus, "inputs[]" },
             { "compare", compare, "" },
             { "compare.differences[]", compare, "differences[]" },
             { "compare.differences[].first", compare,
               "differences[].first" },
             { "compare.differences[].first", compare,
               "differences[].second" },
             { "refusal", { "bench", at("undefined.txt"), at("a.txt"),
                            "--format", "json" }, "" },
             { "refusal", { "corpus", at("undefined.txt"), "--format",
                            "json" }, "" },
             { "refusal", { "compare", at("nested.txt"),
                            at("undefined.txt"), "--format", "json" },
               "" } };
}

/**
 * @brief Holds one run's document to its table.
 *
 * @param[in] _case    the run, and the table.
 * @param[in] _tables  the README's tables.
 * @return whether the object at the case's path has the table's keys, in
 *         the table's order, and none more.
 */
bool held(
    const json_case&                                       _case,
    const std::map<std::string, std::vector<std::string>>& _tables
)
{
    const auto table = _tables.find(_case.table);

    // a table the README does not have, or a run that cannot be made
    if ( (table == _tables.end()) ||
         (_case.arguments.empty()) )
    {
        return false;
    }

    const captured   run  = run_vparse(_case.arguments);
    json_node        root;
    std::size_t      at   = 0u;
    const bool       read = read_node(run.out,
                                      at,
                                      root);
    const json_node* node = read ? node_at(root,
                                           _case.path)
                                 : nullptr;

    return ( (node != nullptr) &&
             (node->object) &&
             (node->keys == table->second) );
}

/*
tests_cli_json
  Every JSON document the command line writes is the one vparse's README
documents (GRAMMAR.md, R19): the README's tables are read, and each command
is run and held to its table, key for key and in order.
  Tests the following:
  - each document, each object within one, and each refusal, has exactly
    the keys its table gives, in order
  - every table is held to a run, so a table the README keeps for a key
    the code dropped fails as well
*/
bool tests_cli_json()
{
    const std::string root  = at("json_runs");
    const captured    saved = run_vparse({ "run", at("sums.txt"),
                                           at("in.txt"), "--save",
                                           "--root", root });
    const auto        tables = documented(read_whole(VPARSE_README));
    std::vector<json_case> cases = json_runs(root);
    std::map<std::string, bool> covered;

    for (const json_case& each : json_measures())
    {
        cases.push_back(each);
    }

    // each run, held to its table
    for (const json_case& each : cases)
    {
        // a document that is not as its table says
        if (!held(each,
                  tables))
        {
            std::printf("    %s, %s: not as the README's table\n",
                        each.table.c_str(),
                        each.arguments.empty() ? "(no run)"
                                               : each.arguments[0].c_str());

            return false;
        }

        covered[each.table] = true;
    }

    return ( (saved.status == cli::exit_status::PASSED) &&
             (tables.size() == covered.size())          &&
             (tables.size() == 19u) );
}

/*
tests_cli_json_given
  run's and corpus's JSON say what they were given, in the keys 2026.10.26
added after the ones that were there (R19).
  Tests the following:
  - run's `argument` is the input's argument -- under --text the text
    itself, where `input` is its name, quoted -- and `as_text` says which;
    over a file, the file's name; over nothing, ""
  - the two come last, after `parts`
  - corpus's `length`, `repeats` and `misses` are the values given, and come
    after `inputs`
*/
bool tests_cli_json_given()
{
    const captured    text  = run_vparse({ "run", at("sums.txt"), "1+2",
                                           "--text", "--format", "json" });
    const captured    file  = run_vparse({ "run", at("sums.txt"),
                                           at("in.txt"), "--format",
                                           "json" });
    const captured    none  = run_vparse({ "run", at("sums.txt"),
                                           "--format", "json" });
    const captured    made  = run_vparse({ "corpus", at("sums.txt"),
                                           "--walks", "2", "--length",
                                           "40", "--repeats", "2",
                                           "--misses", "0", "--format",
                                           "json" });
    const std::size_t parts = text.out.find("\"parts\": ");
    const std::size_t given = text.out.find("\"argument\": \"1+2\"");
    const std::size_t walks = made.out.find("\"inputs\": ");
    const std::size_t after = made.out.find("\"length\": 40");

    return ( (text.status == cli::exit_status::PASSED)              &&
             (has(text.out, "\"input\": \"'1+2'\""))                &&
             (has(text.out, "\"as_text\": true"))                   &&
             (parts != std::string::npos)                           &&
             (given != std::string::npos)                           &&
             (parts < given)                                        &&
             (file.status == cli::exit_status::PASSED)              &&
             (has(file.out, "\"argument\": " +
                            cli::json_quote(at("in.txt"))))         &&
             (has(file.out, "\"as_text\": false"))                  &&
             (has(none.out, "\"argument\": \"\""))                  &&
             (made.status == cli::exit_status::PASSED)              &&
             (walks != std::string::npos)                           &&
             (after != std::string::npos)                           &&
             (walks < after)                                        &&
             (has(made.out, "\"repeats\": 2"))                      &&
             (has(made.out, "\"misses\": 0")) );
}

#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

/*
tests_cli_completion
  The command line writes its own completion script: the kit's `completion`
command.
  Tests the following:
  - completion bash writes a script that knows every subcommand and run's
    options, and ends with 0
  - a shell it has no script for is the kit's usage error
*/
bool tests_cli_completion()
{
    const captured bash = run_vparse({ "completion", "bash" });
    const captured tcsh = run_vparse({ "completion", "tcsh" });

    // every subcommand, and an option
    for (const char* name :
         { "check", "compile", "match", "run", "replay", "saved", "bench",
           "corpus", "compare", "--part" })
    {
        // a name the completion script does not offer
        if (!has(bash.out,
                 name))
        {
            return false;
        }
    }

    return ( (bash.status == cli::exit_status::PASSED)       &&
             (has(bash.out, "complete"))                     &&
             (tcsh.status == cli::exit_status::REFUSED)      &&
             (tcsh.out.empty())                              &&
             (has(tcsh.err, "vparse: error: completion: "))  &&
             (has(tcsh.err, "'tcsh' is not a shell")) );
}

#else

/*
tests_cli_completion
  The argument parser was built without completion scripts, so the kit's
`completion` command has none to write.
  Tests the following:
  - completion bash is trouble
*/
bool tests_cli_completion()
{
    const captured bash = run_vparse({ "completion", "bash" });

    return (bash.status == cli::exit_status::REFUSED);
}

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION

// SECTIONS
//   constant: the suite, in the order it runs.
const section SECTIONS[] =
{
    { "tests_cli_help",        &tests_cli_help        },
    { "tests_cli_check",       &tests_cli_check       },
    { "tests_cli_compile",     &tests_cli_compile     },
    { "tests_cli_match",       &tests_cli_match       },
    { "tests_cli_run",         &tests_cli_run         },
    { "tests_cli_store",       &tests_cli_store       },
    { "tests_cli_store_names", &tests_cli_store_names },
    { "tests_cli_refusals",    &tests_cli_refusals    },
    { "tests_cli_kit",         &tests_cli_kit         },
    { "tests_cli_bench",       &tests_cli_bench       },
    { "tests_cli_corpus",      &tests_cli_corpus      },
    { "tests_cli_compare",     &tests_cli_compare     },
    { "tests_cli_memoize",     &tests_cli_memoize     },
    { "tests_cli_memoize_values",
      &tests_cli_memoize_values },
    { "tests_cli_json",        &tests_cli_json        },
    { "tests_cli_json_given",  &tests_cli_json_given  },
    { "tests_cli_completion",  &tests_cli_completion  }
};

}  // anonymous namespace

/*
main
  The suite's folder is made before the first section and removed after the
last; a folder that cannot be made fails the suite at once.
*/
int main()
{
    // without its files, no section can run
    if (!prepare())
    {
        std::printf("vparse cli tests: cannot make %s\n",
                    SCRATCH);

        return 1;
    }

    const int       status  = ::vparse::tests::run_sections(
                                  "vparse cli tests",
                                  SECTIONS);
    std::error_code ignored;

    fs::remove_all(SCRATCH,
                   ignored);

    return status;
}

#endif  // floor, for now
