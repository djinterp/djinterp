/*******************************************************************************
* djinterp [cpp/parsegen]                                  vparse_tests_memo.cpp
*
* vparse tests: what a run counts as it runs, and the memo -- where a
* program's rules are, a profile held to the trace of the same run, a rule's
* result replayed with its captures and unwound as they are, a memoized
* program recognising what the program does, the nested expression grammar
* made linear, and the rules a profile says a memo pays for.
*   Where a count could come from the code under test, it does not: the
* profile's steps, arrivals, revisits, calls, repeats and peaks are counted
* again here from the trace the same run wrote, as the heat map counts them.
*   No section depends on the language level, so every level from the floor
* up runs all of them.
*
*
* path:      /tests/djinterp/cpp/modules/parsegen/vparse/vparse_tests_memo.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <algorithm>         // std::max, std::find
#include <cstddef>           // std::size_t
#include <cstdio>            // std::sscanf
#include <initializer_list>  // std::initializer_list
#include <set>               // std::set
#include <string>            // std::string
#include <utility>           // std::pair, std::make_pair
#include <vector>            // std::vector


namespace djinterp
{
namespace testing
{

namespace
{

// NESTED
//   constant: expressions, every alternative of Sum and of Prod beginning
// with the same rule, so that each level of parentheses parses the level
// inside it nine times over.
const char* const NESTED =
    "Expr = Sum ;\n"
    "Sum  = Prod '+' Sum | Prod '-' Sum | Prod ;\n"
    "Prod = Atom '*' Prod | Atom '/' Prod | Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// FACTORED
//   constant: the same expressions, each rule's shared beginning factored
// out, so that nothing is parsed twice at a position.
const char* const FACTORED =
    "Expr = Sum ;\n"
    "Sum  = Prod More* ;\n"
    "More = '+' Prod | '-' Prod ;\n"
    "Prod = Atom Then* ;\n"
    "Then = '*' Atom | '/' Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// GATE
//   constant: how many instructions the end-of-input gate takes, before the
// first rule.
const int GATE = 6;

// outcome
//   struct: what one run of a program gave: its verdict, how far it read,
// its captures, its steps, its profile, and its trace when one was asked
// for.
struct outcome
{
    bool                          ok       = false;
    std::size_t                   consumed = 0u;
    std::vector<vp::peg::capture> caps;
    long                          steps    = 0;
    vp::peg::profile              counts;
    std::vector<std::string>      trace;
};

// line
//   struct: one trace line's fields.
struct line
{
    int ip    = -1;
    int off   = -1;
    int calls = 0;
    int back  = 0;
    int marks = 0;
    int caps  = 0;
};

/**
 * @brief Builds the program for grammar text in the notation.
 *
 * @param[in] _text  the grammar.
 * @return the generator's program for it; empty where the text is not a
 *         grammar.
 */
vp::peg::program program_of(
    const char* _text
)
{
    bool                read    = false;
    const vp::ruleset   grammar = vp::notation::parse_grammar(_text,
                                                              &read);

    return read ? vp::gen::compile(grammar)
                : vp::peg::program();
}

/**
 * @brief Runs a program over an input, counting it, and tracing it when
 *        asked.
 *
 * @param[in] _program  the program.
 * @param[in] _input    the input.
 * @param[in] _traced   whether to keep the trace.
 * @return what the run gave.
 */
outcome counted(
    const vp::peg::program& _program,
    const std::string&      _input,
    bool                    _traced = false
)
{
    outcome               out;
    pr::parse_state<char> state(_input.data(),
                                _input.size(),
                                0);
    vp::machine           machine;
    vp::peg::sinks        sinks;

    machine.step_limit = 100000000L;
    sinks.captures     = &out.caps;
    sinks.counts       = &out.counts;

    // a line a dispatch, where the trace is wanted
    if (_traced)
    {
        machine.trace = &out.trace;
    }

    out.ok       = vp::peg::run(machine,
                                state,
                                _program,
                                peg_ops(),
                                sinks);
    out.consumed = out.ok ? state.offset
                          : 0u;
    out.steps    = machine.steps;

    return out;
}

/**
 * @brief Reads a trace line's fields.
 *
 * @param[in] _text  the line, as run() writes it.
 * @return its fields; an ip of -1 where the line does not read.
 */
