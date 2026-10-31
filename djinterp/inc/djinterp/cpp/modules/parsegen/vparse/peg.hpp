/*******************************************************************************
* djinterp [cpp/parsegen]                                                peg.hpp
*
*   The PEG operator family, rebased onto the parse carrier.  run() consumes a
* parse_state<char> (parsing from its offset) and follows the formal match-or-
* restore contract: on success it advances the state's offset to the match end
* and, if a sink is given, hands back the tagged capture spans; on failure it
* leaves the offset unchanged.  Opcodes are family-private; MARK / CAP are
* backtrack-safe.  A compiled program becomes a first-class parser<R, char> via
* the adapter (adapter.hpp).
*   A run can also count what it does, as it does it: steps, arrivals and
* revisits by rule and by position, calls and the calls repeated at a position
* already tried, and each stack's peak -- a profile, which costs no trace
* line, and agrees with the heat map a trace gives. MEMO is CALL through a
* memo table: a rule's result at a position is kept, and replayed on every
* later call there, captures and all. memoized() writes it for the rules a
* profile says a memo pays for (worth_memoizing()), and a PEG's language does
* not change.
*
*
* path:      /inc/djinterp/cpp/modules/parsegen/vparse/peg.hpp
* link(s):   TBA
* author(s): vparse                                          created: 2026.06.19
*                                                            revised: 2026.10.17
*******************************************************************************/

#ifndef DJINTERP_PARSEGEN_VPARSE_PEG_HPP
#define DJINTERP_PARSEGEN_VPARSE_PEG_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <string>   // std::string
#include <vector>   // std::vector
// djinterp
#include "./machine.hpp"            // machine, op_set
#include "../../parse/parse.hpp"    // parse_state, and the carrier's results

NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

// the parse carrier vparse builds on (djinterp::parse)
using ::djinterp::parse::parse_state;
using ::djinterp::parse::parse_result;
using ::djinterp::parse::parse_error;
using ::djinterp::parse::DParseStatus;
using ::djinterp::parse::DParseStatusSuccess;
using ::djinterp::parse::DParseStatusFailure;
using ::djinterp::parse::DParseStatusEndOfInput;
using ::djinterp::parse::DParseStatusMalformed;

// peg
//   namespace: the PEG operator family.
namespace peg {

// opcodes
//   enum: this family's private opcode space. MEMO is a CALL made through
// the run's memo table: the first call of its rule at a position runs the
// rule, as CALL does, and keeps where it ended, or that it failed, with the
// captures it made; every later call there replays that, in one step.
enum
{
    CHAR = 0, ANY, CHOICE, JUMP, CALL, RETURN, COMMIT, FAIL, MATCH, SET,
    MARK, CAP, MEMO
};

// instr
//   struct: one PEG instruction (arg is a jump target or a capture tag).
struct DInstr
{
    int         op;
    int         arg = -1;
    char        ch  = 0;
    std::string set;
};

// program
//   type: a flat PEG instruction stream.
using program = std::vector<DInstr>;

// capture
//   struct: one tagged span -- the tag and the half-open input range.
struct capture
{
    int tag;
    int start;
    int end;
};

// layout
//   struct: where a program's rules are. Both frontends lay the rules out
// after the end-of-input gate (which ends one past the first MATCH), in
// order, each ending in its one RETURN; a program is `regular` when every
// CALL and MEMO lands on the start of one of them. In a program that is
// not, the rules start where its calls land. `owner` is, for each address,
// the index in `starts` of the rule whose code it is: the last that starts
// at or before it, and -1 before the first, the gate among them.
struct layout
{
    std::vector<int> starts;
    std::vector<int> owner;
    bool             regular = false;
};

// profile
//   struct: what a run did, counted as it ran rather than read back from a
// trace. A step is an instruction dispatched, the one that ended the run
// among them, as the trace lists them; each is counted at the cursor before
// it runs, in the rule whose code it is (layout_of()), and none of the
// end-of-input gate's is a rule's.
//   An arrival is a step at a cell -- a position and a rule -- other than
// the step before's, and any step after one of the gate's; a revisit is an
// arrival at a cell arrived at before. These are the heat map's counts, by
// rule and by position: each set sums to `revisits`.
//   A call is a CALL, or a MEMO, of a rule, whether the memo answered it or
// the rule ran; a repeat is a call at a position the rule was called at
// before, and its steps are those the rule ran for it, from the rule's first
// instruction to its RETURN, to the failure that unwound it, or to the end
// of the run -- the steps a memo would save.
struct profile
{
    // DRuleCounts
    //   struct: one rule's counts, the rule named by its first address.
    struct DRuleCounts
    {
        int         start        = -1;
        std::size_t steps        = 0u;
        std::size_t arrivals     = 0u;
        std::size_t revisits     = 0u;
        std::size_t calls        = 0u;
        std::size_t repeats      = 0u;
        std::size_t repeat_steps = 0u;
        std::size_t hits         = 0u;
    };

    // per rule, in layout order; per position of the input, 0 to its length
    std::vector<DRuleCounts> rules;
    std::vector<std::size_t> revisits_at;

