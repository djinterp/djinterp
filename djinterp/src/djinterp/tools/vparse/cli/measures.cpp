/*******************************************************************************
* vparse [cli]                                                      measures.cpp
*
* Definitions for commands.hpp's measures: bench, corpus and compare, which
* run grammars over many inputs -- those given, the grammar's own corpus
* (corpus.hpp), or both -- and say what the runs did.
*   A grammar is judged and built once, by the model's execute() over no
* input, so a measure refuses a grammar in the words every other subcommand
* refuses one in, said as an error. The runs over the inputs are then the
* library's own, untraced: each counted by its profile, and timed apart from
* it. Steps repeat exactly, run after run and machine after machine; time
* does not, so an input's time is the fastest of several runs, and the two
* are never mixed.
*   --memoize auto chooses once, for the whole corpus: every input is run
* without the memo and counted, the profiles are added, and the rules the
* sum says a memo pays for are the ones the program measured calls through
* it. compare's memo is the second program's, so that a program and its
* memoized self can be held to each other.
*   LEVELS. The command line's floor is the model's, C++17, so this file's
* is: below it the file is empty rather than an error.
*
*
* path:      /src/djinterp/tools/vparse/cli/measures.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "./commands.hpp"  // the records, subcommand::*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <algorithm>     // std::max, std::min
#include <chrono>        // std::chrono::steady_clock
#include <cstddef>       // std::size_t
#include <cstdio>        // std::snprintf
#include <filesystem>    // std::filesystem
#include <string>        // std::string, std::to_string
#include <system_error>  // std::error_code
#include <vector>        // std::vector
// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // dj_arg_option
#include "djinterp/cpp/modules/parsegen/vparse/corpus.hpp"           // generate, compare
#include "djinterp/cpp/modules/parsegen/vparse/ebnf.hpp"             // ebnf::parse
#include "djinterp/cpp/modules/parsegen/vparse/peg.hpp"              // run, profile,
                                                         // memoized
#include "djinterp/tools/cli/c/cli_io.h"                 // d_cli_write_file
#include "djinterp/tools/cli/c/cli_program.h"            // d_cli_file_error
// vparse
#include "./common.hpp"  // options, reading, endings, requests
#include "./report.hpp"  // json_object, json_array, json_strings, clipped
#include "../../../../../inc/djinterp/tools/vparse/run.hpp"    // execute,
                                                               // memo_rules
#include "../../../../../inc/djinterp/tools/vparse/store.hpp"  // listing_text,
                                                               // UTF-8 paths


namespace vparse
{
namespace cli
{

namespace
{

namespace fs = std::filesystem;
namespace pr = ::djinterp::parse;
namespace vp = ::djinterp::parsegen::vparse;
namespace wb = ::vparse::workbench;

using steady = std::chrono::steady_clock;

// SHOWN_MOST
//   constant: the most bytes of a corpus input that its name shows.
const std::size_t SHOWN_MOST = 40u;

// TIMES_FALLBACK
//   constant: how many runs of each input are timed where --times cannot be
// read; the option's own default is the same.
const long TIMES_FALLBACK = 3L;

// item
//   struct: one input a measure runs: the name a reader knows it by, the
// argument that named it (the text itself, for an input the corpus made),
// its bytes, and where it came from: "given", "walk" or "miss".
struct item
{
    std::string name;
    std::string argument;
    std::string bytes;
    std::string kind;
};

// measured
//   struct: one input's run: what the program said of it, what the run
// counted, and the fastest time of the runs timed.
struct measured
{
    vp::corpus::verdict verdict;
    vp::peg::profile    counts;
    double              ms = 0.0;
};

// bench_report
//   struct: what bench found: the grammar's name and the frontend that read
// it, the memo setting and the rules it chose, the steps the inputs took
// without the memo where auto chose, each input and its run, the runs'
// profiles added, and the totals.
struct bench_report
{
    std::string              grammar;
    std::string              frontend;
    std::string              memo;
    std::vector<std::string> memoized;
    long                     without    = -1;
    std::vector<item>        items;
    std::vector<measured>    runs;
    vp::peg::profile         total;
    long                     steps      = 0;
    double                   ms         = 0.0;
    std::size_t              matched    = 0u;
    std::size_t              unfinished = 0u;
    long                     times      = 1;
};

// compare_report
//   struct: what compare found: each grammar's name and frontend, the memo
// setting and the rules it chose for the second program, the inputs, and
// the comparison.
struct compare_report
{
    std::string              first;
    std::string              second;
    std::string              first_frontend;
    std::string              second_frontend;
    std::string              memo;
    std::vector<std::string> memoized;
    std::vector<item>        items;
    vp::corpus::comparison   found;
};

//------------------------------------------------------------------------------
// the corpus
//------------------------------------------------------------------------------

/**
 * @brief Reads how a subcommand makes its corpus.
 *
 * @param[in] _corpus  the subcommand's corpus options.
 * @return the settings, each option's value, or its default.
 */
