/*******************************************************************************
* vparse [cli]                                                           cli.cpp
*
* Definitions for cli.hpp: the command tree, and the run of it.
*   The tree is made fresh for every run, on the kit's program (tools/cli):
* a root named vparse, which the kit gives help, a version, `-q`, `-v`,
* `--color` and the `help` and `completion` commands, nine subcommands, and
* `gui`, whose work is the window's module, built outside djinterp
* (build/cmake/config/ui/vparse); a command line names exactly one of them. The
* options a subcommand takes are made by the few functions that make each group
* of them, so that a group -- the grammar and how it is read, say -- is the
* same option, spelled and described the same, on every subcommand that has
* it; and each subcommand's help ends with what its exit status answers.
*   Once a parse succeeds, the kit runs the work of the subcommand that was
* used, which hands the handler its record (commands.hpp), behind the kit's
* guard; any other outcome of the parse -- help, a version, a completion
* script, a refusal -- is ended as the kit ends it, in its words, with its
* exit status.
*   `@file` stands for the arguments a file holds, as GCC's does, so that a
* match over thousands of inputs is not held to a command line's length; an
* argument after `--` is never read so.
*
* path:      /src/djinterp/tools/vparse/cli/cli.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.27
*******************************************************************************/
#include "./cli.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string, std::to_string
#include <vector>   // std::vector
// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "djinterp/tools/cli/c/cli_program.h"            // D_CLI_PROGRAM_*
#include "djinterp/tools/cli/cli_program.hpp"            // kit::program
// vparse
#include "./commands.hpp"  // command_line, the records, subcommand::*
#include "../../../../../inc/djinterp/cpp/modules/parsegen/vparse/corpus.hpp"  // settings
#include "../../../../../inc/djinterp/tools/vparse/run.hpp"  // part_names,
                                                             // run_request


// VPARSE_VERSION
//   macro: the build's version, "<major>.<minor>.<patch>", which the build
// defines from its project() line; a unit built without it says so rather
// than claim a number.
#ifndef VPARSE_VERSION
    #define VPARSE_VERSION "(unversioned)"
#endif  // VPARSE_VERSION