    // the run as a whole, the gate's steps among `steps`
    std::size_t              steps        = 0u;
    std::size_t              gate_steps   = 0u;
    std::size_t              touched      = 0u;
    std::size_t              revisits     = 0u;

    // the most each stack held at once
    std::size_t              peak_calls   = 0u;
    std::size_t              peak_back    = 0u;
    std::size_t              peak_marks   = 0u;
    std::size_t              peak_caps    = 0u;

    // the memo: the results it held at the end, which is the most it held,
    // and the calls it answered and the calls it did not
    std::size_t              memo_entries = 0u;
    std::size_t              memo_hits    = 0u;
    std::size_t              memo_misses  = 0u;
};

// sinks
//   struct: where run() puts what it found besides its verdict, each left
// alone while it is null: the capture spans, on a match, and the profile,
// whatever the outcome.
struct sinks
{
    std::vector<capture>* captures = nullptr;
    profile*              counts   = nullptr;
};

// MEMO_COST
//   constant: what keeping a rule's result and looking it up again costs a
// call, in steps: the time a call through the memo takes beyond a CALL's,
// over the time a step takes, as measured on 2026.10.17.
const std::size_t MEMO_COST = 4u;

op_set make_ops();
bool run(
    parse_state<char>&    _state,
    const program&        _prog,
    const op_set&         _ops,
    std::vector<capture>* _caps = nullptr
);
std::string fmt(
    const DInstr& _ins
);

// run (caller's machine)
//   function: run() on a machine the caller supplies, as lr::run takes one.
// Its step_limit is honoured, and when its trace is set, one line is appended
// per dispatch, before the operator runs:
//
//     ip=<ip> off=<cursor> calls=<n> back=<n> marks=<n> caps=<n> | <fmt(instr)>
//
// Everything before " | " is space-separated key=value integers; everything
// after it is the instruction's disassembly. On return, steps / ok / halted /
// error describe the run and ext is cleared. The four-argument run() above is
// this with a default machine.
bool run(
    machine&              _machine,
    parse_state<char>&    _state,
    const program&        _prog,
    const op_set&         _ops,
    std::vector<capture>* _caps = nullptr
);

/**
 * @brief Runs a program on the caller's machine, as the run() above does,
 *        handing back what the sinks ask for.
 *
 * @note A profile costs a few counts a step, and a bit for each position of
 *       the input in each rule that runs there; none of it is a string.
 *
 * @param[in,out] _machine  the machine: its step limit and trace apply, and
 *                          on return it describes the run.
 * @param[in,out] _state    the input, parsed from its offset.
 * @param[in]     _prog     the program.
 * @param[in]     _ops      the operators it is dispatched through.
 * @param[in]     _sinks    where the captures and the profile go.
 * @post   On a match the offset is where the match ended, and the captures
 *         sink holds the spans; otherwise neither has changed. The profile
 *         sink, if any, holds this run's counts and nothing before them.
 * @return whether the program matched.
 */
bool run(
    machine&           _machine,
    parse_state<char>& _state,
    const program&     _prog,
    const op_set&      _ops,
    const sinks&       _sinks
);

/**
 * @brief Finds where a program's rules are.
 *
 * @param[in] _prog  the program.
 * @return the start of each rule, in address order, the rule each address
 *         belongs to, and whether every call lands on a rule's start.
 */
layout layout_of(
    const program& _prog
);

/**
 * @brief Gives a program whose calls to some of its rules go through the
 *        memo.
 *
 * @note A rule called through the memo must leave the mark and backtrack
 *       stacks as it found them, as every rule both frontends lay out does:
 *       its result is then its own, whatever called it, and replaying it is
 *       running it again.
 *
 * @param[in] _prog    the program.
 * @param[in] _starts  the rules to memoize, by their first addresses.
 * @return `_prog` with every CALL to one of them made a MEMO to the same
 *         address; the same size, so no address moves.
 */
program memoized(
    const program&          _prog,
    const std::vector<int>& _starts
);

/**
 * @brief Chooses the rules a memo pays for, from a profile of runs made
 *        without one.
 *
 * @note A rule is chosen when the steps its repeated calls ran come to more
 *       than MEMO_COST steps for each call of it. A rule the memo already
 *       answers has run no repeated call to count, so the profile to choose
 *       from is a run of the program before memoized().
 *
 * @param[in] _profile  the counts.
 * @return the chosen rules' first addresses, in layout order.
 */
std::vector<int> worth_memoizing(
    const profile& _profile
);

/**
 * @brief Adds one run's counts to another's, as one profile of both runs.
 *
 * @param[in,out] _total  the counts so far; an empty profile takes the
 *                        rules of the first run added to it.
 * @param[in]     _run    a run of the same program.
 * @post   Each count in `_total` is the sum of the runs', each peak the
 *         greater, and the revisits by position are summed position by
 *         position, as far as the longer input.
 */
void accumulate(
    profile&       _total,
    const profile& _run
);

}  // namespace peg

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_PARSEGEN_VPARSE_PEG_HPP