vp::corpus::settings settings_of(
    const corpus_options& _corpus
)
{
    const vp::corpus::settings fresh;
    vp::corpus::settings       out;

    out.walks   = static_cast<std::size_t>(
                      number_of(_corpus.walks,
                                static_cast<long>(fresh.walks)));
    out.depth   = static_cast<std::size_t>(
                      number_of(_corpus.depth,
                                static_cast<long>(fresh.depth)));
    out.length  = static_cast<std::size_t>(
                      number_of(_corpus.length,
                                static_cast<long>(fresh.length)));
    out.repeats = static_cast<std::size_t>(
                      number_of(_corpus.repeats,
                                static_cast<long>(fresh.repeats)));
    out.misses  = static_cast<std::size_t>(
                      number_of(_corpus.misses,
                                static_cast<long>(fresh.misses)));
    out.seed    = static_cast<re_std::uint64_t>(
                      number_of(_corpus.seed,
                                static_cast<long>(fresh.seed)));

    return out;
}

/**
 * @brief Makes a built grammar's corpus.
 *
 * @param[in] _built     the run that read and built the grammar.
 * @param[in] _settings  how to make it.
 * @return the samples generate() makes from the grammar's rules, as the
 *         frontend that read it reads them.
 */
std::vector<vp::corpus::sample> corpus_of(
    const wb::run_record&       _built,
    const vp::corpus::settings& _settings
)
{
    // the notation's rules are the run's
    if (_built.request.frontend != "ebnf")
    {
        return vp::corpus::generate(_built.rules,
                                    _settings);
    }

    bool       read   = false;
    const auto parsed = vp::ebnf::parse(_built.request.grammar,
                                        &read);

    return read ? vp::corpus::generate(parsed.first,
                                       _settings)
                : std::vector<vp::corpus::sample>();
}

/**
 * @brief Lists what a measure runs: the inputs given, then the corpus.
 *
 * @param[in] _given  the inputs given, in order.
 * @param[in] _made   the corpus, in order.
 * @return each as an item, a corpus input named by its text, clipped.
 */
std::vector<item> items_of(
    const std::vector<source>&             _given,
    const std::vector<vp::corpus::sample>& _made
)
{
    std::vector<item> out;

    // the inputs given, by the names they were given by
    for (const source& each : _given)
    {
        out.push_back(item{ each.name,
                            each.argument,
                            each.bytes,
                            "given" });
    }

    // the corpus, by its texts
    for (const vp::corpus::sample& each : _made)
    {
        out.push_back(item{ "'" + clipped(wb::escape(each.text),
                                          SHOWN_MOST) + "'",
                            each.text,
                            each.text,
                            each.derived ? "walk" : "miss" });
    }

    return out;
}

/**
 * @brief Determines whether a measure runs its grammar's corpus.
 *
 * @param[in] _inputs  the inputs given.
 * @param[in] _walks   the subcommand's --walks.
 * @return whether no input was given, or --walks was.
 */
bool runs_corpus(
    const std::vector<source>& _inputs,
    struct dj_arg_option*      _walks
)
{
    return ( (_inputs.empty()) ||
             (given(_walks)) );
}

/**
 * @brief Counts a measure's inputs by where they came from, for a sentence.
 *
 * @param[in] _items  the inputs.
 * @return "N inputs", and, where the corpus made some, how many were walks
 *         and how many near-misses, in parentheses.
 */
std::string counted_inputs(
    const std::vector<item>& _items
)
{
    std::size_t walks  = 0u;
    std::size_t misses = 0u;

    // each made input, by kind
    for (const item& each : _items)
    {
        walks  += (each.kind == "walk") ? 1u : 0u;
        misses += (each.kind == "miss") ? 1u : 0u;
    }

    std::string out = std::to_string(_items.size()) +
                      ((_items.size() == 1u) ? " input" : " inputs");

    // the corpus's share
    if ( (walks != 0u) ||
         (misses != 0u) )
    {
        out += " (" + std::to_string(walks) + " walks, " +
               std::to_string(misses) + " near-misses)";
    }

    return out;
}

//------------------------------------------------------------------------------
// runs
//------------------------------------------------------------------------------

/**
 * @brief Gives the PEG operators, made once.
 *
 * @return the registry.
 */
const vp::op_set& operators()
{
    static const vp::op_set ops = vp::peg::make_ops();

    return ops;
}

/**
 * @brief Runs a program over an input, counted once and timed apart.
 *
 * @param[in] _program  the program.
 * @param[in] _input    the input.
 * @param[in] _limit    the step limit.
 * @param[in] _times    how many runs to time; at least 1.
 * @return what it said, what it counted, and the fastest timed run.
 */
measured measure(
    const vp::peg::program& _program,
    const std::string&      _input,
    long                    _limit,
    long                    _times
)
{
    measured              out;
    pr::parse_state<char> counted(_input.data(),
                                  _input.size(),
                                  0);
    vp::machine           machine;
    vp::peg::sinks        sinks;

    machine.step_limit = _limit;
    sinks.counts       = &out.counts;

    out.verdict.ok       = vp::peg::run(machine,
                                        counted,
                                        _program,
                                        operators(),
                                        sinks);
    out.verdict.consumed = out.verdict.ok ? counted.offset
                                          : 0u;
    out.verdict.steps    = machine.steps;
    out.verdict.finished = machine.halted;

    // each timed run, uncounted, the fastest kept
    for (long round = 0; round < _times; round++)
    {
        pr::parse_state<char>    timed(_input.data(),
                                       _input.size(),
                                       0);
        vp::machine              clocked;
        const steady::time_point start = steady::now();

        clocked.step_limit = _limit;
        (void)vp::peg::run(clocked,
                           timed,
                           _program,
                           operators());

        const double ms = std::chrono::duration<double, std::milli>(
                              steady::now() - start).count();

        out.ms = (round == 0) ? ms
                              : std::min(out.ms,
                                         ms);
    }

    return out;
}

