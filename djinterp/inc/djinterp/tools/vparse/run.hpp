/*******************************************************************************
* vparse [vparse]                                                        run.hpp
*
* One vparse run, kept whole: every stage, its trace, and what the trace says.
*   A run takes a grammar text and an input through vparse's three stages and
* records each one:
*
*     reading     the grammar text becomes rules. For notation this is itself
*                 a PEG run (the meta-grammar's program over the text), so it
*                 is traced like any other; for EBNF it is ebnf::parse.
*     building    the rules become a PEG program -- through a generation plan
*                 for notation, directly for EBNF.
*     running     the program runs over the input.
*
*   From each traced PEG run the record derives the rules that matched (a tree
* of spans over the text) and, when the run failed, its frontier: the furthest
* character reached, the rules active there, and what they expected. Every
* part of the run is also laid out as a listing -- rows of text -- which is
* what the workbench displays, compares between runs, and saves to files.
*   Every PEG run is counted as it runs, traced or not (peg::profile): its
* steps, arrivals and revisits by rule, its calls and the calls it repeated,
* and its stacks' peaks. A request can have the program call some of its
* rules through the memo: every rule, the rules it names, or the rules a run
* without the memo says the memo pays for.
*
* path:      /inc/djinterp/tools/vparse/run.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PARTS OF A RUN
    --------------
    1.  trace_step
    2.  span
    3.  frontier
    4.  parse_pass
    5.  listing
2.  THE RUN
    -------
    1.  run_request
    2.  run_record
    3.  refusal
3.  OPERATIONS
    ----------
    1.  Running
         1.  execute
         2.  memo_rules
         3.  frontend_for_file
    2.  Describing
         1.  headline
         2.  part_names / part_title
         3.  part_listing
         4.  escape / line_col
         5.  disassemble
         6.  refusal_of
*/

#ifndef DJINTERP_TOOLS_VPARSE_RUN_HPP
#define DJINTERP_TOOLS_VPARSE_RUN_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as the
// rest of the workbench is: its floor is uxoxo's.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <map>      // std::map
#include <memory>   // std::shared_ptr
#include <string>   // std::string
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "djinterp/cpp/modules/parsegen/vparse/gen.hpp"      // gen_program, plan
#include "djinterp/cpp/modules/parsegen/vparse/peg.hpp"      // program, capture
#include "djinterp/cpp/modules/parsegen/vparse/ruleset.hpp"  // ruleset


namespace vparse
{
namespace workbench
{

namespace vp = ::djinterp::parsegen::vparse;

// heat_space
//   struct: where a run spent its work; defined in workbench_heat.hpp.
struct heat_space;


//==============================================================================
// 1.  PARTS OF A RUN
//==============================================================================


// 1.1    trace_step
//------------------------------------------------------------------------------
// 1.1.1
// trace_step
//   struct: the machine's state before one dispatch, parsed from the line
// peg::run writes to machine::trace. The instruction is program[ip].
struct trace_step
{
    int ip    = 0;
    int off   = 0;
    int calls = 0;
    int back  = 0;
    int marks = 0;
    int caps  = 0;
};

// 1.2    span
//------------------------------------------------------------------------------
// 1.2.1
// span
//   struct: a rule that matched text [start, end), nested depth levels deep.
struct span
{
    std::string rule;
    int         start = 0;
    int         end   = 0;
    int         depth = 0;
};

// 1.3    frontier
//------------------------------------------------------------------------------
// 1.3.1
// frontier
//   struct: where a failing parse got furthest -- the offset, the rules that
// were active there (outermost first), what the failing steps expected, and
// the trace step of the last failure there.
struct frontier
{
    int                      offset = -1;
    std::vector<std::string> stack;
    std::vector<std::string> expected;
    int                      step   = -1;
};

// 1.4    parse_pass
//------------------------------------------------------------------------------
// 1.4.1
// parse_pass
//   struct: one PEG run over one text, what it counted as it ran, and what
// its trace shows.
struct parse_pass
{
    bool                          ran      = false;
    bool                          matched  = false;
    std::size_t                   consumed = 0;
    long                          steps    = 0;
    std::string                   error;
    std::vector<vp::peg::capture> captures;
    vp::peg::profile              counts;
    std::vector<trace_step>       trace;
    std::vector<span>             matches;   // pre-order: parents first
    frontier                      far;
    double                        ms = 0.0;
};

// 1.5    listing
//------------------------------------------------------------------------------
// 1.5.1
// listing
//   struct: one part of a run as rows of text. keys are what two runs are
// compared by (a row's text without its position), marks tint a row (0 plain,
// 1 muted, 2 error, 3 a rule entry), and note explains an empty listing.
struct listing
{
    std::vector<std::string>              columns;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string>              keys;
    std::vector<int>                      marks;
    std::string                           note;
};


//==============================================================================
// 2.  THE RUN
//==============================================================================


// 2.1    run_request
//------------------------------------------------------------------------------
// 2.1.1
// run_request
//   struct: what to run. frontend is "notation" or "ebnf"; memoize is which
// rules the program calls through the memo -- "none", "all", "auto", or
// rule names separated by commas (memo_rules); origin says where the request
// came from (typed in, or a saved run's folder).
struct run_request
{
    std::string frontend   = "notation";
    std::string grammar;
    std::string input;
    bool        trace      = true;
    long        step_limit = 100000;
    std::string memoize    = "none";
    std::string origin;
};

// 2.2    run_record
//------------------------------------------------------------------------------
// 2.2.1
// run_record
//   struct: a finished run, every stage kept.
struct run_record
{
    int         id = 0;
    std::string when;
    run_request request;