line read_line(
    const std::string& _text
)
{
    line out;

    // the six fields before the instruction, or none of them
    if (std::sscanf(_text.c_str(),
                    "ip=%d off=%d calls=%d back=%d marks=%d caps=%d",
                    &out.ip,
                    &out.off,
                    &out.calls,
                    &out.back,
                    &out.marks,
                    &out.caps) != 6)
    {
        out.ip = -1;
    }

    return out;
}

/**
 * @brief Writes an input nested in parentheses.
 *
 * @param[in] _depth  how many pairs.
 * @return `1` inside `_depth` pairs of parentheses.
 */
std::string nest(
    std::size_t _depth
)
{
    const std::string open(_depth,
                           '(');
    const std::string close(_depth,
                            ')');

    return open + "1" + close;
}

/**
 * @brief Says whether two runs recognised an input alike.
 *
 * @param[in] _one    a run.
 * @param[in] _other  another run.
 * @return whether both matched or both did not, read as far, and made the
 *         same captures in the same order.
 */
bool alike(
    const outcome& _one,
    const outcome& _other
)
{
    // the verdict, and how far each read
    if ( (_one.ok != _other.ok)              ||
         (_one.consumed != _other.consumed)  ||
         (_one.caps.size() != _other.caps.size()) )
    {
        return false;
    }

    // capture by capture
    for (std::size_t at = 0u; at < _one.caps.size(); at++)
    {
        // a capture that differs in its tag or its span
        if ( (_one.caps[at].tag != _other.caps[at].tag)      ||
             (_one.caps[at].start != _other.caps[at].start)  ||
             (_one.caps[at].end != _other.caps[at].end) )
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Says whether a profile is the one its run's trace gives.
 *
 * @param[in] _run      a traced, counted run.
 * @param[in] _program  the program it ran.
 * @return whether the steps, the arrivals, revisits and steps of each rule,
 *         the revisits by position, the calls and repeats of each rule, and
 *         the stacks' peaks, counted again from the trace as the heat map
 *         counts them, are the profile's.
 */
bool agrees_with_trace(
    const outcome&          _run,
    const vp::peg::program& _program
)
{
    const vp::peg::layout   where = vp::peg::layout_of(_program);
    const vp::peg::profile& p     = _run.counts;
    const std::size_t       rules = where.starts.size();

    const std::vector<std::size_t>  none(rules,
                                         0u);
    std::vector<std::size_t>        steps    = none;
    std::vector<std::size_t>        arrivals = none;
    std::vector<std::size_t>        revisits = none;
    std::vector<std::size_t>        calls    = none;
    std::vector<std::size_t>        repeats  = none;
    std::vector<std::size_t>        by_position(p.revisits_at.size(),
                                                0u);
    std::set<std::pair<int, int> >  cells;
    std::set<std::pair<int, int> >  called;
    line                            peak;
    int                             last_rule = -1;
    int                             last_off  = -1;

    std::size_t rule_steps = p.gate_steps;
    std::size_t rule_sum   = 0u;
    std::size_t place_sum  = 0u;

    // the rules' steps and revisits, summed
    for (const vp::peg::profile::DRuleCounts& each : p.rules)
    {
        rule_steps += each.steps;
        rule_sum   += each.revisits;
    }

    // and the revisits by position
    for (const std::size_t each : p.revisits_at)
    {
        place_sum += each;
    }

    // a line a step; the rules' steps and the gate's make them all, and the
    // revisits by rule and by position each make the run's
    if ( (p.steps != _run.trace.size())  ||
         (rule_steps != p.steps)         ||
         (rule_sum != p.revisits)        ||
         (place_sum != p.revisits)       ||
         (p.rules.size() != rules) )
    {
        return false;
    }

    // the trace, a line at a time, as the heat map reads it
    for (const std::string& text : _run.trace)
    {
        const line now = read_line(text);

        // a line that does not read
        if (now.ip < 0)
        {
            return false;
        }

        const int rule = where.owner[static_cast<std::size_t>(now.ip)];

        peak.calls = std::max(peak.calls,
                              now.calls);
        peak.back  = std::max(peak.back,
                              now.back);
        peak.marks = std::max(peak.marks,
                              now.marks);
        peak.caps  = std::max(peak.caps,
                              now.caps);

        // a rule's step, and an arrival where the cell is new to this step
        if (rule >= 0)
        {
            const std::size_t index = static_cast<std::size_t>(rule);

            steps[index]++;

            // an arrival, and a revisit where the cell was arrived at before
            if ( (rule != last_rule) ||
                 (now.off != last_off) )
            {
                arrivals[index]++;

                // a cell arrived at before
                if (!cells.insert(std::make_pair(now.off,
                                                 rule)).second)
                {
                    revisits[index]++;
                    by_position[static_cast<std::size_t>(now.off)]++;
                }
            }
        }

        last_rule = rule;
        last_off  = now.off;

        const vp::peg::DInstr& ins = _program[static_cast<std::size_t>(now.ip)];

        // a call, and whether its rule was called at this position before
        if ( (ins.op == vp::peg::CALL) ||
             (ins.op == vp::peg::MEMO) )
        {
            const int callee = where.owner[static_cast<std::size_t>(ins.arg)];

            // a call of a rule
            if (callee >= 0)
            {
                calls[static_cast<std::size_t>(callee)]++;

                // the rule called here before
                if (!called.insert(std::make_pair(now.off,
                                                  callee)).second)
                {
                    repeats[static_cast<std::size_t>(callee)]++;
                }
            }
        }
    }

    // rule by rule
    for (std::size_t index = 0u; index < rules; index++)
    {
        const vp::peg::profile::DRuleCounts& each = p.rules[index];

        // a count the trace does not give
        if ( (each.start != where.starts[index])  ||
             (each.steps != steps[index])         ||
             (each.arrivals != arrivals[index])   ||
             (each.revisits != revisits[index])   ||
             (each.calls != calls[index])         ||
             (each.repeats != repeats[index]) )
        {
            return false;
        }
    }

    return ( (by_position == p.revisits_at)                               &&
             (p.touched == cells.size())                                  &&
             (p.peak_calls == static_cast<std::size_t>(peak.calls))       &&
             (p.peak_back == static_cast<std::size_t>(peak.back))         &&
             (p.peak_marks == static_cast<std::size_t>(peak.marks))       &&
             (p.peak_caps == static_cast<std::size_t>(peak.caps)) );
}

}  // anonymous namespace


/*
tests_memo_layout
  Where a program's rules are, and what memoizing some of them changes.
  Tests the following:
  - the generator's program is regular: its rules start after the gate, in
    order, and every address is its rule's, the gate's no rule's
  - a program whose call lands inside a rule is not, and its rules start
    where its calls land
  - memoized() turns the calls to the rules it is given into MEMOs to the
    same addresses, and changes nothing else; MEMO prints with its address
*/
bool tests_memo_layout()
{
    using namespace vp::peg;

    const program nested = program_of(NESTED);
    const layout  where  = layout_of(nested);

    // five rules after the gate, each address its rule's
    if ( (!where.regular)                  ||
         (where.starts.size() != 5u)       ||
         (where.starts[0] != GATE)         ||
         (where.owner.size() != nested.size()) )
    {
        return false;
    }

    // every address, the last rule to start at or before it
    for (std::size_t at = 0u; at < nested.size(); at++)
    {
        const int expected = static_cast<int>(
            std::upper_bound(where.starts.begin(),
                             where.starts.end(),
                             static_cast<int>(at)) -
            where.starts.begin()) - 1;

        // an address given to a rule that does not start at or before it
        if (where.owner[at] != expected)
        {
            return false;
        }
    }

    // after a gate of two, a rule from 2 to its RETURN: a call to 2 lands on
    // its start, and a call to 3 inside it
    const program fits   = { ins(CALL, 2), ins(MATCH), ins(CHAR, -1, 'a'),
                             ins(CHAR, -1, 'b'), ins(RETURN) };
    const program inside = { ins(CALL, 2), ins(MATCH), ins(CALL, 3),
                             ins(CHAR, -1, 'b'), ins(RETURN) };
    const layout  split  = layout_of(inside);

    // read by its RETURNs, and by where its calls land
    if ( (!layout_of(fits).regular)                ||
         (split.regular)                           ||
         (split.starts != std::vector<int>{ 2, 3 }) ||
         (split.owner[2] != 0)                     ||
         (split.owner[4] != 1) )
    {
        return false;
    }

    const std::vector<int> chosen = { where.starts[1], where.starts[2] };
    const program          memo   = memoized(nested,
                                             chosen);

    // the same size: nothing moved
    if (memo.size() != nested.size())
    {
        return false;
    }

    // the same program, but for the calls to the chosen rules
    for (std::size_t at = 0u; at < nested.size(); at++)
    {
        const bool to_chosen = ( (nested[at].op == CALL) &&
                                 (std::find(chosen.begin(),
                                            chosen.end(),
                                            nested[at].arg) != chosen.end()) );

        // a MEMO where a chosen call was, and the instruction elsewhere
        if ( (memo[at].op != (to_chosen ? static_cast<int>(MEMO)
                                        : nested[at].op))  ||
             (memo[at].arg != nested[at].arg) )
        {
            return false;
        }
    }

    return (fmt(ins(MEMO, 8)) == "MEMO 8");
}

/*
tests_memo_profile
  A profile is what the trace of the same run says, counted the heat map's
way.
  Tests the following:
  - its steps are the trace's lines, the rules' and the gate's together
  - each rule's steps, arrivals, revisits, calls and repeats, the revisits by
    position and the cells touched are those the trace gives, and the
    revisits by rule and by position each sum to the run's
  - the peaks are the deepest each stack is in the trace
  - so for a run that matches, one that does not, a memoized one, and one
    whose captures are made in a rule that is backtracked out of
  - a profile counted twice holds the second run alone
*/
bool tests_memo_profile()
{
    const vp::peg::program nested  = program_of(NESTED);
    const vp::peg::layout  where   = vp::peg::layout_of(nested);
    const vp::peg::program memo    = vp::peg::memoized(nested,
                                                       where.starts);
    const vp::peg::program grammar = vp::gen::compile(
                                         vp::notation::meta_grammar());

    // each run, held to its trace
    for (const std::string& input :
         { std::string("(1+2)*3"), nest(3), std::string("(1+") })
    {
        const outcome plain   = counted(nested,
                                        input,
                                        true);
        const outcome by_memo = counted(memo,
                                        input,
                                        true);

        // the plain program and the memoized one
        if ( (!agrees_with_trace(plain,
                                 nested)) ||
             (!agrees_with_trace(by_memo,
                                 memo)) )
        {
            return false;
        }
    }

    // the grammar of grammars makes captures, and backtracks out of some
    const outcome reading = counted(grammar,
                                    FACTORED,
                                    true);

    // its profile, held to its trace
    if (!agrees_with_trace(reading,
                           grammar))
    {
        return false;
    }

    const std::string     empty;
    const std::string     input = nest(2);
    pr::parse_state<char> first(empty.data(),
                                empty.size(),
                                0);
    vp::machine           machine;
    vp::peg::sinks        sinks;
    vp::peg::profile      counts;

    // one profile, counted for two runs, one after the other
    sinks.counts = &counts;
    (void)vp::peg::run(machine,
                       first,
                       nested,
                       peg_ops(),
                       sinks);

    pr::parse_state<char> second(input.data(),
                                 input.size(),
                                 0);

    (void)vp::peg::run(machine,
                       second,
                       nested,
                       peg_ops(),
                       sinks);

    const outcome alone = counted(nested,
                                  input);

    return ( (counts.steps == alone.counts.steps)                &&
             (counts.revisits_at.size() == (input.size() + 1u)) );
}

/*
tests_memo_replay
  A rule's result, kept and replayed: invariant 5 for captures a run did not
make again.
  Tests the following:
  - a rule that makes a capture, called again at the same position after a
    backtrack, is answered by the memo: the capture is replayed, and is
    unwound by the next backtrack as a capture made there would be
  - a rule's failure is kept, and a later call there fails without running
  - the memo's counts: one result kept, answered twice
  - the memoized program's verdict, length and captures are the program's,
    in fewer steps where the memo answered
*/
bool tests_memo_replay()
{
    // S = A 'b' | A 'c' | A 'd' ;  A = 'a'#3 'a'*
    vp::ruleset g;

    g.rules.push_back(vp::rule{ "S",
        { { vp::ref("A"), vp::lit('b') },
          { vp::ref("A"), vp::lit('c') },
          { vp::ref("A"), vp::lit('d') } } });
    g.rules.push_back(vp::rule{ "A",
        { { vp::lit('a', 3), vp::cls("a", true) } } });

    const vp::peg::program plain = vp::gen::compile(g);
    const vp::peg::layout  where = vp::peg::layout_of(plain);

    // two rules, the second the one to memoize
    if (where.starts.size() != 2u)
    {
        return false;
    }

    const vp::peg::program memo = vp::peg::memoized(plain,
                                                    { where.starts[1] });

    const outcome third  = counted(plain,
                                   "aaad");
    const outcome thrice = counted(memo,
                                   "aaad");
    const outcome none   = counted(plain,
                                   "c");
    const outcome failed = counted(memo,
                                   "c");
    const outcome first  = counted(memo,
                                   "aab");

    return ( (alike(third,
                    thrice))                                &&
             (thrice.ok)                                    &&
             (thrice.caps.size() == 1u)                     &&
             (thrice.caps[0].tag == 3)                      &&
             (thrice.caps[0].start == 0)                    &&
             (thrice.caps[0].end == 1)                      &&
             (thrice.counts.memo_misses == 1u)              &&
             (thrice.counts.memo_hits == 2u)                &&
             (thrice.counts.memo_entries == 1u)             &&
             (thrice.counts.rules[1].hits == 2u)            &&
             (thrice.counts.peak_caps == 1u)                &&
             (thrice.steps < third.steps)                   &&
             (alike(none,
                    failed))                                &&
             (!failed.ok)                                   &&
             (failed.counts.memo_hits == 2u)                &&
             (failed.counts.memo_entries == 1u)             &&
             (first.ok)                                     &&
             (first.counts.memo_hits == 0u)                 &&
             (first.counts.memo_misses == 1u) );
}

/*
tests_memo_language
  Memoizing keeps a PEG's language.
  Tests the following:
  - the expression grammar, with every rule memoized or with Sum and Prod,
    accepts what the program accepts, refuses what it refuses, and reads as
    far
  - the grammar of grammars, every rule memoized, reads grammar texts to the
    same captures, and refuses what is not one at the same place
*/
bool tests_memo_language()
{
    const vp::peg::program nested  = program_of(NESTED);
    const vp::peg::layout  where   = vp::peg::layout_of(nested);
    const vp::peg::program all     = vp::peg::memoized(nested,
                                                       where.starts);
    const vp::peg::program two     = vp::peg::memoized(nested,
                                                       { where.starts[1],
                                                         where.starts[2] });
    const vp::peg::program grammar = vp::gen::compile(
                                         vp::notation::meta_grammar());
    const vp::peg::program reader  = vp::peg::memoized(
                                         grammar,
                                         vp::peg::layout_of(grammar).starts);

    // inputs the grammar derives, and inputs it does not
    for (const char* input : { "1", "(1)", "1+2*3", "((1+2)*3)/4-5", "12",
                               "", "(", "1+", "(1))", "1**2", "+1" })
    {
        const outcome plain = counted(nested,
                                      input);
        const outcome every = counted(all,
                                      input);
        const outcome some  = counted(two,
                                      input);

        // either memoized program read otherwise
        if ( (!alike(plain,
                     every)) ||
             (!alike(plain,
                     some)) )
        {
            return false;
        }
    }

    // grammars in the notation, and texts that are not
    for (const char* text : { NESTED, FACTORED, "Digits = [0-9]+ ;",
                              "S = [a-z] T ; T = 'x' ;", "S = [0-9",
                              "= ;" })
    {
        const outcome plain      = counted(grammar,
                                           text);
        const outcome remembered = counted(reader,
                                           text);

        // the reader memoized reads the text otherwise
        if (!alike(plain,
                   remembered))
        {
            return false;
        }
    }

    return (counted(grammar,
                    NESTED).caps.size() > 0u);
}

/*
tests_memo_nesting
  The nested grammar, nine times the steps a level without the memo, is
linear with Sum and Prod memoized.
  Tests the following:
  - without the memo, each level of parentheses costs at least eight times
    the steps of the level inside it
  - with Sum and Prod memoized, each level costs the same number of steps
    more than the last, from 1 to 12 levels, and the memo keeps the same
    number of results more
  - at every depth the memoized program matches as the program does
*/
bool tests_memo_nesting()
{
    const vp::peg::program nested = program_of(NESTED);
    const vp::peg::layout  where  = vp::peg::layout_of(nested);
    const vp::peg::program memo   = vp::peg::memoized(nested,
                                                      { where.starts[1],
                                                        where.starts[2] });

    // without the memo, nine times a level
    for (std::size_t depth = 1u; depth <= 4u; depth++)
    {
        const outcome inner = counted(nested,
                                      nest(depth - 1u));
        const outcome outer = counted(nested,
                                      nest(depth));

        // a level that cost less than eight times the one inside it
        if ( (!outer.ok) ||
             (outer.steps < (8 * inner.steps)) )
        {
            return false;
        }
    }

    const outcome zero = counted(memo,
                                 nest(0u));
    const outcome one  = counted(memo,
                                 nest(1u));
    const long    step = one.steps - zero.steps;
    const long    kept = static_cast<long>(one.counts.memo_entries) -
                         static_cast<long>(zero.counts.memo_entries);

    // with it, the same few steps a level, and the same few results
    for (std::size_t depth = 1u; depth <= 12u; depth++)
    {
        const outcome inner = counted(memo,
                                      nest(depth - 1u));
        const outcome outer = counted(memo,
                                      nest(depth));
        const long    more  = static_cast<long>(outer.counts.memo_entries) -
                              static_cast<long>(inner.counts.memo_entries);

        // a level that cost otherwise, or that did not match
        if ( (!outer.ok)                          ||
             ((outer.steps - inner.steps) != step) ||
             (more != kept) )
        {
            return false;
        }
    }

    return ( (step > 0)                                  &&
             (step < 100)                                &&
             (kept > 0)                                  &&
             (alike(counted(nested,
                            nest(4u)),
                    counted(memo,
                            nest(4u)))) );
}

/*
tests_memo_choice
  The rules a profile says a memo pays for, and profiles added together.
  Tests the following:
  - from a profile of the nested grammar without the memo, Sum, Prod and
    Atom are chosen, and Expr, called once, and Num, whose repeats run no
    more steps than the memo would cost, are not
  - the factored grammar repeats no call, and nothing is chosen
  - the chosen rules memoized make the nested grammar linear as Sum and
    Prod do
  - two profiles added are their counts summed and their peaks the greater,
    the revisits by position as far as the longer input
*/
bool tests_memo_choice()
{
    const vp::peg::program nested   = program_of(NESTED);
    const vp::peg::layout  where    = vp::peg::layout_of(nested);
    const outcome          deep     = counted(nested,
                                              nest(3u));
    const std::vector<int> chosen   = vp::peg::worth_memoizing(deep.counts);
    const std::vector<int> expected = { where.starts[1],
                                        where.starts[2],
                                        where.starts[3] };
    const vp::peg::program factored = program_of(FACTORED);
    const outcome          flat     = counted(factored,
                                              "((1+2)*3)/4-5");

    // Sum, Prod and Atom, and nothing in the factored grammar
    if ( (chosen != expected)                                    ||
         (!vp::peg::worth_memoizing(flat.counts).empty()) )
    {
        return false;
    }

    const vp::peg::program memo  = vp::peg::memoized(nested,
                                                     chosen);
    const outcome          four  = counted(memo,
                                           nest(4u));
    const outcome          five  = counted(memo,
                                           nest(5u));
    const outcome          eight = counted(memo,
                                           nest(8u));
    const outcome          nine  = counted(memo,
                                           nest(9u));

    // linear, as with Sum and Prod
    if ( (five.steps - four.steps) != (nine.steps - eight.steps) )
    {
        return false;
    }

    const outcome    one   = counted(nested,
                                     nest(1u));
    const outcome    two   = counted(nested,
                                     "1+2*3");
    vp::peg::profile total;

    vp::peg::accumulate(total,
                        one.counts);
    vp::peg::accumulate(total,
                        two.counts);

    // rule by rule, the two runs' sums
    for (std::size_t index = 0u; index < total.rules.size(); index++)
    {
        const vp::peg::profile::DRuleCounts& sum = total.rules[index];
        const vp::peg::profile::DRuleCounts& a   = one.counts.rules[index];
        const vp::peg::profile::DRuleCounts& b   = two.counts.rules[index];

        // a count that is not the sum
        if ( (sum.start != a.start)                       ||
             (sum.steps != (a.steps + b.steps))           ||
             (sum.calls != (a.calls + b.calls))           ||
             (sum.repeats != (a.repeats + b.repeats))     ||
             (sum.revisits != (a.revisits + b.revisits)) )
        {
            return false;
        }
    }

    return ( (total.rules.size() == where.starts.size())               &&
             (total.steps == (one.counts.steps + two.counts.steps))    &&
             (total.revisits == (one.counts.revisits +
                                 two.counts.revisits))                 &&
             (total.peak_calls == std::max(one.counts.peak_calls,
                                           two.counts.peak_calls))     &&
             (total.revisits_at.size() == 6u)                          &&
             (total.revisits_at[0] == (one.counts.revisits_at[0] +
                                       two.counts.revisits_at[0]))     &&
             (total.revisits_at[5] == two.counts.revisits_at[5]) );
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