/**
 * @brief Works out the rules a measured program calls through the memo.
 *
 * @param[in]  _built    the run that built the program, its entries
 *                       labelled.
 * @param[in]  _items    the inputs the program will run over.
 * @param[in]  _setting  the --memoize value.
 * @param[in]  _limit    the step limit.
 * @param[out] _unknown  receives a name the setting gives that no rule has.
 * @param[out] _without  receives, for "auto", the steps the inputs took
 *                       without the memo; left alone otherwise.
 * @return the rules, by their first addresses; none where a name is
 *         unknown.
 *
 * "auto" counts every input's run without the memo and chooses from the sum
 * (peg::accumulate, peg::worth_memoizing), so that one choice serves the
 * whole corpus, as one program does.
 */
std::vector<int> memo_for(
    const wb::run_record&    _built,
    const std::vector<item>& _items,
    const std::string&       _setting,
    long                     _limit,
    std::string&             _unknown,
    long&                    _without
)
{
    // names, all, or none
    if (_setting != "auto")
    {
        return wb::memo_rules(_built,
                              _setting,
                              &_unknown);
    }

    vp::peg::profile total;

    _without = 0;

    // every input, without the memo, counted
    for (const item& each : _items)
    {
        const measured run = measure(_built.program,
                                     each.bytes,
                                     _limit,
                                     0);

        vp::peg::accumulate(total,
                            run.counts);
        _without += run.verdict.steps;
    }

    return vp::peg::worth_memoizing(total);
}

/**
 * @brief Names a measured program's memoized rules.
 *
 * @param[in] _built   the run that built the program.
 * @param[in] _starts  the rules, by their first addresses.
 * @return their names, in the order given.
 */
std::vector<std::string> names_of(
    const wb::run_record&   _built,
    const std::vector<int>& _starts
)
{
    std::vector<std::string> out;

    // each by the label at its start
    for (const int start : _starts)
    {
        const std::size_t at = static_cast<std::size_t>(start);

        out.push_back( (at < _built.entry.size()) &&
                       (!_built.entry[at].empty())
                           ? _built.entry[at]
                           : "@" + std::to_string(start));
    }

    return out;
}

//------------------------------------------------------------------------------
// what is printed
//------------------------------------------------------------------------------

/**
 * @brief Writes milliseconds as text: three decimals.
 *
 * @param[in] _ms  the time.
 * @return the number, for text and for JSON alike.
 */
std::string ms_of(
    double _ms
)
{
    char text[32] = {};

    (void)std::snprintf(text,
                        sizeof(text),
                        "%.3f",
                        _ms);

    return std::string(text);
}

/**
 * @brief Says what a run said of its input, in a word or three.
 *
 * @param[in] _verdict  the run's verdict.
 * @return "matched", "did not match", or "ran out of steps".
 */
std::string said(
    const vp::corpus::verdict& _verdict
)
{
    // the step limit stopped it
    if (!_verdict.finished)
    {
        return "ran out of steps";
    }

    return _verdict.ok ? "matched"
                       : "did not match";
}

/**
 * @brief Says what the memo did over a measure's inputs.
 *
 * @param[in] _report  what bench found.
 * @return the sentence, its period included.
 */
std::string memo_sentence(
    const bench_report& _report
)
{
    const bool chose = (_report.memo == "auto");

    // none asked for
    if ( (_report.memo.empty()) ||
         (_report.memo == "none") )
    {
        return "Memo: none.";
    }

    // asked for, and nothing to call through it
    if (_report.memoized.empty())
    {
        return chose ? "Memo: auto -- no rule repeats enough work to pay "
                       "for it."
                     : "Memo: none.";
    }

    std::string out = "Memo: " + std::string(chose ? "auto -- " : "") +
                      joined(_report.memoized) + "; " +
                      std::to_string(_report.total.memo_entries) +
                      " results kept at most, " +
                      std::to_string(_report.total.memo_hits) +
                      " calls answered";

    // where a run without it was counted
    if (_report.without >= 0)
    {
        out += "; " + std::to_string(_report.without) + " steps without it";
    }

    return out + ".";
}

/**
 * @brief Lays out bench's inputs, a row each.
 *
 * @param[in] _report  what bench found.
 * @return the listing: an input, what the program said, its steps, its
 *         revisits, and its time.
 */
wb::listing inputs_listing(
    const bench_report& _report
)
{
    wb::listing out;

    out.columns = { "input", "result", "steps", "revisits", "ms" };
    out.note    = "No input was run.";

    // a row an input, in order
    for (std::size_t at = 0u; at < _report.items.size(); at++)
    {
        const measured& run = _report.runs[at];

        out.rows.push_back({ _report.items[at].name,
                             said(run.verdict),
                             std::to_string(run.verdict.steps),
                             std::to_string(run.counts.revisits),
                             ms_of(run.ms) });
        out.marks.push_back(run.verdict.finished ? 0 : 2);
    }

    return out;
}