    // reading the grammar
    bool                     read_ok = false;
    std::string              read_error;
    parse_pass               reading;       // notation: the meta-grammar run
    vp::peg::program         reader;        // notation: the meta-program
    std::vector<std::string> reader_entry;  // its rule names by address
    vp::ruleset              rules;         // notation
    std::vector<std::string> rule_names;    // both front ends, in order
    std::vector<std::string> ebnf_rules;    // EBNF: "name = definition"
    std::vector<std::string> ebnf_tree;     // EBNF: the syntax tree, indented
    double                   read_ms = 0.0;

    // building the program
    bool                     built = false;
    std::string              build_error;
    vp::gen::gen_program     plan;          // notation only
    vp::peg::program         program;
    std::vector<std::string> entry;         // rule name by address, or ""
    double                   build_ms = 0.0;

    // the memo: the rules the program calls through it, in program order,
    // and, where "auto" chose them, the steps the program took without it
    std::vector<std::string> memoized;
    long                     unmemoized_steps = -1;

    // running the program over the input
    parse_pass recognition;

    // where the recognition pass spent its work (null without a trace)
    std::shared_ptr<const heat_space> heat;

    // every part as a listing, keyed by part name (trace parts excepted)
    std::map<std::string, listing> parts;
};


// 2.3    refusal
//------------------------------------------------------------------------------
// 2.3.1
// refusal
//   struct: why a run's grammar was refused, for a diagnostic that puts the
// place first, as a compiler writes one: the 1-based line and column in the
// grammar where its reader stopped, both 0 where there is no place -- the
// reader's step limit, the EBNF reader, a program that was not built -- and
// the headline's sentence, the place taken out of it.
struct refusal
{
    int         line   = 0;
    int         column = 0;
    std::string sentence;
};


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


// 3.1    Running
//------------------------------------------------------------------------------
// 3.1.1
// execute
//   function: runs _request through every stage and returns the record, with
// its listings built. Never throws for a bad grammar or input; problems are
// recorded in the run.
run_record execute(
    int                _id,
    const run_request& _request
);

// 3.1.2
// memo_rules
//   function: the rules a memo setting names, by their first addresses in
// _run's program, in program order: every rule for "all"; none for "none"
// or nothing; and for names separated by commas, each rule named. A name no
// rule has is written to _unknown, when it is given, and none is then
// returned. "auto" names no rule: execute() chooses its rules from a run.
std::vector<int> memo_rules(
    const run_record&  _run,
    const std::string& _setting,
    std::string*       _unknown
);

// 3.1.3
// frontend_for_file
//   function: the frontend a grammar's file name calls for, when nothing else
// says which: "ebnf" for a name that ends in ".ebnf", in any case, and
// "notation" for any other. Both programs open a grammar file by it.
std::string frontend_for_file(
    const std::string& _name
);


// 3.2    Describing
//------------------------------------------------------------------------------
// 3.2.1
// headline
//   function: the run's outcome as one sentence.
std::string headline(
    const run_record& _run
);

// 3.2.2
// part_names / part_title
//   functions: the parts of a run in display order, and each one's title.
const std::vector<std::string>& part_names();
std::string part_title(
    const std::string& _part
);

// 3.2.3
// part_listing
//   function: the listing for _part. The trace parts are built here on demand
// (they can run to a hundred thousand rows); the rest come from run.parts.
listing part_listing(
    const run_record&  _run,
    const std::string& _part
);

// trace_row
//   function: one row of a trace listing, built without building the rest.
std::vector<std::string> trace_row(
    const run_record&  _run,
    const std::string& _part,
    std::size_t        _row
);

// trace_rows
//   function: how many rows a trace part has.
std::size_t trace_rows(
    const run_record&  _run,
    const std::string& _part
);

// 3.2.4
// escape / line_col
//   functions: control characters written visibly (\n, \t, \x01), and the
// 1-based line and column of _offset in _text.
std::string escape(
    const std::string& _text
);
std::pair<int, int> line_col(
    const std::string& _text,
    int                _offset
);

// 3.2.5
// disassemble
//   function: one instruction as vparse's fmt writes it, with a SET's members
// compressed to ranges ("SET [0-9]") and control characters escaped.
std::string disassemble(
    const vp::peg::DInstr& _instr
);

// 3.2.6
// refusal_of
//   function: the refusal of a run whose grammar was refused; for any other
// run, its headline, with no place.
refusal refusal_of(
    const run_record& _run
);

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now

#endif  // DJINTERP_TOOLS_VPARSE_RUN_HPP
