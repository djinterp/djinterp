/*******************************************************************************
* vparse [tests]                                        workbench_core_tests.cpp
*
* The model's suite: what a run holds, what the session decides for it, the
* heat it leaves, what it counts and what its memo does, and the files it is
* saved to.
*   The model is what both of vparse's programs stand on -- the command line
* and the window -- and it needs nothing but djinterp: no uxoxo, no Dear
* ImGui, no display. So its suite is built wherever vparse's command line
* is, the window or not; what the window adds to it -- its model of events,
* its face -- is workbench_tests.cpp's.
*   Where a number could come from the code under test, it does not: the
* program a run holds is compared with the session's disassembly, and the
* heat figures are the ones the first map of this grammar measured.
*   These sections were the workbench suite's, moved here unchanged when the
* window and the command line became two programs, but for the reading
* section, which came with the reader's own step limit, and the memo's and
* the profile's, which came with them.
*   LEVELS. The model's floor is C++17, so this file's is: below it the file
* is empty rather than an error.
*
*
* path:      /tests/djinterp/tools/vparse/workbench_core_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.27
*******************************************************************************/
#include "./tool_tests.hpp"  // the suite's harness and grammars
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor its header has

// std
#include <cstddef>       // std::size_t
#include <filesystem>    // std::filesystem
#include <string>        // std::string
#include <system_error>  // std::error_code
#include <utility>       // std::pair
#include <vector>        // std::vector
// djinterp
#include "djinterp/cpp/modules/parsegen/vparse/peg.hpp"      // peg::CALL, peg::MEMO
#include "djinterp/cpp/modules/parsegen/vparse/session.hpp"  // session, for the programs
                                                 // it builds
// vparse
#include "../../../../inc/djinterp/tools/vparse/heat.hpp"   // build_heat,
                                                            // heat_summary,
                                                            // heat_svg
#include "../../../../inc/djinterp/tools/vparse/run.hpp"    // execute,
                                                            // headline,
                                                            // part_listing
#include "../../../../inc/djinterp/tools/vparse/store.hpp"  // save_run,
                                                            // list_saved,
                                                            // load_request