/**
 * @brief Lays out bench's rules, a row each, over every input.
 *
 * @param[in] _report  what bench found.
 * @param[in] _built   the run that built the program, for the rules' names.
 * @return the listing, as a run's profile part lays one out.
 */
wb::listing rules_listing(
    const bench_report&   _report,
    const wb::run_record& _built
)
{
    wb::listing out;

    out.columns = { "rule", "calls", "repeats", "repeat steps", "memo hits",
                    "steps", "arrivals", "revisits" };
    out.note    = "No input was run, so nothing was counted.";

    // a row a rule, in program order
    for (const vp::peg::profile::DRuleCounts& each : _report.total.rules)
    {
        out.rows.push_back({ names_of(_built,
                                      { each.start })[0],
                             std::to_string(each.calls),
                             std::to_string(each.repeats),
                             std::to_string(each.repeat_steps),
                             std::to_string(each.hits),
                             std::to_string(each.steps),
                             std::to_string(each.arrivals),
                             std::to_string(each.revisits) });
        out.marks.push_back((each.repeats > 0u) ? 2 : 0);
    }

    return out;
}

/**
 * @brief Says what bench found, as text.
 *
 * @param[in] _report  what bench found.
 * @param[in] _built   the run that built the program.
 * @return the totals, a line each, then the inputs and the rules under
 *         their titles.
 */
std::string bench_text(
    const bench_report&   _report,
    const wb::run_record& _built
)
{
    const vp::peg::profile& total = _report.total;
    std::string             out   =
        _report.grammar + " over " + counted_inputs(_report.items) + ": " +
        std::to_string(_report.matched) + " matched";

    // runs the step limit stopped
    if (_report.unfinished != 0u)
    {
        out += ", " + std::to_string(_report.unfinished) +
               " ran out of steps";
    }

    out += ".\nSteps: " + std::to_string(_report.steps) + " in all; " +
           std::to_string(total.revisits) + " revisits; at most " +
           std::to_string(total.peak_calls) + " calls, " +
           std::to_string(total.peak_back) + " backtrack points, " +
           std::to_string(total.peak_marks) + " marks, " +
           std::to_string(total.peak_caps) + " captures.\n" +
           memo_sentence(_report) + "\nTime: " + ms_of(_report.ms) +
           " ms in all, the fastest of " + std::to_string(_report.times) +
           ((_report.times == 1) ? " run" : " runs") + " of each input.\n";

    out += "\nInputs\n------\n" +
           wb::listing_text(inputs_listing(_report)) +
           "\nRules\n-----\n" +
           wb::listing_text(rules_listing(_report,
                                          _built));

    return out;
}

/**
 * @brief Says what bench found, as JSON.
 *
 * @param[in] _report  what bench found.
 * @param[in] _built   the run that built the program.
 * @return the JSON document, and a newline.
 */
std::string bench_json(
    const bench_report&   _report,
    const wb::run_record& _built
)
{
    const vp::peg::profile& total = _report.total;
    json_array              rules;
    json_array              runs;
    json_object             peaks;
    json_object             memo;

    // a record a rule
    for (const vp::peg::profile::DRuleCounts& each : total.rules)
    {
        json_object rule;

        rule.add_string("rule",
                        names_of(_built,
                                 { each.start })[0])
            .add_count("calls",
                       each.calls)
            .add_count("repeats",
                       each.repeats)
            .add_count("repeat_steps",
                       each.repeat_steps)
            .add_count("hits",
                       each.hits)
            .add_count("steps",
                       each.steps)
            .add_count("arrivals",
                       each.arrivals)
            .add_count("revisits",
                       each.revisits);
        rules.add(rule.line());
    }

    // a record an input
    for (std::size_t at = 0u; at < _report.items.size(); at++)
    {
        const measured& one = _report.runs[at];
        json_object     run;

        run.add_string("input",
                       _report.items[at].argument)
           .add_string("kind",
                       _report.items[at].kind)
           .add_bool("matched",
                     one.verdict.ok)
           .add_count("consumed",
                      one.verdict.consumed)
           .add_count("steps",
                      static_cast<std::size_t>(std::max(0L,
                                                        one.verdict.steps)))
           .add_bool("finished",
                     one.verdict.finished)
           .add_count("revisits",
                      one.counts.revisits)
           .add("ms",
                ms_of(one.ms));
        runs.add(run.line());
    }

    peaks.add_count("calls",
                    total.peak_calls)
         .add_count("back",
                    total.peak_back)
         .add_count("marks",
                    total.peak_marks)
         .add_count("caps",
                    total.peak_caps);
    memo.add_count("entries",
                   total.memo_entries)
        .add_count("hits",
                   total.memo_hits)
        .add_count("misses",
                   total.memo_misses);

    json_object out;

    out.add_string("grammar",
                   _report.grammar)
       .add_string("frontend",
                   _report.frontend)
       .add_bool("accepted",
                 true)
       .add_string("error",
                   std::string())
       .add_string("memoize",
                   _report.memo)
       .add("memoized",
            json_strings(_report.memoized))
       .add_count("inputs",
                  _report.items.size())
       .add_count("matched",
                  _report.matched)
       .add_count("unfinished",
                  _report.unfinished)
       .add_count("steps",
                  static_cast<std::size_t>(std::max(0L,
                                                    _report.steps)))
       .add("unmemoized_steps",
            (_report.without >= 0) ? std::to_string(_report.without)
                                   : std::string("null"))
       .add_count("revisits",
                  total.revisits)
       .add("peaks",
            peaks.line())
       .add("memo",
            memo.line())
       .add("ms",
            ms_of(_report.ms))
       .add_count("times",
                  static_cast<std::size_t>(_report.times))
       .add("rules",
            rules.text(2u))
       .add("runs",
            runs.text(2u));

    return out.text(0u) + "\n";
}