namespace vparse
{
namespace cli
{

namespace
{

namespace kit = ::djinterp::cli;
namespace vp  = ::djinterp::parsegen::vparse;
namespace wb  = ::vparse::workbench;

// PROGRAM
//   constant: the root's name, which help and failures show whatever the
// file the program was started from is called.
const char* const PROGRAM = "vparse";

// DESCRIPTION
//   constant: what help says the program is, in lower case and with no full
// stop, as the kit's help takes it.
const char* const DESCRIPTION =
    "the parser generator of djinterp: a grammar in, a parser that runs "
    "out, and every stage of the run to look at";

// FLAGS
//   constant: the kit's program flags: commands, and `@file`.
const unsigned int FLAGS = D_CLI_PROGRAM_COMMANDS |
                           D_CLI_PROGRAM_RESPONSE_FILES;

// VERSION_TEXT
//   constant: what --version prints on its first line.
const char* const VERSION_TEXT = "vparse " VPARSE_VERSION;

// FRONTENDS
//   constant: the frontends --frontend takes.
const char* const FRONTENDS[] = { "notation", "ebnf" };

// FORMATS
//   constant: the formats --format takes.
const char* const FORMATS[] = { "text", "json" };

// STEP_LIMIT_MOST
//   constant: the most steps --step-limit allows. A billion steps is minutes
// of running, and a trace of them would not fit in memory; and the bound is
// the same on every system, where `long`'s own maximum is not.
const long STEP_LIMIT_MOST = 1000000000L;

// CORPUS_MOST
//   constant: the most --walks, --depth, --repeats and --misses allow, and
// ten times it for --length: a corpus of more is minutes of running, and
// the bound is the same on every system.
const long CORPUS_MOST = 100000L;

// SEED_MOST
//   constant: the largest --seed: the largest `long` on every system.
const long SEED_MOST = 2147483647L;

// TIMES_MOST
//   constant: the most runs of an input --times allows.
const long TIMES_MOST = 1000L;

// DEFAULT_TIMES
//   constant: how many runs of each input bench times, when not told.
const long DEFAULT_TIMES = 3L;

// GRAMMAR_HELP
//   constant: what help says the grammar is, where there is one.
const char* const GRAMMAR_HELP =
    "the grammar: a file, or - for standard input";

// DEFAULT_ROOT
//   constant: where runs are saved when --root is not given: the folder the
// window saves in by default.
const char* const DEFAULT_ROOT = "vparse_runs";

// RESPONSE_FILES
//   constant: what the root's help ends with: how an `@FILE`, which the
// kit's options list (GRAMMAR.md, R14), is split, and why vparse reads one.
const char* const RESPONSE_FILES =
    "An @FILE is split as GCC splits a response file -- as cl splits one, "
    "on Windows -- and an argument after -- is never read so: a match over "
    "thousands of inputs is not held to a command line's length.";

// CHECK_ANSWERS ... COMPARE_ANSWERS
//   constants: what each subcommand's exit status answers, and what -q
// leaves out of what it prints, as its help ends (GRAMMAR.md, section 8: a
// command documents what it answers).
const char* const CHECK_ANSWERS =
    "Exit status: 0 when the grammar is accepted; 1 when it is refused, the "
    "reason said as an error; 2 for trouble. -q prints nothing but errors: "
    "the status says whether the grammar was accepted.";
const char* const COMPILE_ANSWERS =
    "Exit status: 0 when the program is written; 2 when the grammar is "
    "refused -- the reason said as an error -- and for any other trouble.";
const char* const MATCH_ANSWERS =
    "Exit status: 0 when every input matched; 1 when one did not; 2 when "
    "the grammar is refused -- the reason said as an error -- and for any "
    "other trouble. -q prints nothing but errors: the status says whether "
    "every input matched, and an input that did not is not run again to "
    "say where it stopped.";
const char* const RUN_ANSWERS =
    "Exit status: 0 when the input matched; 1 when it did not; 2 when the "
    "grammar is refused -- the run printed all the same, and the reason "
    "said as an error -- and for any other trouble. -q prints the parts "
    "alone, without the run's headline; JSON is printed whole.";
const char* const REPLAY_ANSWERS =
    "Exit status: as run's, for the saved input: 0 when it matched; 1 when "
    "it did not; 2 when the grammar is refused, and for any other trouble, "
    "a folder that is not a saved run among them. -q prints the parts "
    "alone, without the run's headline; JSON is printed whole.";
const char* const SAVED_ANSWERS =
    "Exit status: 0, also when there are no runs; 2 for trouble.";
const char* const BENCH_ANSWERS =
    "Exit status: 0 when every run was made, matched or not; 2 when the "
    "grammar is refused, or --memoize names a rule it does not have -- the "
    "reason said as an error -- and for any other trouble.";
const char* const CORPUS_ANSWERS =
    "Exit status: 0, also for a grammar whose start rule no walk can end, "
    "which has no corpus; 2 when the grammar is refused -- the reason said "
    "as an error -- and for any other trouble.";
const char* const COMPARE_ANSWERS =
    "Exit status: 0 when the two programs agree over every input; 1 when "
    "they differ on one; 2 when either grammar is refused, or --memoize "
    "names a rule the second does not have -- the reason said as an error "
    "-- and for any other trouble. -q prints nothing but errors: the status "
    "says whether the two agreed.";

/**
 * @brief Adds the options that name a grammar and say how it is read.
 *
 * @param[in,out] _command      the subcommand.
 * @param[in]     _names        the grammar's operand's name: GRAMMAR, or
 *                              compare's FIRST.
 * @param[in]     _description  what help says the grammar is.
 * @param[out]    _source       receives the options.
 */
void add_source(
    struct dj_arg_command* _command,
    const char*            _names,
    const char*            _description,
    source_options&        _source
)
{
    _source.grammar  = d_arg_command_add_option(
        _command,
        _names,
        _description);
    _source.frontend = d_arg_command_add_option(
        _command,
        "-f,--frontend",
        "how the grammar is read: notation, vparse's own, or ebnf; by "
        "default ebnf for a file whose name ends in .ebnf, and notation "
        "otherwise");
    _source.ebnf     = d_arg_command_add_flag(
        _command,
        "--ebnf",
        "read the grammar as EBNF: the same as --frontend ebnf");

    d_arg_option_set_required(_source.grammar,
                              true);
    (void)d_arg_option_set_type_name(_source.grammar,
                                     "FILE");
    (void)d_arg_option_set_type_name(_source.frontend,
                                     "FRONTEND");
    (void)d_arg_option_check(_source.frontend,
                             d_arg_validator_is_member(
                                 nullptr,
                                 FRONTENDS,
                                 sizeof(FRONTENDS) / sizeof(FRONTENDS[0]),
                                 nullptr),
                             nullptr);
    (void)d_arg_option_excludes(_source.ebnf,
                                _source.frontend);

    return;
}

/**
 * @brief Adds the options that say how the machine runs.
 *
 * @param[in,out] _command      the subcommand.
 * @param[in]     _no_trace     what --no-trace says, for a subcommand that
 *                              takes it; NULL for one that does not.
 * @param[out]    _run          receives the options.
 */
void add_run(
    struct dj_arg_command* _command,
    const char*            _no_trace,
    run_options&           _run
)
{
    _run.step_limit = d_arg_command_add_option(
        _command,
        "--step-limit",
        "stop the machine's run over an input after this many steps; a "
        "grammar is read under a limit of its own, which grows with it");

    (void)d_arg_option_set_type_name(_run.step_limit,
                                     "STEPS");
    (void)d_arg_option_check(_run.step_limit,
                             d_arg_validator_range_long(1L,
                                                        STEP_LIMIT_MOST),
                             nullptr);
    (void)d_arg_option_set_default_value(
        _run.step_limit,
        std::to_string(wb::run_request().step_limit).c_str());

    // the trace, where it is the subcommand's to leave out
    if (_no_trace != nullptr)
    {
        _run.no_trace = d_arg_command_add_flag(_command,
                                               "--no-trace",
                                               _no_trace);
    }

    return;
}

/**
 * @brief Adds the option that names the rules the program calls through the
 *        memo.
 *
 * @param[in,out] _command  the subcommand.
 * @param[in]     _whose    what help says the setting is for: the program,
 *                          or the second of two.
 * @param[out]    _run      receives the option, as its `memoize`.
 */
void add_memoize(
    struct dj_arg_command* _command,
    const std::string&     _whose,
    run_options&           _run
)
{
    const std::string help =
        "call rules of " + _whose + " through the memo, which keeps each "
        "rule's result at each position and replays it: none; all; auto, "
        "the rules a run without the memo says it pays for; or rule names, "
        "the option given again or the names separated by commas. A name "
        "after names is added to them; anything else replaces what came "
        "before it";

    _run.memoize = d_arg_command_add_option(_command,
                                            "--memoize",
                                            help.c_str());

    (void)d_arg_option_set_type_name(_run.memoize,
                                     "RULES");
    d_arg_option_set_delimiter(_run.memoize,
                               ',');
    d_arg_option_set_multi_option_policy(_run.memoize,
                                         D_ARG_MULTI_TAKE_ALL);
    (void)d_arg_option_set_default_value(_run.memoize,
                                         wb::run_request().memoize.c_str());

    return;
}

/**
 * @brief Adds one of the options that say how a corpus is made: a whole
 *        number in a range, with a default.
 *
 * @param[in,out] _command      the subcommand.
 * @param[in]     _name         the option's name.
 * @param[in]     _description  what help says it is.
 * @param[in]     _least        the least it takes.
 * @param[in]     _most         the most it takes.
 * @param[in]     _default      what it is when not given.
 * @return the option.
 */
struct dj_arg_option* add_number(
    struct dj_arg_command* _command,
    const char*            _name,
    const char*            _description,
    long                   _least,
    long                   _most,
    long                   _default
)
{
    struct dj_arg_option* const number = d_arg_command_add_option(
        _command,
        _name,
        _description);

    (void)d_arg_option_set_type_name(number,
                                     "N");
    (void)d_arg_option_check(number,
                             d_arg_validator_range_long(_least,
                                                        _most),
                             nullptr);
    (void)d_arg_option_set_default_value(number,
                                         std::to_string(_default).c_str());

    return number;
}

/**
 * @brief Adds the options that say how a corpus is made from a grammar.
 *
 * @param[in,out] _command  the subcommand.
 * @param[in]     _walks    what help says --walks does on this subcommand.
 * @param[out]    _corpus   receives the options.
 */
void add_corpus(
    struct dj_arg_command* _command,
    const char*            _walks,
    corpus_options&        _corpus
)
{
    const vp::corpus::settings fresh;

    _corpus.walks   = add_number(_command,
                                 "--walks",
                                 _walks,
                                 0L,
                                 CORPUS_MOST,
                                 static_cast<long>(fresh.walks));
    _corpus.depth   = add_number(_command,
                                 "--depth",
                                 "how many rules deep a walk goes, one "
                                 "inside the next, before it takes each "
                                 "rule's shortest way out",
                                 0L,
                                 CORPUS_MOST,
                                 static_cast<long>(fresh.depth));
    _corpus.length  = add_number(_command,
                                 "--length",
                                 "how long a walk's text grows before it "
                                 "takes the shortest way out",
                                 1L,
                                 CORPUS_MOST * 10L,
                                 static_cast<long>(fresh.length));
    _corpus.repeats = add_number(_command,
                                 "--repeats",
                                 "the most times a walk repeats a * or a +",
                                 0L,
                                 CORPUS_MOST,
                                 static_cast<long>(fresh.repeats));
    _corpus.misses  = add_number(_command,
                                 "--misses",
                                 "how many near-misses each walk is copied "
                                 "to: a character deleted, inserted or "
                                 "replaced",
                                 0L,
                                 CORPUS_MOST,
                                 static_cast<long>(fresh.misses));
    _corpus.seed    = add_number(_command,
                                 "--seed",
                                 "the seed the walks are drawn from: one "
                                 "seed, one corpus, on every system",
                                 0L,
                                 SEED_MOST,
                                 static_cast<long>(fresh.seed));

    return;
}

/**
 * @brief Adds the option that says how to print.
 *
 * @param[in,out] _command  the subcommand.
 * @param[out]    _output   receives the option.
 */
void add_output(
    struct dj_arg_command* _command,
    output_options&        _output
)
{
    _output.format = d_arg_command_add_option(
        _command,
        "--format",
        "how to print: text, for reading, or json, for a program");

    (void)d_arg_option_set_type_name(_output.format,
                                     "FORMAT");
    (void)d_arg_option_check(_output.format,
                             d_arg_validator_is_member(
                                 nullptr,
                                 FORMATS,
                                 sizeof(FORMATS) / sizeof(FORMATS[0]),
                                 nullptr),
                             nullptr);
    (void)d_arg_option_set_default_value(_output.format,
                                         FORMATS[0]);

    return;
}

/**
 * @brief Adds the option that names where runs are saved.
 *
 * @param[in,out] _command  the subcommand.
 * @return the option.
 */
struct dj_arg_option* add_root(
    struct dj_arg_command* _command
)
{
    struct dj_arg_option* const root = d_arg_command_add_option(
        _command,
        "--root",
        "the folder runs are saved in, a session folder each time");

    (void)d_arg_option_set_type_name(root,
                                     "DIR");
    (void)d_arg_option_set_default_value(root,
                                         DEFAULT_ROOT);

    return root;
}

/**
 * @brief Adds the options that save runs.
 *
 * @param[in,out] _command  the subcommand.
 * @param[out]    _save     receives the options.
 */
void add_save(
    struct dj_arg_command* _command,
    save_options&          _save
)
{
    _save.save = d_arg_command_add_flag(
        _command,
        "--save",
        "save each run, as the window saves one: its grammar, its input "
        "and every part, in a new session folder under --root");
    _save.root = add_root(_command);

    return;
}

/**
 * @brief Adds the option that names the parts of a run to print.
 *
 * @param[in,out] _command  the subcommand.
 * @return the option.
 *
 * Its help names every part, since the kit's help, as GNU help does, says
 * what an option takes in its words rather than in its validator's.
 */
struct dj_arg_option* add_parts(
    struct dj_arg_command* _command
)
{
    const std::vector<std::string>& known = wb::part_names();
    std::vector<const char*>        names;
    std::string                     help  =
        "the parts of the run to print, in order, the option given again "
        "or the parts separated by commas: ";

    // every part a run has, by name, in its words and its validator's
    for (std::size_t at = 0u; at < known.size(); at++)
    {
        names.push_back(known[at].c_str());
        help += known[at] + ( ((at + 1u) == known.size()) ? "; "
                              : ((at + 2u) == known.size()) ? " or "
                                                            : ", " );
    }

    names.push_back("all");
    help += "all for every part";

    struct dj_arg_option* const parts = d_arg_command_add_option(
        _command,
        "-p,--part",
        help.c_str());

    (void)d_arg_option_set_type_name(parts,
                                     "PART");
    d_arg_option_set_delimiter(parts,
                               ',');
    d_arg_option_set_multi_option_policy(parts,
                                         D_ARG_MULTI_TAKE_ALL);
    (void)d_arg_option_check(parts,
                             d_arg_validator_is_member(nullptr,
                                                       names.data(),
                                                       names.size(),
                                                       nullptr),
                             nullptr);
    (void)d_arg_option_set_default_value(parts,
                                         "summary");

    return parts;
}

/**
 * @brief Adds the input a run reads, or the inputs a match reads.
 *
 * @param[in,out] _command      the subcommand.
 * @param[in]     _many         whether it takes one input or more.
 * @param[in]     _required     whether, taking more, it needs one at least.
 * @param[in]     _description  what help says the input is.
 * @return the option.
 */
struct dj_arg_option* add_input(
    struct dj_arg_command* _command,
    bool                   _many,
    bool                   _required,
    const char*            _description
)
{
    struct dj_arg_option* const input = d_arg_command_add_option(
        _command,
        "INPUT",
        _description);

    (void)d_arg_option_set_type_name(input,
                                     "FILE");

    // a match runs every input given, positional or named
    if (_many)
    {
        d_arg_option_set_expected_range(input,
                                        1,
                                        D_ARG_EXPECTED_UNLIMITED);
        d_arg_option_set_multi_option_policy(input,
                                             D_ARG_MULTI_TAKE_ALL);
        d_arg_option_set_required(input,
                                  _required);
    }

    return input;
}

/**
 * @brief Adds the switch that reads each input as text.
 *
 * @param[in,out] _command  the subcommand.
 * @return the option.
 */
struct dj_arg_option* add_text(
    struct dj_arg_command* _command
)
{
    return d_arg_command_add_flag(
        _command,
        "-t,--text",
        "read each input argument as the input itself, not as a file name");
}


/**
 * @brief Adds a subcommand to the program: its work runs its handler over
 *        its record, and its help ends with what its exit status answers.
 *
 * @tparam Record  the subcommand's record (commands.hpp).
 * @param[in,out] _program      the program.
 * @param[in]     _name         the subcommand's name.
 * @param[in]     _description  what it does, in lower case and with no full
 *                              stop, as the command list shows it.
 * @param[in]     _answers      what its exit status answers, as its help
 *                              ends.
 * @param[in]     _handler      its handler (commands.hpp, section 3).
 * @param[in]     _context      what the handler reads and writes; it must
 *                              outlive the program's run.
 * @param[in,out] _record       its record, whose `command` receives the
 *                              subcommand; it must outlive the program's run.
 *
 * A subcommand the kit could not add is a NULL command, whose options are
 * then made of nothing; the kit keeps the failure, and the run ends in
 * trouble and says why.
 */
template<typename Record>
void add_subcommand(
    kit::program&  _program,
    const char*    _name,
    const char*    _description,
    const char*    _answers,
    int (*_handler)(const Record&, const context&),
    const context& _context,
    Record&        _record
)
{
    const Record* const  record = &_record;
    const context* const io     = &_context;

    _record.command = _program.add_command(
        _name,
        _description,
        [record, io, _handler](kit::program&, kit::command)
        {
            return _handler(*record,
                            *io);
        }).get();

    (void)d_arg_command_set_footer(_record.command,
                                   _answers);

    return;
}

/**
 * @brief Makes `check`, which reads a grammar and judges it.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _check    receives its record.
 */
void define_check(
    kit::program&  _program,
    const context& _context,
    check_command& _check
)
{
    add_subcommand(_program,
                   "check",
                   "read a grammar and judge it, as the session judges it",
                   CHECK_ANSWERS,
                   &subcommand::check,
                   _context,
                   _check);
    add_source(_check.command,
               "GRAMMAR",
               GRAMMAR_HELP,
               _check.source);
    add_run(_check.command,
            nullptr,
            _check.run);
    add_output(_check.command,
               _check.output);

    return;
}

/**
 * @brief Makes `compile`, which writes a grammar's program out.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _compile  receives its record.
 */
void define_compile(
    kit::program&    _program,
    const context&   _context,
    compile_command& _compile
)
{
    add_subcommand(_program,
                   "compile",
                   "build a grammar's program and write it out, an "
                   "instruction a line",
                   COMPILE_ANSWERS,
                   &subcommand::compile,
                   _context,
                   _compile);
    add_source(_compile.command,
               "GRAMMAR",
               GRAMMAR_HELP,
               _compile.source);

    _compile.output_file = d_arg_command_add_option(
        _compile.command,
        "-o,--output",
        "write the program to this file, whole or not at all; - for "
        "standard output, as without it");

    (void)d_arg_option_set_type_name(_compile.output_file,
                                     "FILE");

    return;
}

/**
 * @brief Makes `match`, which runs a grammar over each input.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _match    receives its record.
 */
void define_match(
    kit::program&  _program,
    const context& _context,
    match_command& _match
)
{
    add_subcommand(_program,
                   "match",
                   "run a grammar over each input, and say whether it "
                   "matched",
                   MATCH_ANSWERS,
                   &subcommand::match,
                   _context,
                   _match);
    add_source(_match.command,
               "GRAMMAR",
               GRAMMAR_HELP,
               _match.source);
    _match.inputs = add_input(_match.command,
                              true,
                              true,
                              "the inputs: files, or - for standard input");
    _match.text   = add_text(_match.command);
    add_run(_match.command,
            "never record the trace: an input that does not match is not "
            "run again to say where it stopped",
            _match.run);
    add_memoize(_match.command,
                "the program",
                _match.run);
    add_output(_match.command,
               _match.output);
    add_save(_match.command,
             _match.save);

    return;
}

/**
 * @brief Makes `run`, which runs a grammar over an input and prints the
 *        parts of the run.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _run      receives its record.
 */
void define_run(
    kit::program&  _program,
    const context& _context,
    run_command&   _run
)
{
    add_subcommand(_program,
                   "run",
                   "run a grammar over an input, and print the parts of the "
                   "run",
                   RUN_ANSWERS,
                   &subcommand::run,
                   _context,
                   _run);
    add_source(_run.command,
               "GRAMMAR",
               GRAMMAR_HELP,
               _run.source);
    _run.inputs = add_input(_run.command,
                            false,
                            false,
                            "the input: a file, or - for standard input; with "
                            "none, the grammar runs over nothing");
    _run.text   = add_text(_run.command);
    _run.parts  = add_parts(_run.command);
    add_run(_run.command,
            "do not record the trace: faster, but a run that fails cannot "
            "say where it stopped, and has no heat, no matches and no trace "
            "to print",
            _run.run);
    add_memoize(_run.command,
                "the program",
                _run.run);
    add_output(_run.command,
               _run.output);
    add_save(_run.command,
             _run.save);

    return;
}

/**
 * @brief Makes `replay`, which runs a saved run again, and `saved`, which
 *        lists the runs saved under a folder.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what their handlers read and write.
 * @param[out]    _replay   receives replay's record.
 * @param[out]    _saved    receives saved's record.
 */
void define_stores(
    kit::program&   _program,
    const context&  _context,
    replay_command& _replay,
    saved_command&  _saved
)
{
    add_subcommand(_program,
                   "replay",
                   "run a saved run again, and print its parts",
                   REPLAY_ANSWERS,
                   &subcommand::replay,
                   _context,
                   _replay);
    add_subcommand(_program,
                   "saved",
                   "list the runs saved under a folder, newest first",
                   SAVED_ANSWERS,
                   &subcommand::saved,
                   _context,
                   _saved);

    _replay.folder = d_arg_command_add_option(
        _replay.command,
        "FOLDER",
        "a saved run's folder, as saved lists them");

    d_arg_option_set_required(_replay.folder,
                              true);
    (void)d_arg_option_set_type_name(_replay.folder,
                                     "DIR");
    _replay.parts = add_parts(_replay.command);
    add_output(_replay.command,
               _replay.output);

    _saved.root = add_root(_saved.command);
    add_output(_saved.command,
               _saved.output);

    return;
}

/**
 * @brief Makes `bench`, which counts and times a grammar's runs over many
 *        inputs.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _bench    receives its record.
 */
void define_bench(
    kit::program&  _program,
    const context& _context,
    bench_command& _bench
)
{
    add_subcommand(_program,
                   "bench",
                   "run a grammar over many inputs, counting each run and "
                   "timing it",
                   BENCH_ANSWERS,
                   &subcommand::bench,
                   _context,
                   _bench);
    add_source(_bench.command,
               "GRAMMAR",
               GRAMMAR_HELP,
               _bench.source);
    _bench.inputs = add_input(_bench.command,
                              true,
                              false,
                              "the inputs: files, or - for standard input; "
                              "with none, the grammar's corpus");
    _bench.text   = add_text(_bench.command);
    add_corpus(_bench.command,
               "run the grammar's corpus too, of this many walks: by "
               "default, only where no input is given",
               _bench.corpus);
    add_run(_bench.command,
            nullptr,
            _bench.run);
    add_memoize(_bench.command,
                "the program, over the whole corpus",
                _bench.run);
    _bench.times = add_number(_bench.command,
                              "--times",
                              "how many runs of each input are timed; the "
                              "fastest is the time, and the steps, which "
                              "repeat exactly, are counted once",
                              1L,
                              TIMES_MOST,
                              DEFAULT_TIMES);
    add_output(_bench.command,
               _bench.output);

    return;
}

/**
 * @brief Makes `corpus`, which makes inputs from a grammar.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _corpus   receives its record.
 */
void define_corpus(
    kit::program&   _program,
    const context&  _context,
    corpus_command& _corpus
)
{
    add_subcommand(_program,
                   "corpus",
                   "make inputs from a grammar: random walks of its rules, "
                   "and near-misses of them",
                   CORPUS_ANSWERS,
                   &subcommand::corpus,
                   _context,
                   _corpus);
    add_source(_corpus.command,
               "GRAMMAR",
               GRAMMAR_HELP,
               _corpus.source);
    add_corpus(_corpus.command,
               "how many random walks the corpus has, each with its "
               "near-misses",
               _corpus.corpus);

    _corpus.folder = d_arg_command_add_option(
        _corpus.command,
        "-o,--output",
        "write each input to a file of its own in this folder, made if it "
        "is not there, rather than list them");

    (void)d_arg_option_set_type_name(_corpus.folder,
                                     "DIR");
    add_output(_corpus.command,
               _corpus.output);

    return;
}

/**
 * @brief Makes `compare`, which runs two grammars over the same inputs.
 *
 * @param[in,out] _program  the program.
 * @param[in]     _context  what its handler reads and writes.
 * @param[out]    _compare  receives its record.
 */
void define_compare(
    kit::program&    _program,
    const context&   _context,
    compare_command& _compare
)
{
    add_subcommand(_program,
                   "compare",
                   "run two grammars over the same inputs, and name each "
                   "input they say different things of",
                   COMPARE_ANSWERS,
                   &subcommand::compare,
                   _context,
                   _compare);
    add_source(_compare.command,
               "FIRST",
               "the grammar compared against: a file, or - for standard "
               "input",
               _compare.source);

    _compare.second = d_arg_command_add_option(
        _compare.command,
        "SECOND",
        "the grammar compared: a file, or - for standard input");

    d_arg_option_set_required(_compare.second,
                              true);
    (void)d_arg_option_set_type_name(_compare.second,
                                     "FILE");
    _compare.inputs = add_input(_compare.command,
                                true,
                                false,
                                "the inputs: files, or - for standard input; "
                                "with none, the first grammar's corpus");
    _compare.text   = add_text(_compare.command);
    add_corpus(_compare.command,
               "run the first grammar's corpus too, of this many walks: by "
               "default, only where no input is given",
               _compare.corpus);
    add_run(_compare.command,
            nullptr,
            _compare.run);
    add_memoize(_compare.command,
                "the second program",
                _compare.run);
    add_output(_compare.command,
               _compare.output);

    return;
}

/**
 * @brief Makes the whole command tree on the program: the nine commands,
 *        and `gui`, the window's module (GRAMMAR.md, section 7).
 *
 * @param[in,out] _program  the program, made.
 * @param[in]     _context  what the handlers read and write.
 * @param[out]    _line     receives a record for each subcommand.
 */
void define(
    kit::program&  _program,
    const context& _context,
    command_line&  _line
)
{
    define_check(_program,
                 _context,
                 _line.check);
    define_compile(_program,
                   _context,
                   _line.compile);
    define_match(_program,
                 _context,
                 _line.match);
    define_run(_program,
               _context,
               _line.run);
    define_stores(_program,
                  _context,
                  _line.replay,
                  _line.saved);
    define_bench(_program,
                 _context,
                 _line.bench);
    define_corpus(_program,
                  _context,
                  _line.corpus);
    define_compare(_program,
                   _context,
                   _line.compare);

    // the window: a module, found and loaded only when it is run
    (void)_program.add_plugin("gui",
                              "open the workbench on a grammar and an "
                              "input, or on its example");

    return;
}

}  // anonymous namespace

/*
run
  The program is made, used and released here, so that every run of the
command line starts from nothing, a suite's runs included. Its output goes
to the context's sinks from the start, so that its help and its failures
land where the subcommands' output does; and the context its subcommands
are handed is the caller's, with the program in it, which says their
diagnostics.
*/
int run(
    int                _argc,
    const char* const* _argv,
    const context&     _context
)
{
    kit::program program(PROGRAM,
                         VPARSE_VERSION,
                         DESCRIPTION,
                         FLAGS);
    command_line line;
    context      io = _context;

    (void)program.output(_context.out,
                         _context.err);
    io.program = &program.kit();
    define(program,
           io,
           line);
    (void)d_arg_command_set_footer(&program.kit().parser->command,
                                   RESPONSE_FILES);

    return program.run(_argc,
                       _argv);
}

/*
version
  A literal, so it outlives every caller.
*/
const char* version()
{
    return VERSION_TEXT;
}

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now