namespace
{

namespace fs = std::filesystem;
namespace vp = ::djinterp::parsegen::vparse;
namespace wb = ::vparse::workbench;

using ::vparse::tests::ARITHMETIC;
using ::vparse::tests::NESTED;
using ::vparse::tests::NESTED_INPUT;
using ::vparse::tests::SUMS;
using ::vparse::tests::chain;
using ::vparse::tests::disassembly;
using ::vparse::tests::has;
using ::vparse::tests::request;
using ::vparse::tests::section;

/*
tests_workbench_refusals
  The session judges every grammar the workbench reads, so the workbench
refuses what the library refuses, in its words, in either notation.
  Tests the following:
  - a reference to an undefined rule, a rule defined twice, and text with no
    rule are refused in both notations, with the session's message
  - a refused grammar is read but not built, and nothing runs
  - a grammar the reader cannot read is reported as unread, with where the
    reader stopped, and is not judged further
  - a refusal puts the place where the reader stopped apart from its
    sentence -- the line and column of the reading's furthest offset, and
    the headline without them -- and has no place, and the headline whole,
    where the grammar was read and refused, or the EBNF reader refused it
*/
bool tests_workbench_refusals()
{
    struct refusal
    {
        const char* frontend;
        const char* grammar;
    };

    const refusal refusals[] = {
        { "notation", "S = A ;"                       },
        { "notation", "S = 'a' ;\nS = 'b' ;"          },
        { "notation", ""                              },
        { "ebnf",     "S = A ;"                       },
        { "ebnf",     "S = \"a\" ;\nS = \"b\" ;"      },
        { "ebnf",     "# no rule\n"                   } };

    for (const refusal& each : refusals)
    {
        const wb::run_record run =
            wb::execute(1, request(each.frontend, each.grammar, "a"));
        vp::session              judge;
        const vp::compile_result verdict =
            judge.load(each.grammar,
                       (std::string(each.frontend) == "ebnf")
                           ? vp::frontend::ebnf
                           : vp::frontend::metacircular);

        const wb::refusal said = wb::refusal_of(run);

        if ( (verdict.ok)                                   ||
             (!run.read_ok)                                 ||
             (run.built)                                    ||
             (run.build_error != verdict.error)             ||
             (run.recognition.ran)                          ||
             (!has(wb::headline(run), verdict.error))       ||
             (said.line != 0)                               ||
             (said.sentence != wb::headline(run)) )
        {
            return false;
        }
    }

    // the same three faults, the same three words, in either notation
    const std::string undefined =
        wb::execute(1, request("notation", "S = A ;", "")).build_error;

    if ( (undefined != "rule not defined: A") ||
         (wb::execute(1, request("ebnf", "S = A ;", "")).build_error !=
          undefined) )
    {
        return false;
    }

    const wb::run_record      unread =
        wb::execute(1, request("notation", "S = 'a' ;\nT = [0-9]+", "1"));
    const wb::run_record      misread =
        wb::execute(1, request("ebnf", "S = ", "1"));
    const wb::refusal         placed  = wb::refusal_of(unread);
    const wb::refusal         plain   = wb::refusal_of(misread);
    const int                 far     = unread.reading.far.offset;
    const std::pair<int, int> at      = wb::line_col(unread.request.grammar,
                                                     far);
    std::string               without = wb::headline(unread);
    const std::string         place   = " at line " +
                                        std::to_string(at.first) +
                                        ", column " +
                                        std::to_string(at.second) +
                                        " (character " +
                                        std::to_string(far) + ")";
    const std::size_t         found   = without.find(place);

    // the headline as it would read without its place
    if (found != std::string::npos)
    {
        without.erase(found,
                      place.size());
    }

    return ( (!unread.read_ok)                               &&
             (unread.build_error.empty())                    &&
             (has(unread.read_error, "the reader stopped"))  &&
             (!unread.built)                                 &&
             (found != std::string::npos)                    &&
             (at.first == 2)                                 &&
             (placed.line == at.first)                       &&
             (placed.column == at.second)                    &&
             (placed.sentence == without)                    &&
             (!misread.read_ok)                              &&
             (plain.line == 0)                               &&
             (plain.column == 0)                             &&
             (plain.sentence == wb::headline(misread)) );
}

/*
tests_workbench_parity
  The program the workbench shows is the session's program, instruction for
instruction, in either notation.
  Tests the following:
  - for grammars in each notation, the run's program lists exactly as the
    session's disassembly of the same text
  - a notation run keeps the generator's plan, which an EBNF run has none of
  - the program's first instruction calls the start rule's entry
*/
bool tests_workbench_parity()
{
    const wb::run_request runs[] = {
        request("notation", SUMS, "12+3"),
        request("notation", NESTED, NESTED_INPUT),
        request("notation", "S = [0-9]+ ;", "123"),
        request("ebnf", ARITHMETIC, "2*(3+4)"),
        request("ebnf", "S = \"a\" S? ;", "aaa") };

    for (const wb::run_request& each : runs)
    {
        const wb::run_record run = wb::execute(1, each);
        vp::session          judge;

        const bool loaded =
            judge.load(each.grammar,
                       (each.frontend == "ebnf")
                           ? vp::frontend::ebnf
                           : vp::frontend::metacircular).ok;

        if ( (!loaded)                                              ||
             (!run.built)                                           ||
             (disassembly(run.program) != judge.disassembly())      ||
             (!run.recognition.matched) )
        {
            return false;
        }

        if ( (each.frontend == "ebnf")
                 ? (!run.plan.empty())
                 : (run.plan.empty()) )
        {
            return false;
        }

        if ( (run.program.empty()) ||
             (run.program[0].op != vp::peg::CALL) )
        {
            return false;
        }
    }

    return true;
}

/*
tests_workbench_failure
  A run that fails says where, and a run without a trace still says whether.
  Tests the following:
  - an input that stops short reports its furthest offset, the rules open
    there, and what was expected
  - the matches made before the failure are kept
  - left recursion stops at the step limit, and the headline says why
  - without the trace a run still has its result, but no frontier and no
    heat
*/
bool tests_workbench_failure()
{
    const wb::run_record failed =
        wb::execute(1, request("notation", SUMS, "12+3-*45"));
    const wb::parse_pass& pass = failed.recognition;

    if ( (pass.matched)                              ||
         (pass.far.offset != 5)                      ||
         (pass.far.stack.empty())                    ||
         (pass.far.stack.back() != "Term")           ||
         (pass.far.expected.empty())                 ||
         (pass.matches.empty())                      ||
         (!has(wb::headline(failed), "column 6")) )
    {
        return false;
    }

    wb::run_request looping = request("notation", "S = S 'a' | 'a' ;", "aa");
    looping.step_limit = 500;

    const wb::run_record loops = wb::execute(1, looping);

    if ( (loops.recognition.matched)                       ||
         (loops.recognition.error != "step limit")         ||
         (!has(wb::headline(loops), "step limit of 500")) )
    {
        return false;
    }

    wb::run_request blind = request("notation", SUMS, "12+3-*45");
    blind.trace = false;

    const wb::run_record unseen = wb::execute(1, blind);

    return ( (unseen.recognition.ran)                &&
             (!unseen.recognition.matched)           &&
             (unseen.recognition.trace.empty())      &&
             (unseen.recognition.far.offset < 0)     &&
             (!unseen.heat)                          &&
             (!wb::part_listing(unseen, "heat").note.empty()) );
}

/*
tests_workbench_reading
  A grammar is read under the reader's own step limit, as the session reads
it, and a request's limit is the run over its input's.
  Tests the following:
  - a grammar whose reading takes more steps than a request's default limit
    is read and built under a limit of 1,000, and matches
  - its program is the session's, instruction for instruction
  - an input that takes more steps than the limit stops at it, its grammar
    read all the same
  - the reading's captures are placed at the line and column they start at
*/
bool tests_workbench_reading()
{
    wb::run_request outrun = request("notation",
                                     chain(500u),
                                     "abc0");

    outrun.trace      = false;
    outrun.step_limit = 1000;

    const std::string letters(499u,
                              'a');
    wb::run_request   deep = outrun;

    deep.input = letters + "0";

    const wb::run_record read    = wb::execute(1,
                                               outrun);
    const wb::run_record stopped = wb::execute(2,
                                               deep);
    vp::session          judge;

    const bool loaded = judge.load(outrun.grammar,
                                   vp::frontend::metacircular).ok;

    // SUMS's second rule is named at the start of line 2, and its first
    // rule refers to Term at line 1, column 8
    const wb::listing captures =
        wb::part_listing(wb::execute(3,
                                     request("notation",
                                             SUMS,
                                             "1")),
                         "reading");
    bool              more     = false;
    bool              term     = false;

    // each capture, as the listing places it
    for (const std::vector<std::string>& row : captures.rows)
    {
        // a row is the capture's number, its tag, where it starts, its text
        if (row.size() != 4u)
        {
            continue;
        }

        more = ( (more) ||
                 ( (row[1] == "NAME") &&
                   (row[2] == "2:1")  &&
                   (row[3] == "More") ) );
        term = ( (term) ||
                 ( (row[1] == "REF")  &&
                   (row[2] == "1:8")  &&
                   (row[3] == "Term") ) );
    }

    return ( (loaded)                                                 &&
             (read.read_ok)                                           &&
             (read.reading.steps > wb::run_request().step_limit)      &&
             (read.built)                                             &&
             (read.recognition.matched)                               &&
             (disassembly(read.program) == judge.disassembly())       &&
             (stopped.read_ok)                                        &&
             (!stopped.recognition.matched)                           &&
             (stopped.recognition.error == "step limit")              &&
             (more)                                                   &&
             (term) );
}

/*
tests_workbench_heat
  The heat of a re-parsing grammar is the heat first measured for it, and a
long input is mapped in shared columns.
  Tests the following:
  - NESTED over its input touches 56 cells, with 7350 arrivals beyond the
    first, and 540 at the hottest: Atom, at the closing parenthesis
  - the hot-spots listing leads with that cell, and the summary names it
  - an input of 1000 characters is mapped in 143 columns of 7
  - the saved picture is an SVG with a cell for each touched place, and a
    failed run's has its frontier marked
*/
bool tests_workbench_heat()
{
    const wb::run_record run =
        wb::execute(1, request("notation", NESTED, NESTED_INPUT));

    if (!run.heat)
    {
        return false;
    }

    const wb::heat_space& heat = *run.heat;

    if ( (heat.touched != 56u)                              ||
         (heat.revisits != 7350u)                           ||
         (heat.max_entries != 540u)                         ||
         (heat.hot_row < 0)                                 ||
         (heat.rows[static_cast<std::size_t>(heat.hot_row)] != "Atom") ||
         (heat.columns[static_cast<std::size_t>(heat.hot_col)] != ")") )
    {
        return false;
    }

    const wb::listing spots = wb::part_listing(run, "heat");

    if ( (spots.rows.empty())                       ||
         (spots.rows[0][0] != "Atom")               ||
         (spots.rows[0][3] != "540")                ||
         (!has(wb::heat_summary(heat), "540")) )
    {
        return false;
    }

    const wb::run_record wide = wb::execute(
        1, request("notation", "S = [a-z]* ;", std::string(1000u, 'q')));

    if ( (!wide.heat)                       ||
         (wide.heat->bucket != 7u)          ||
         (wide.heat->columns.size() != 143u) )
    {
        return false;
    }

    const std::string picture = wb::heat_svg(heat, run);

    const wb::run_record failed =
        wb::execute(1, request("notation", NESTED, "((1+2)*(3-4)/5"));
    const std::string frontier = wb::heat_svg(*failed.heat, failed);

    // the frontier is outlined in the ribbon's red, and only a failed run
    // has one
    return ( (picture.rfind("<svg", 0) == 0u)           &&
             (has(picture, "</svg>"))                    &&
             (!has(picture, "stroke=\"#b42318\""))      &&
             (has(frontier, "stroke=\"#b42318\"")) );
}

/*
tests_workbench_store
  A saved run is its request and its parts on disk, and loading it runs it
again to the same parts.
  Tests the following:
  - save_run writes the run's folder, with every part, the request, and the
    heat picture
  - list_saved finds the run, with its front end and result
  - load_request reads the request back byte for byte
  - the request run again gives every part identically, but for the three
    summary rows that belong to a run rather than to its request -- when it
    ran, how long it took, and where it came from, which for the replay is
    the folder
*/
bool tests_workbench_store()
{
    const fs::path  root = fs::path("workbench_tests_runs");
    std::error_code ignored;

    fs::remove_all(root, ignored);

    const wb::run_record first =
        wb::execute(1, request("notation", NESTED, "((1+2)*(3-4)/5"));
    const fs::path session = root / wb::session_name();
    std::string    error;
    const fs::path folder  = wb::save_run(first, session, &error);

    bool ok = ( (!folder.empty())                             &&
                (error.empty())                               &&
                (fs::exists(folder / "heat.svg"))             &&
                (fs::exists(folder / "grammar.txt"))          &&
                (fs::exists(folder / "trace.txt")) );

    const std::vector<wb::saved_run> saved = wb::list_saved(root);

    ok = ( (ok)                                  &&
           (saved.size() == 1u)                  &&
           (saved[0].id == 1)                    &&
           (saved[0].frontend == "notation") );

    wb::run_request again;

    ok = ( (ok)                                            &&
           (wb::load_request(folder, again, &error))       &&
           (again.grammar == first.request.grammar)        &&
           (again.input == first.request.input)            &&
           (again.frontend == first.request.frontend) );

    if (ok)
    {
        const wb::run_record second = wb::execute(1, again);

        for (const std::string& part : wb::part_names())
        {
            wb::listing before = wb::part_listing(first, part);
            wb::listing after  = wb::part_listing(second, part);

            // a run's own rows: its time, its timings, its origin
            if (part == "summary")
            {
                for (wb::listing* each : { &before, &after })
                {
                    for (std::vector<std::string>& row : each->rows)
                    {
                        if ( (row.size() == 2u)      &&
                             ( (row[0] == "when")   ||
                               (row[0] == "time")   ||
                               (row[0] == "origin") ) )
                        {
                            row[1].clear();
                        }
                    }
                }
            }

            if (wb::listing_text(before) != wb::listing_text(after))
            {
                ok = false;
            }
        }

        ok = ( (ok) &&
               (has(second.request.origin, folder.string())) );
    }

    fs::remove_all(root, ignored);

    return ok;
}

/**
 * @brief Counts a program's instructions of one opcode.
 *
 * @param[in] _program  the program.
 * @param[in] _op       the opcode.
 * @return how many of them there are.
 */
std::size_t count_of(
    const vp::peg::program& _program,
    int                     _op
)
{
    std::size_t out = 0u;

    // each instruction
    for (const vp::peg::DInstr& each : _program)
    {
        out += (each.op == _op) ? 1u : 0u;
    }

    return out;
}

/**
 * @brief Runs NESTED over an input, its memo set.
 *
 * @param[in] _input    the input.
 * @param[in] _setting  the memo setting.
 * @return the run.
 */
wb::run_record nested_run(
    const std::string& _input,
    const std::string& _setting
)
{
    wb::run_request asked = request("notation",
                                    NESTED,
                                    _input);

    asked.memoize = _setting;

    return wb::execute(1,
                       asked);
}

/*
tests_workbench_memo
  A run's program calls the rules its request names through the memo, and
says what it did, in every part as the run without it says it.
  Tests the following:
  - memo_rules reads none, all, names in any order with spaces, an empty
    name and a name twice, gives the rules in program order, and names a
    name no rule has
  - a run with Sum and Prod memoized calls them by MEMO, as many as the
    calls it had, matches as the run without the memo does, in fewer steps,
    with the same matches and, for an input that fails, the same frontier
  - "auto" chooses Sum, Prod and Atom, and counts the steps of the run
    without the memo; a name no rule has is refused, the program not built
  - the setting is saved with the run, and read back with it
*/
bool tests_workbench_memo()
{
    const wb::run_record plain = nested_run(NESTED_INPUT,
                                            "none");
    std::string          unknown;

    const std::vector<int> all   = wb::memo_rules(plain,
                                                  "all",
                                                  &unknown);
    const std::vector<int> two   = wb::memo_rules(plain,
                                                  " Prod , Sum,,Prod",
                                                  &unknown);
    const std::vector<int> none  = wb::memo_rules(plain,
                                                  "none",
                                                  &unknown);
    const std::vector<int> chose = wb::memo_rules(plain,
                                                  "auto",
                                                  &unknown);
    const bool             clean = unknown.empty();
    const std::vector<int> nope  = wb::memo_rules(plain,
                                                  "Sum,Nope",
                                                  &unknown);

    // the rules, in program order, by name
    if ( (all.size() != 5u)                                        ||
         (two.size() != 2u)                                        ||
         (plain.entry[static_cast<std::size_t>(two[0])] != "Sum")  ||
         (plain.entry[static_cast<std::size_t>(two[1])] != "Prod") ||
         (!none.empty())                                           ||
         (!chose.empty())                                          ||
         (!clean)                                                  ||
         (!nope.empty())                                           ||
         (unknown != "Nope") )
    {
        return false;
    }

    const wb::run_record memo   = nested_run(NESTED_INPUT,
                                             "Sum,Prod");
    std::size_t          called = 0u;

    // the plain program's calls to Sum and Prod
    for (const vp::peg::DInstr& each : plain.program)
    {
        called += ( (each.op == vp::peg::CALL) &&
                    ( (each.arg == two[0]) ||
                      (each.arg == two[1]) ) ) ? 1u : 0u;
    }

    const wb::run_record failed      = nested_run("((1+2)*(3-4)/5",
                                                  "none");
    const wb::run_record failed_memo = nested_run("((1+2)*(3-4)/5",
                                                  "all");
    const wb::listing    matches     = wb::part_listing(memo,
                                                        "matches");
    const wb::listing    plain_ones  = wb::part_listing(plain,
                                                        "matches");
    const std::size_t    memos       = count_of(memo.program,
                                                vp::peg::MEMO);

    // the same run, by fewer steps
    if ( (!memo.built)                                                ||
         (memo.memoized != std::vector<std::string>{ "Sum", "Prod" }) ||
         (memos != called)                                            ||
         (called == 0u)                                               ||
         (memo.recognition.matched != plain.recognition.matched)      ||
         (memo.recognition.steps >= plain.recognition.steps)          ||
         (wb::listing_text(matches) != wb::listing_text(plain_ones))  ||
         (failed_memo.recognition.matched)                            ||
         (wb::headline(failed_memo) != wb::headline(failed)) )
    {
        return false;
    }

    const wb::run_record automatic = nested_run(NESTED_INPUT,
                                                "auto");
    const wb::run_record refused   = nested_run(NESTED_INPUT,
                                                "Sum,Nope");

    // chosen by a run without it, and refused for a name
    if ( (automatic.memoized !=
          std::vector<std::string>{ "Sum", "Prod", "Atom" })         ||
         (automatic.unmemoized_steps != plain.recognition.steps)     ||
         (refused.built)                                             ||
         (!has(refused.build_error,
               "Nope"))                                              ||
         (!has(wb::headline(refused),
               "could not be built")) )
    {
        return false;
    }

    const fs::path  root = fs::path("workbench_tests_memo_runs");
    std::error_code ignored;
    std::string     error;
    wb::run_request again;

    fs::remove_all(root,
                   ignored);

    const fs::path folder = wb::save_run(memo,
                                         root / wb::session_name(),
                                         &error);
    const bool     loaded = ( (!folder.empty()) &&
                              (wb::load_request(folder,
                                                again,
                                                &error)) );

    fs::remove_all(root,
                   ignored);

    return ( (loaded) &&
             (again.memoize == "Sum,Prod") );
}

/*
tests_workbench_profile
  What a run counts as it runs is what its heat map says, the memoized run's
too.
  Tests the following:
  - the profile part has a row for each rule, then the gate's and the whole
    run's, and the whole run's revisits are the heat map's: 7350 for NESTED
    over its input
  - each rule's steps, arrivals and revisits are the sums of its row of the
    heat map, and the revisits by position sum to the revisits by rule
  - with every rule memoized, the heat map and the profile still agree, and
    the memo's answers are counted as hits; the heat map's call stack opens
    a frame for each call the memo did not answer
  - the summary says how deep the stacks went, and what the memo did
*/
bool tests_workbench_profile()
{
    // each run, held to its own heat map
    for (const char* setting : { "none", "all" })
    {
        const wb::run_record    run    = nested_run(NESTED_INPUT,
                                                    setting);
        const vp::peg::profile& counts = run.recognition.counts;

        // a heat map to hold it to
        if (!run.heat)
        {
            return false;
        }

        const wb::heat_space& heat  = *run.heat;
        const std::size_t     rules = counts.rules.size();
        std::size_t           place = 0u;

        // the revisits by position
        for (const std::size_t each : counts.revisits_at)
        {
            place += each;
        }

        std::size_t entered = 0u;

        // the calls the rules ran
        for (const vp::peg::profile::DRuleCounts& each : counts.rules)
        {
            entered += each.calls - each.hits;
        }

        // the map's totals, its frames among them
        if ( (rules != 5u)                          ||
             (heat.rows.size() != rules)            ||
             (counts.revisits != heat.revisits)     ||
             (counts.touched != heat.touched)       ||
             (place != counts.revisits)             ||
             (heat.frames.size() != entered) )
        {
            return false;
        }

        // rule by rule, its row of the map
        for (std::size_t index = 0u; index < rules; index++)
        {
            const std::size_t row      = rules - 1u - index;
            std::size_t       steps    = 0u;
            std::size_t       arrivals = 0u;

            // the row's cells
            for (std::size_t col = 0u; col < heat.columns.size(); col++)
            {
                steps    += heat.steps.total({ col, row });
                arrivals += heat.entries.total({ col, row });
            }

            // a count the map does not give
            if ( (counts.rules[index].steps != steps)        ||
                 (counts.rules[index].arrivals != arrivals) )
            {
                return false;
            }
        }
    }

    const wb::run_record plain = nested_run(NESTED_INPUT,
                                            "none");
    const wb::run_record memo  = nested_run(NESTED_INPUT,
                                            "all");
    const wb::listing    part  = wb::part_listing(plain,
                                                  "profile");

    // a summary row's text, by its label
    const auto row_of = [](const wb::run_record& _run,
                           const std::string&    _label)
    {
        const wb::listing summary = wb::part_listing(_run,
                                                     "summary");

        // each row, for the one labelled so
        for (const std::vector<std::string>& row : summary.rows)
        {
            // the row asked for
            if ( (row.size() == 2u) &&
                 (row[0] == _label) )
            {
                return row[1];
            }
        }

        return std::string();
    };

    const std::string stacks    = row_of(memo,
                                         "stacks");
    const std::string memo_row  = row_of(memo,
                                         "memo");
    const std::string plain_row = row_of(plain,
                                         "memo");

    return ( (part.rows.size() == 7u)                             &&
             (part.rows[5][0] == "gate")                          &&
             (part.rows[6][0] == "all")                           &&
             (part.rows[6][7] == "7350")                          &&
             (plain.recognition.counts.revisits == 7350u)         &&
             (memo.recognition.counts.memo_hits > 0u)             &&
             (has(stacks,
                  "at most "))                                    &&
             (has(memo_row,
                  "results kept"))                                &&
             (plain_row == "none") );
}

// SECTIONS
//   constant: the suite, in the order it runs.
const section SECTIONS[] =
{
    { "tests_workbench_refusals", &tests_workbench_refusals },
    { "tests_workbench_parity",   &tests_workbench_parity   },
    { "tests_workbench_failure",  &tests_workbench_failure  },
    { "tests_workbench_reading",  &tests_workbench_reading  },
    { "tests_workbench_heat",     &tests_workbench_heat     },
    { "tests_workbench_store",    &tests_workbench_store    },
    { "tests_workbench_memo",     &tests_workbench_memo     },
    { "tests_workbench_profile",  &tests_workbench_profile  }
};

}  // anonymous namespace

/*
main
  Runs every section, then the summary line the build's test matches.
*/
int main()
{
    return ::vparse::tests::run_sections("vparse workbench core tests",
                                         SECTIONS);
}

#endif  // floor, for now