/**
 * @brief Ends a measure that ran nothing, a grammar refused: the refusal
 *        said as an error, and, for JSON, printed as a document too.
 *
 * @param[in] _output   the subcommand's output options.
 * @param[in] _context  the run of the command line.
 * @param[in] _print    whether to print the document: false where -q leaves
 *                      out what the status says.
 * @param[in] _name     the grammar's name.
 * @param[in] _run      the run that found it refused.
 * @return exit_status::REFUSED.
 */
int refused(
    const output_options& _output,
    const context&        _context,
    bool                  _print,
    const std::string&    _name,
    const wb::run_record& _run
)
{
    const int said = refuse(_context,
                            _name,
                            _run);

    // text has nothing more to say
    if ( (!wants_json(_output)) ||
         (!_print) )
    {
        return said;
    }

    json_object out;

    out.add_string("grammar",
                   _name)
       .add_string("frontend",
                   _run.request.frontend)
       .add_bool("accepted",
                 false)
       .add_string("error",
                   wb::headline(_run));

    return emit(_context,
                out.text(0u) + "\n",
                said);
}

/**
 * @brief Says what one program said of an input, for compare's sentence.
 *
 * @param[in] _name     the program's grammar.
 * @param[in] _verdict  what it said.
 * @param[in] _length   whether to say how far a match read.
 * @return "NAME matched", with the length where asked, "NAME did not
 *         match" or "NAME ran out of steps".
 */
std::string said_by(
    const std::string&         _name,
    const vp::corpus::verdict& _verdict,
    bool                       _length
)
{
    std::string out = _name + " " + said(_verdict);

    // two matches that read different lengths say how far
    if ( (_verdict.ok) &&
         (_length) )
    {
        out += " " + std::to_string(_verdict.consumed) + " characters";
    }

    return out;
}

/**
 * @brief Says what compare found, as text.
 *
 * @param[in] _report  what compare found.
 * @return whether the programs agreed, each input they did not agree on, a
 *         line each, and each program's steps.
 */
std::string compare_text(
    const compare_report& _report
)
{
    const vp::corpus::comparison& found = _report.found;
    std::string                   out;

    // agreed: one sentence
    if (found.differences.empty())
    {
        out = _report.first + " and " + _report.second + " agree over " +
              counted_inputs(_report.items) + ": " +
              std::to_string(found.matched) + " matched by both.\n";
    }
    else
    {
        out = _report.first + " and " + _report.second + " differ on " +
              std::to_string(found.differences.size()) + " of " +
              counted_inputs(_report.items) + ":\n";

        // a line an input, as each program said it
        for (const vp::corpus::difference& each : found.differences)
        {
            const bool length = ( (each.first.ok) &&
                                  (each.second.ok) );

            out += "  " + _report.items[each.index].name + ": " +
                   said_by(_report.first,
                           each.first,
                           length) + "; " +
                   said_by(_report.second,
                           each.second,
                           length) + ".\n";
        }
    }

    out += _report.first + " took " + std::to_string(found.first_steps) +
           " steps; " + _report.second + " " +
           std::to_string(found.second_steps);

    // the second program's memo, where it has one
    if (!_report.memoized.empty())
    {
        out += ", its memo for " + joined(_report.memoized) +
               ((_report.memo == "auto") ? " (auto)" : "");
    }

    return out + ".\n";
}

/**
 * @brief Writes one program's verdict on an input as a JSON object.
 *
 * @param[in] _verdict  the verdict.
 * @return the object, on one line.
 */
std::string verdict_json(
    const vp::corpus::verdict& _verdict
)
{
    json_object out;

    out.add_bool("matched",
                 _verdict.ok)
       .add_count("consumed",
                  _verdict.consumed)
       .add_count("steps",
                  static_cast<std::size_t>(std::max(0L,
                                                    _verdict.steps)))
       .add_bool("finished",
                 _verdict.finished);

    return out.line();
}

/**
 * @brief Says what compare found, as JSON.
 *
 * @param[in] _report  what compare found.
 * @return the JSON document, and a newline.
 */
std::string compare_json(
    const compare_report& _report
)
{
    const vp::corpus::comparison& found = _report.found;
    json_array                    differences;

    // a record an input they did not agree on
    for (const vp::corpus::difference& each : found.differences)
    {
        json_object one;

        one.add_string("input",
                       _report.items[each.index].argument)
           .add_string("kind",
                       _report.items[each.index].kind)
           .add("first",
                verdict_json(each.first))
           .add("second",
                verdict_json(each.second));
        differences.add(one.line());
    }

    json_object out;

    out.add_string("first",
                   _report.first)
       .add_string("second",
                   _report.second)
       .add_string("first_frontend",
                   _report.first_frontend)
       .add_string("second_frontend",
                   _report.second_frontend)
       .add_bool("accepted",
                 true)
       .add_string("error",
                   std::string())
       .add_string("memoize",
                   _report.memo)
       .add("memoized",
            json_strings(_report.memoized))
       .add_count("inputs",
                  found.inputs)
       .add_count("matched",
                  found.matched)
       .add_bool("agreed",
                 found.differences.empty())
       .add("differences",
            differences.text(2u))
       .add_count("first_steps",
                  static_cast<std::size_t>(std::max(0L,
                                                    found.first_steps)))
       .add_count("second_steps",
                  static_cast<std::size_t>(std::max(0L,
                                                    found.second_steps)));

    return out.text(0u) + "\n";
}

/**
 * @brief Lays out a corpus, an input a row.
 *
 * @param[in] _samples  the corpus.
 * @return the listing: each input's number, kind and text.
 */
wb::listing corpus_listing(
    const std::vector<vp::corpus::sample>& _samples
)
{
    wb::listing out;

    out.columns = { "#", "kind", "text" };
    out.note    = "No walk of the grammar ends: its start rule has no way "
                  "out, every alternative reaching it again before any text.";

    // a row an input, in order
    for (std::size_t at = 0u; at < _samples.size(); at++)
    {
        out.rows.push_back({ std::to_string(at + 1u),
                             _samples[at].derived ? "walk" : "miss",
                             wb::escape(_samples[at].text) });
        out.marks.push_back(_samples[at].derived ? 0 : 1);
    }

    return out;
}

/**
 * @brief Names the file a corpus input is written to.
 *
 * @param[in] _index   the input's place in the corpus, from 0.
 * @param[in] _count   how many inputs there are.
 * @param[in] _sample  the input.
 * @return its number, zero-padded to the widest number, then its kind:
 *         `0001-walk.txt`.
 */
std::string file_name(
    std::size_t               _index,
    std::size_t               _count,
    const vp::corpus::sample& _sample
)
{
    const std::size_t width  = std::max<std::size_t>(4u,
                                                     std::to_string(_count)
                                                         .size());
    std::string       number = std::to_string(_index + 1u);

    number.insert(0u,
                  width - std::min(width,
                                   number.size()),
                  '0');

    return number + (_sample.derived ? "-walk.txt" : "-miss.txt");
}

/**
 * @brief Writes a corpus, an input a file, to a folder, each file whole or
 *        not at all, as the kit writes one (GRAMMAR.md, R20).
 *
 * @param[in]  _folder   the folder, in UTF-8; made if it is not there.
 * @param[in]  _samples  the corpus.
 * @param[in]  _context  the run of the command line, which says a failure.
 * @param[out] _files    receives each file's path, in UTF-8, in order.
 * @return exit_status::PASSED when every file was written; REFUSED, said,
 *         when the folder could not be made, or a file not written -- the
 *         files before it written, and the one that failed as it was.
 */
int write_corpus(
    const std::string&                     _folder,
    const std::vector<vp::corpus::sample>& _samples,
    const context&                         _context,
    std::vector<std::string>&              _files
)
{
    const fs::path  folder = wb::path_from_utf8(_folder);
    std::error_code code;

    fs::create_directories(folder,
                           code);

    // a folder that is not there, and cannot be made
    if ( (code) &&
         (!fs::is_directory(folder)) )
    {
        return fail(_context,
                    "cannot make '" + _folder + "': " + code.message());
    }

    // a file an input
    for (std::size_t at = 0u; at < _samples.size(); at++)
    {
        const std::string file   =
            wb::utf8_from_path(folder / file_name(at,
                                                  _samples.size(),
                                                  _samples[at]));
        const int         failed = d_cli_write_file(file.c_str(),
                                                    _samples[at].text.data(),
                                                    _samples[at].text.size());

        // a file that cannot be written ends the writing
        if (failed != 0)
        {
            return d_cli_file_error(_context.program,
                                    "write",
                                    file.c_str(),
                                    failed);
        }

        _files.push_back(file);
    }

    return exit_status::PASSED;
}

/**
 * @brief Says what corpus made, as text.
 *
 * @param[in] _grammar  the grammar's name.
 * @param[in] _samples  the corpus.
 * @param[in] _folder   the folder it was written to; empty when it was not.
 * @return the listing, or the sentence saying where the files went.
 */
std::string corpus_text(
    const std::string&                     _grammar,
    const std::vector<vp::corpus::sample>& _samples,
    const std::string&                     _folder
)
{
    // listed, where it was not written
    if (_folder.empty())
    {
        return wb::listing_text(corpus_listing(_samples));
    }

    std::size_t walks = 0u;

    // the walks among them
    for (const vp::corpus::sample& each : _samples)
    {
        walks += each.derived ? 1u : 0u;
    }

    return "Wrote " + std::to_string(_samples.size()) + " inputs from " +
           _grammar + " to " + _folder + ": " + std::to_string(walks) +
           " walks and " + std::to_string(_samples.size() - walks) +
           " near-misses.\n";
}

/**
 * @brief Says what corpus made, as JSON.
 *
 * @param[in] _grammar   the grammar's name.
 * @param[in] _settings  how the corpus was made.
 * @param[in] _samples   the corpus.
 * @param[in] _files     each input's file; none when it was not written.
 * @return the JSON document, and a newline.
 */
std::string corpus_json(
    const std::string&                     _grammar,
    const vp::corpus::settings&            _settings,
    const std::vector<vp::corpus::sample>& _samples,
    const std::vector<std::string>&        _files
)
{
    json_array inputs;

    // a record an input
    for (std::size_t at = 0u; at < _samples.size(); at++)
    {
        json_object one;

        one.add_string("text",
                       _samples[at].text)
           .add_string("kind",
                       _samples[at].derived ? "walk" : "miss")
           .add_string("file",
                       (at < _files.size()) ? _files[at]
                                            : std::string());
        inputs.add(one.line());
    }

    json_object out;

    out.add_string("grammar",
                   _grammar)
       .add_bool("accepted",
                 true)
       .add_string("error",
                   std::string())
       .add_count("walks",
                  _settings.walks)
       .add_count("depth",
                  _settings.depth)
       .add_count("seed",
                  static_cast<std::size_t>(_settings.seed))
       .add("inputs",
            inputs.text(2u))
       .add_count("length",
                  _settings.length)
       .add_count("repeats",
                  _settings.repeats)
       .add_count("misses",
                  _settings.misses);

    return out.text(0u) + "\n";
}

//------------------------------------------------------------------------------
// reading compare's grammars
//------------------------------------------------------------------------------

/**
 * @brief Works out which frontend reads compare's second grammar.
 *
 * @param[in] _command  the subcommand.
 * @return --ebnf's or --frontend's, as for the first; and, when neither was
 *         given, the one the second grammar's own name calls for.
 */
std::string second_frontend(
    const compare_command& _command
)
{
    // said outright, for both
    if ( (given(_command.source.ebnf)) ||
         (given(_command.source.frontend)) )
    {
        return frontend_of(_command.source);
    }

    return wb::frontend_for_file(value_of(_command.second));
}

/**
 * @brief Reads what compare is given: both grammars, then the inputs, the
 *        standard input at most once among them.
 *
 * @param[in]  _command  the subcommand.
 * @param[in]  _context  the standard input `-` reads, and where a failure
 *                       is written.
 * @param[out] _read     receives the first grammar and the inputs.
 * @param[out] _second   receives the second grammar.
 * @return exit_status::PASSED when everything was read; otherwise REFUSED,
 *         the failure said.
 */
int read_compared(
    const compare_command& _command,
    const context&         _context,
    sources&               _read,
    source&                _second
)
{
    const bool                     as_text  = given(_command.text);
    const std::vector<std::string> names    = values_of(_command.inputs);
    const std::size_t              standard =
        standard_inputs(names,
                        as_text) +
        ((value_of(_command.source.grammar) == STANDARD_INPUT) ? 1u : 0u) +
        ((value_of(_command.second) == STANDARD_INPUT) ? 1u : 0u);

    // '-' named for more than one of them
    if (standard > 1u)
    {
        return usage_error(_command.command,
                           "standard input can be read once: - may stand "
                           "for one grammar or for one input",
                           _context);
    }

    std::string unread = read_grammar(_command.source,
                                      _context,
                                      _read.grammar);
    std::string error;

    // the second grammar, once the first is read
    if ( (unread.empty()) &&
         (!read_source(value_of(_command.second),
                       false,
                       _context,
                       _second,
                       error)) )
    {
        unread = error;
    }

    // the inputs, once both grammars are read
    if (unread.empty())
    {
        unread = read_inputs(names,
                             as_text,
                             _context,
                             _read.inputs);
    }

    // a file that cannot be read
    if (!unread.empty())
    {
        return fail(_context,
                    unread);
    }

    return exit_status::PASSED;
}

}  // anonymous namespace

namespace subcommand
{

/*
subcommand::bench
  The grammar is built once, by a run over no input; then every input, given
or made, is run by the library on the program the memo setting makes of it,
counted once and timed --times over. Nothing is traced: a corpus of
thousands would be a trace of millions of lines, and the profile counts
what the heat map would, without one.
*/
int bench(
    const bench_command& _command,
    const context&       _context
)
{
    sources   read;
    const int status = read_all(_command,
                                _context,
                                read);

    // a file named wrongly, or '-' named twice
    if (status != exit_status::PASSED)
    {
        return status;
    }

    const std::string    frontend = frontend_of(_command.source);
    const long           limit    = step_limit_of(_command.run);
    const wb::run_record built    = wb::execute(1,
                                                request_for(frontend,
                                                            read.grammar,
                                                            source(),
                                                            false,
                                                            limit));

    // refused: the run's own sentence says why
    if (!accepted(built))
    {
        return refused(_command.output,
                       _context,
                       true,
                       read.grammar.name,
                       built);
    }

    bench_report report;
    std::string  unknown;

    report.grammar  = read.grammar.name;
    report.frontend = frontend;
    report.memo     = memo_of(_command.run.memoize);
    report.times    = std::max(1L,
                               number_of(_command.times,
                                         TIMES_FALLBACK));
    report.items    = items_of(read.inputs,
                               runs_corpus(read.inputs,
                                           _command.corpus.walks)
                                   ? corpus_of(built,
                                               settings_of(_command.corpus))
                                   : std::vector<vp::corpus::sample>());

    const std::vector<int> starts = memo_for(built,
                                             report.items,
                                             report.memo,
                                             limit,
                                             unknown,
                                             report.without);

    // a name no rule has: the request is refused, as a run refuses it
    if (!unknown.empty())
    {
        return fail(_context,
                    read.grammar.name + ": no rule is named " + unknown +
                        ", to be called through the memo");
    }

    const vp::peg::program program = vp::peg::memoized(built.program,
                                                       starts);

    report.memoized = names_of(built,
                               starts);

    // every input, counted and timed
    for (const item& each : report.items)
    {
        const measured run = measure(program,
                                     each.bytes,
                                     limit,
                                     report.times);

        vp::peg::accumulate(report.total,
                            run.counts);
        report.steps      += run.verdict.steps;
        report.ms         += run.ms;
        report.matched    += run.verdict.ok ? 1u : 0u;
        report.unfinished += run.verdict.finished ? 0u : 1u;
        report.runs.push_back(run);
    }

    return emit(_context,
                wants_json(_command.output) ? bench_json(report,
                                                         built)
                                            : bench_text(report,
                                                         built),
                exit_status::PASSED);
}

/*
subcommand::corpus
  The grammar is built as every subcommand builds one, so a grammar refused
anywhere is refused here; the corpus is then made from the rules as the
frontend read them.
*/
int corpus(
    const corpus_command& _command,
    const context&        _context
)
{
    source            grammar;
    const std::string unread = read_grammar(_command.source,
                                            _context,
                                            grammar);

    // a grammar that cannot be read
    if (!unread.empty())
    {
        return fail(_context,
                    unread);
    }

    const wb::run_record built =
        wb::execute(1,
                    request_for(frontend_of(_command.source),
                                grammar,
                                source(),
                                false,
                                wb::run_request().step_limit));

    // refused: the run's own sentence says why
    if (!accepted(built))
    {
        return refused(_command.output,
                       _context,
                       true,
                       grammar.name,
                       built);
    }

    const vp::corpus::settings            settings =
        settings_of(_command.corpus);
    const std::vector<vp::corpus::sample> samples  = corpus_of(built,
                                                               settings);
    const std::string                     folder   =
        value_of(_command.folder);
    std::vector<std::string>              files;
    const int                             written  =
        folder.empty() ? static_cast<int>(exit_status::PASSED)
                       : write_corpus(folder,
                                      samples,
                                      _context,
                                      files);

    // written before anything is printed, so a failure prints only itself
    if (written != exit_status::PASSED)
    {
        return written;
    }

    return emit(_context,
                wants_json(_command.output) ? corpus_json(grammar.name,
                                                          settings,
                                                          samples,
                                                          files)
                                            : corpus_text(grammar.name,
                                                          samples,
                                                          folder),
                exit_status::PASSED);
}

/*
subcommand::compare
  Both grammars are built as every subcommand builds one, the corpus is the
first's, and corpus::compare() runs the two programs over every input: the
first as built, the second as the memo setting makes it.
*/
int compare(
    const compare_command& _command,
    const context&         _context
)
{
    sources   read;
    source    second;
    const int status = read_compared(_command,
                                     _context,
                                     read,
                                     second);

    // a file named wrongly, or '-' named twice
    if (status != exit_status::PASSED)
    {
        return status;
    }

    compare_report report;

    report.first           = read.grammar.name;
    report.second          = second.name;
    report.first_frontend  = frontend_of(_command.source);
    report.second_frontend = second_frontend(_command);
    report.memo            = memo_of(_command.run.memoize);

    const long           limit = step_limit_of(_command.run);
    const wb::run_record one   = wb::execute(1,
                                             request_for(report.first_frontend,
                                                         read.grammar,
                                                         source(),
                                                         false,
                                                         limit));
    const wb::run_record other =
        wb::execute(1,
                    request_for(report.second_frontend,
                                second,
                                source(),
                                false,
                                limit));

    // either refused: its own sentence says why
    if ( (!accepted(one)) ||
         (!accepted(other)) )
    {
        const bool first_refused = !accepted(one);

        return refused(_command.output,
                       _context,
                       !quiet(_context),
                       first_refused ? report.first
                                     : report.second,
                       first_refused ? one
                                     : other);
    }

    std::string unknown;
    long        without = -1;

    report.items = items_of(read.inputs,
                            runs_corpus(read.inputs,
                                        _command.corpus.walks)
                                ? corpus_of(one,
                                            settings_of(_command.corpus))
                                : std::vector<vp::corpus::sample>());

    const std::vector<int> starts = memo_for(other,
                                             report.items,
                                             report.memo,
                                             limit,
                                             unknown,
                                             without);

    // a name the second grammar has no rule for
    if (!unknown.empty())
    {
        return fail(_context,
                    report.second + ": no rule is named " + unknown +
                        ", to be called through the memo");
    }

    std::vector<std::string> texts;

    // the inputs' bytes, in order
    for (const item& each : report.items)
    {
        texts.push_back(each.bytes);
    }

    report.memoized = names_of(other,
                               starts);
    report.found    = vp::corpus::compare(one.program,
                                          vp::peg::memoized(other.program,
                                                            starts),
                                          texts,
                                          limit);

    const int verdict = report.found.differences.empty()
                            ? exit_status::PASSED
                            : exit_status::NO_MATCH;

    // the status says it all
    if (quiet(_context))
    {
        return verdict;
    }

    return emit(_context,
                wants_json(_command.output) ? compare_json(report)
                                            : compare_text(report),
                verdict);
}

}  // namespace subcommand

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now
