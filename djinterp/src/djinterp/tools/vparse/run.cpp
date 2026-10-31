/*******************************************************************************
* vparse [vparse]                                                        run.cpp
*
* Executing a run and describing what it did.
*   Reading notation is done the way notation::parse_grammar does it --
* compile the meta-grammar, run it over the text under notation::read_limit,
* fold the captures -- but on a traced machine, so the grammar's own reading
* can be inspected like any other run. A request's step limit is for the run
* over its input, as a match's is. Two hazards are caught before the program
* runs rather than left to the machine: a reference to an undefined rule (the
* generator would resolve it to address 0 and loop until the step limit) and
* an empty program (which the driver would index past its end).
*
* path:      /src/djinterp/tools/vparse/run.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.27
*******************************************************************************/
#include "../../../../inc/djinterp/tools/vparse/run.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has
// std
#include <algorithm>   // std::sort, std::unique, std::reverse, std::min,
                       // std::max, std::upper_bound
#include <chrono>      // std::chrono::steady_clock
#include <cstddef>     // std::ptrdiff_t, std::size_t
#include <cstdio>      // std::snprintf
#include <cstdlib>     // std::atoi
#include <ctime>       // std::time, std::localtime, std::strftime
#include <functional>  // std::function
#include <map>         // std::map
#include <memory>      // std::make_shared
#include <set>         // std::set
#include <sstream>     // std::istringstream
#include <utility>     // std::make_pair, std::pair
// djinterp
#include "djinterp/cpp/modules/parsegen/vparse/ebnf.hpp"      // ebnf::parse, compile
#include "djinterp/cpp/modules/parsegen/vparse/machine.hpp"   // machine, op_set
#include "djinterp/cpp/modules/parsegen/vparse/notation.hpp"  // meta_grammar, reader,
                                                  // read_limit
#include "djinterp/cpp/modules/parsegen/vparse/session.hpp"   // session, frontend,
                                                  // compile_result
// vparse
#include "../../../../inc/djinterp/tools/vparse/heat.hpp"  // build_heat,
                                                           // heat_listing,
                                                           // heat_summary


namespace vparse
{
namespace workbench
{

namespace
{

namespace pr = ::djinterp::parse;

using steady = std::chrono::steady_clock;

// elapsed_ms
//   milliseconds since _from.
double elapsed_ms(
    steady::time_point _from
)
{
    return std::chrono::duration<double, std::milli>(steady::now() - _from)
        .count();
}

// now_text
//   the local time as "YYYY-MM-DD HH:MM:SS".
std::string now_text()
{
    const std::time_t now = std::time(nullptr);
    char              text[32];

    std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&now));

    return text;
}

// ms_text
//   a duration in milliseconds, to two decimals.
std::string ms_text(
    double _ms
)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%.2f ms", _ms);

    return text;
}

// plural
//   "1 step", "2 steps".
std::string plural(
    std::size_t        _count,
    const std::string& _noun
)
{
    return std::to_string(_count) + " " + _noun + ((_count == 1) ? "" : "s");
}

// ---------------------------------------------------------------- rendering

// compress_set
//   a character set written compactly: sorted, with runs of three or more
// consecutive characters as ranges, e.g. "0123456789" -> "0-9".
std::string compress_set(
    const std::string& _set
)
{
    std::string chars = _set;
    std::sort(chars.begin(), chars.end());
    chars.erase(std::unique(chars.begin(), chars.end()), chars.end());

    std::string out;

    for (std::size_t i = 0; i < chars.size(); )
    {
        std::size_t j = i;

        // extend the run while characters stay consecutive
        while ( ((j + 1) < chars.size()) &&
                (chars[j + 1] == static_cast<char>(chars[j] + 1)) )
        {
            ++j;
        }

        const std::string first = escape(std::string(1, chars[i]));

        if ((j - i) >= 2)
        {
            out += first + "-" + escape(std::string(1, chars[j]));
        }
        else
        {
            for (std::size_t k = i; k <= j; ++k)
            {
                out += escape(std::string(1, chars[k]));
            }
        }

        i = j + 1;
    }

    return out;
}

// render_term
//   one ruleset term in notation style: 'c', [0-9], Ref, . -- with * for a
// starred term and #N for a capture tag.
std::string render_term(
    const vp::term& _term
)
{
    std::string out;

    switch (_term.kind)
    {
        case vp::T_CHAR:
            out = "'" + escape(std::string(1, _term.ch)) + "'";
            break;
        case vp::T_SET:
            out = "[" + compress_set(_term.set) + "]";
            break;
        case vp::T_REF:
            out = _term.ref;
            break;
        default:
            out = ".";
            break;
    }

    if (_term.star)
    {
        out += "*";
    }

    if (_term.cap != 0)
    {
        out += "#" + std::to_string(_term.cap);
    }

    return out;
}

// render_ebnf
//   one EBNF node as an expression; _nested wraps a sequence or choice.
std::string render_ebnf(
    const vp::ebnf::node_ptr& _node,
    bool                      _nested
)
{
    if (!_node)
    {
        return "?";
    }

    const auto join = [](const vp::ebnf::node_ptr& _parent,
                         const std::string&        _separator)
    {
        std::string out;

        for (std::size_t i = 0; i < _parent->kids.size(); ++i)
        {
            if (i != 0)
            {
                out += _separator;
            }

            out += render_ebnf(_parent->kids[i], true);
        }

        return out;
    };

    const auto first = [&_node]()
    {
        return _node->kids.empty() ? std::string("?")
                                   : render_ebnf(_node->kids[0], true);
    };

    switch (_node->kind)
    {
        case vp::ebnf::N_LIT:  return "'" + escape(_node->text) + "'";
        case vp::ebnf::N_ANY:  return ".";
        case vp::ebnf::N_SET:  return "[" + compress_set(_node->text) + "]";
        case vp::ebnf::N_REF:  return _node->text;
        case vp::ebnf::N_OPT:  return first() + "?";
        case vp::ebnf::N_STAR: return first() + "*";
        case vp::ebnf::N_PLUS: return first() + "+";
        case vp::ebnf::N_NOT:  return "!" + first();
        case vp::ebnf::N_SEQ:
        {
            const std::string body = join(_node, " ");

            return ( (_nested) &&
                     (_node->kids.size() > 1) ) ? "(" + body + ")"
                                                : body;
        }
        case vp::ebnf::N_CHOICE:
        {
            const std::string body = join(_node, " | ");

            return ( (_nested) &&
                     (_node->kids.size() > 1) ) ? "(" + body + ")"
                                                : body;
        }
        default:
            return "?";
    }
}

// dump_ebnf
//   the EBNF syntax tree, one node per line, indented by depth.
void dump_ebnf(
    const vp::ebnf::node_ptr& _node,
    int                       _depth,
    std::vector<std::string>& _out
)
{
    static const char* const kinds[] = { "literal", "any", "set", "ref",
                                         "sequence", "choice", "optional",
                                         "star", "plus", "not" };

    if (!_node)
    {
        return;
    }

    std::string line(static_cast<std::size_t>(_depth) * 2, ' ');
    const int   kind = _node->kind;

    line += ( (kind >= 0) &&
              (kind <= vp::ebnf::N_NOT) ) ? kinds[kind] : "node";

    if (!_node->text.empty())
    {
        line += "  " + escape(_node->text);
    }

    _out.push_back(line);

    for (const vp::ebnf::node_ptr& kid : _node->kids)
    {
        dump_ebnf(kid, _depth + 1, _out);
    }

    return;
}

// ---------------------------------------------------------------- programs

// preamble_end
//   the first address after the end-of-input gate: one past the first MATCH.
std::size_t preamble_end(
    const vp::peg::program& _program
)
{
    std::size_t at = 0;

    while ( (at < _program.size()) &&
            (_program[at].op != vp::peg::MATCH) )
    {
        ++at;
    }

    return at + 1;
}

// label_entries
//   the rule name at each rule's first address. Both front ends lay rules out
// after the gate in rule order, each ending in RETURN, which is how
// peg::layout_of reads a program; the labels are used only where every call
// lands on a rule's start and there is a start for every name, and calls'
// targets are named by address otherwise.
std::vector<std::string> label_entries(
    const vp::peg::program&         _program,
    const std::vector<std::string>& _names
)
{
    std::vector<std::string> labels(_program.size());
    const vp::peg::layout    where = vp::peg::layout_of(_program);

    // the layout agrees with the rules: a name at each start
    if ( (where.regular) &&
         (where.starts.size() == _names.size()) )
    {
        // each rule's start, by its name
        for (std::size_t index = 0u; index < where.starts.size(); index++)
        {
            labels[static_cast<std::size_t>(where.starts[index])] =
                _names[index];
        }

        return labels;
    }

    // each call's target, by its address
    for (const vp::peg::DInstr& ins : _program)
    {
        const bool calls = ( (ins.op == vp::peg::CALL) ||
                             (ins.op == vp::peg::MEMO) );

        // a call into the program
        if ( (calls) &&
             (ins.arg >= 0) &&
             (static_cast<std::size_t>(ins.arg) < labels.size()) )
        {
            labels[static_cast<std::size_t>(ins.arg)] =
                "@" + std::to_string(ins.arg);
        }
    }

    return labels;
}

// entry_name
//   the rule name for a CALL target.
std::string entry_name(
    const std::vector<std::string>& _entry,
    int                             _target
)
{
    if ( (_target >= 0) &&
         (static_cast<std::size_t>(_target) < _entry.size()) &&
         (!_entry[static_cast<std::size_t>(_target)].empty()) )
    {
        return _entry[static_cast<std::size_t>(_target)];
    }

    return "@" + std::to_string(_target);
}

// parse_trace_line
//   "ip=3 off=0 calls=1 back=0 marks=0 caps=0 | SET [...]" -> a trace_step.
trace_step parse_trace_line(
    const std::string& _line
)
{
    trace_step         step;
    std::istringstream fields(_line.substr(0, _line.find(" | ")));
    std::string        field;

    while (fields >> field)
    {
        const std::size_t eq = field.find('=');

        // a field without a value is not one of ours; skip it
        if (eq == std::string::npos)
        {
            continue;
        }

        const std::string key   = field.substr(0, eq);
        const int         value = std::atoi(field.c_str() + eq + 1);

        if      (key == "ip")    { step.ip    = value; }
        else if (key == "off")   { step.off   = value; }
        else if (key == "calls") { step.calls = value; }
        else if (key == "back")  { step.back  = value; }
        else if (key == "marks") { step.marks = value; }
        else if (key == "caps")  { step.caps  = value; }
    }

    return step;
}

// expectation
//   what a failing test was looking for.
std::string expectation(
    const vp::peg::DInstr& _ins,
    bool                   _in_gate
)
{
    switch (_ins.op)
    {
        case vp::peg::CHAR:
            // a quote reads better in double quotes than as three quotes
            return (_ins.ch == '\'')
                       ? std::string("\"'\"")
                       : "'" + escape(std::string(1, _ins.ch)) + "'";
        case vp::peg::SET:
        {
            const std::string members = compress_set(_ins.set);

            // the one set every grammar has reads better as a word
            return (members == "\\t\\n\\r ") ? std::string("whitespace")
                                               : "[" + members + "]";
        }
        case vp::peg::ANY:  return "any character";
        default:            break;
    }

    return _in_gate ? "the end of the input" : "something else";
}

// analyse
//   replays a trace to recover the rules that matched and, for a failure, the
// frontier. CALL opens a frame and RETURN closes it into a span; CHOICE
// remembers how many spans existed; a failed test that backtracks discards the
// spans closed since that CHOICE and the frames above the restored depth.
// A test failed when the next line has one backtrack point fewer, or when it
// is the last line of a run that did not match.
//   A MEMO is a CALL where the next line is one call deeper. Where it is not,
// the memo answered it: a failure is a failed test like any other, and a
// success is the spans the rule closed the first time it matched there,
// replayed at this depth, since the rule did not run again to close them.
void analyse(
    parse_pass&                     _pass,
    const vp::peg::program&         _program,
    const std::vector<std::string>& _entry
)
{
    struct frame
    {
        int         target;
        int         start;
        std::size_t first;
        bool        memo;
    };

    std::vector<frame>                                frames;
    std::vector<std::size_t>                          checkpoints;
    std::vector<span>                                 closed;
    std::map<std::pair<int, int>, std::vector<span>>  remembered;
    const std::size_t                                 gate =
        preamble_end(_program);

    const auto stack_names = [&frames, &_entry]()
    {
        std::vector<std::string> names;

        for (const frame& f : frames)
        {
            names.push_back(entry_name(_entry, f.target));
        }

        return names;
    };

    for (std::size_t k = 0; k < _pass.trace.size(); ++k)
    {
        const trace_step& step = _pass.trace[k];

        // a step outside the program cannot be interpreted
        if ( (step.ip < 0) ||
             (static_cast<std::size_t>(step.ip) >= _program.size()) )
        {
            break;
        }

        const vp::peg::DInstr& ins      =
            _program[static_cast<std::size_t>(step.ip)];
        const bool            last     = ((k + 1) == _pass.trace.size());
        const bool            entered  =
            ( (ins.op == vp::peg::MEMO) &&
              (!last) &&
              (_pass.trace[k + 1].calls == (step.calls + 1)) );
        const bool            answered = ( (ins.op == vp::peg::MEMO) &&
                                           (!entered) );
        const bool            test     = ( (ins.op == vp::peg::CHAR) ||
                                           (ins.op == vp::peg::ANY)  ||
                                           (ins.op == vp::peg::SET)  ||
                                           (ins.op == vp::peg::FAIL) ||
                                           (answered) );
        const bool            failed =
            ( (test) &&
              ( ( (!last) &&
                  (_pass.trace[k + 1].back == (step.back - 1)) ) ||
                ( (last) &&
                  (!_pass.matched) ) ) );

        // the gate's ANY consumes a character before its COMMIT and FAIL
        // run, so those two say nothing about where parsing stopped
        const bool in_gate   = (static_cast<std::size_t>(step.ip) < gate);
        const bool gate_tail = ( (in_gate) &&
                                 ( (ins.op == vp::peg::COMMIT) ||
                                   (ins.op == vp::peg::FAIL) ) );

        // the frontier moves only forward
        if ( (!gate_tail) &&
             (step.off > _pass.far.offset) )
        {
            _pass.far.offset = step.off;
            _pass.far.stack  = stack_names();
            _pass.far.expected.clear();
        }

        const auto expect = [&_pass](const std::string& _want)
        {
            if (std::find(_pass.far.expected.begin(),
                          _pass.far.expected.end(),
                          _want) == _pass.far.expected.end())
            {
                _pass.far.expected.push_back(_want);
            }

            return;
        };

        // a rule's failure at the frontier says what it wanted there; the
        // memo's answer says it again, and was heard the first time
        if ( (failed) &&
             (!in_gate) &&
             (!answered) &&
             (step.off == _pass.far.offset) )
        {
            _pass.far.stack = stack_names();
            _pass.far.step  = static_cast<int>(k);
            expect(expectation(ins, false));
        }

        // the gate's ANY succeeding means input remained where the match
        // ended: the end of the input was expected there
        if ( (in_gate) &&
             (ins.op == vp::peg::ANY) &&
             (!failed) &&
             (step.off == _pass.far.offset) )
        {
            if (_pass.far.expected.empty())
            {
                _pass.far.stack.clear();
                _pass.far.step = static_cast<int>(k);
            }

            expect("the end of the input");
        }

        // the analysis's own reading of the instruction -- not a dispatch
        if ( (ins.op == vp::peg::CALL) ||
             (entered) )
        {
            frames.push_back(frame{ ins.arg,
                                    step.off,
                                    closed.size(),
                                    entered });
        }
        else if ( (ins.op == vp::peg::RETURN) &&
                  (!frames.empty()) )
        {
            const frame done = frames.back();
            frames.pop_back();
            closed.push_back(span{ entry_name(_entry, done.target),
                                   done.start,
                                   step.off,
                                   static_cast<int>(frames.size()) });

            // the spans a MEMO's match closed, the rule's own last, for the
            // MEMOs the memo answers later; a CALL's are never asked for
            if (done.memo)
            {
                remembered[std::make_pair(done.target,
                                          done.start)] =
                    std::vector<span>(closed.begin() +
                                          static_cast<std::ptrdiff_t>(
                                              std::min(done.first,
                                                       closed.size())),
                                      closed.end());
            }
        }
        else if ( (answered) &&
                  (!failed) )
        {
            const auto found =
                remembered.find(std::make_pair(ins.arg,
                                               step.off));
            const int  depth = static_cast<int>(frames.size());

            // the first match's spans, moved to this depth
            if ( (found != remembered.end()) &&
                 (!found->second.empty()) )
            {
                const int shift = depth - found->second.back().depth;

                // each, in the order it closed
                for (span each : found->second)
                {
                    each.depth += shift;
                    closed.push_back(each);
                }
            }
            else
            {
                closed.push_back(span{ entry_name(_entry,
                                                  ins.arg),
                                       step.off,
                                       last ? step.off
                                            : _pass.trace[k + 1].off,
                                       depth });
            }
        }
        else if (ins.op == vp::peg::CHOICE)
        {
            checkpoints.push_back(closed.size());
        }
        else if ( (ins.op == vp::peg::COMMIT) &&
                  (!checkpoints.empty()) )
        {
            checkpoints.pop_back();
        }

        // a backtrack unwinds spans and frames to the CHOICE it resumes
        if ( (failed) &&
             (!last) )
        {
            const std::size_t keep = checkpoints.empty() ? 0
                                                         : checkpoints.back();

            if (!checkpoints.empty())
            {
                checkpoints.pop_back();
            }

            closed.resize(std::min(closed.size(), keep));
            frames.resize(std::min(frames.size(),
                                   static_cast<std::size_t>(
                                       _pass.trace[k + 1].calls)));
        }
    }

    // spans closed in post-order become a tree, emitted parents first. After
    // a failure these are what had matched, and not been undone, when the
    // machine stopped.
    struct node
    {
        std::size_t              at;
        std::vector<std::size_t> kids;
    };

    std::vector<node>        nodes;
    std::vector<std::size_t> pending;

    for (std::size_t i = 0; i < closed.size(); ++i)
    {
        node here{ i, {} };

        // every pending node deeper than this one is one of its children
        while ( (!pending.empty()) &&
                (closed[nodes[pending.back()].at].depth > closed[i].depth) )
        {
            here.kids.push_back(pending.back());
            pending.pop_back();
        }

        std::reverse(here.kids.begin(), here.kids.end());
        nodes.push_back(here);
        pending.push_back(nodes.size() - 1);
    }

    std::function<void(std::size_t)> walk = [&](std::size_t _node)
    {
        _pass.matches.push_back(closed[nodes[_node].at]);

        for (std::size_t kid : nodes[_node].kids)
        {
            walk(kid);
        }

        return;
    };

    for (std::size_t root : pending)
    {
        walk(root);
    }

    return;
}

// run_pass
//   one traced PEG run of _program over _text.
parse_pass run_pass(
    const vp::peg::program&         _program,
    const std::vector<std::string>& _entry,
    const std::string&              _text,
    const run_request&              _request
)
{
    parse_pass pass;

    // an empty program would be indexed past its end by the driver
    if (_program.empty())
    {
        pass.error = "the program is empty";

        return pass;
    }

    static const vp::op_set ops = vp::peg::make_ops();

    pr::parse_state<char>    state(_text.data(), _text.size(), 0);
    std::vector<std::string> lines;
    vp::machine              machine;

    machine.step_limit = _request.step_limit;

    if (_request.trace)
    {
        machine.trace = &lines;
    }

    const steady::time_point start = steady::now();

    vp::peg::sinks sinks;

    sinks.captures = &pass.captures;
    sinks.counts   = &pass.counts;

    pass.matched  = vp::peg::run(machine,
                                 state,
                                 _program,
                                 ops,
                                 sinks);
    pass.ms       = elapsed_ms(start);
    pass.ran      = true;
    pass.steps    = machine.steps;
    pass.error    = machine.error;
    pass.consumed = pass.matched ? state.offset : 0;

    pass.trace.reserve(lines.size());

    for (const std::string& line : lines)
    {
        pass.trace.push_back(parse_trace_line(line));
    }

    analyse(pass, _program, _entry);

    return pass;
}

// frontier_place
//   "line 2, column 7 (character 18)": where a parse got furthest.
std::string frontier_place(
    const frontier&    _far,
    const std::string& _text
)
{
    const std::pair<int, int> at = line_col(_text, _far.offset);

    return "line " + std::to_string(at.first) + ", column " +
           std::to_string(at.second) + " (character " +
           std::to_string(_far.offset) + ")";
}

// frontier_rest
//   ", inside Rule > Alt, expecting ';' or '|'": what was being read there,
// and what would have let it go on; "" when neither is known.
std::string frontier_rest(
    const frontier& _far
)
{
    std::string out;

    if (!_far.stack.empty())
    {
        out += ", inside ";

        for (std::size_t i = 0; i < _far.stack.size(); ++i)
        {
            out += ((i == 0) ? "" : " > ") + _far.stack[i];
        }
    }

    if (!_far.expected.empty())
    {
        out += ", expecting ";

        for (std::size_t i = 0; i < _far.expected.size(); ++i)
        {
            if (i != 0)
            {
                out += ((i + 1) == _far.expected.size()) ? " or " : ", ";
            }

            out += _far.expected[i];
        }
    }

    return out;
}

// frontier_text
//   "line 2, column 7 (character 18), inside Rule > Alt, expecting ';' or '|'"
std::string frontier_text(
    const frontier&    _far,
    const std::string& _text
)
{
    if (_far.offset < 0)
    {
        return "before reading anything";
    }

    return frontier_place(_far, _text) + frontier_rest(_far);
}

// ---------------------------------------------------------------- stages

// read_notation
//   the notation front end, traced: meta-grammar program over the text, then
// the capture fold. Undefined references are reported here, before the
// generator can turn them into a CALL to address 0. A reading that stops is
// traced even when the request is not: where it stopped is known only from
// the trace, and the machine is deterministic, so a second reading, traced,
// is the first one watched.
void read_notation(
    run_record& _run
)
{
    static const vp::ruleset      meta   = vp::notation::meta_grammar();
    static const vp::peg::program reader = vp::gen::compile(meta);

    std::vector<std::string> meta_names;

    for (const vp::rule& r : meta.rules)
    {
        meta_names.push_back(r.name);
    }

    // the grammar is read under the reader's own limit, as parse_grammar
    // reads it: the request's limit is for the run over the input
    run_request reading = _run.request;

    reading.step_limit = vp::notation::read_limit(_run.request.grammar.size());

    _run.reader       = reader;
    _run.reader_entry = label_entries(reader, meta_names);
    _run.reading      = run_pass(reader, _run.reader_entry,
                                 _run.request.grammar, reading);

    // a reading that stopped untraced is read again, traced, to say where
    if ( (!_run.reading.matched) &&
         (!reading.trace) )
    {
        reading.trace = true;
        _run.reading  = run_pass(reader, _run.reader_entry,
                                 _run.request.grammar, reading);
    }

    if (!_run.reading.matched)
    {
        _run.read_error =
            (_run.reading.error == "step limit")
                ? "the reader hit the step limit"
                : "the reader stopped at " +
                      frontier_text(_run.reading.far, _run.request.grammar);

        return;
    }

    _run.rules = vp::notation::read_ruleset(_run.reading.captures,
                                            _run.request.grammar);

    for (const vp::rule& r : _run.rules.rules)
    {
        _run.rule_names.push_back(r.name);
    }

    _run.read_ok = true;

    return;
}

// build_notation
//   ruleset -> plan -> program.
void build_notation(
    run_record& _run
)
{
    static const vp::op_set gen_ops = vp::gen::make_ops();

    _run.plan    = vp::gen::plan(_run.rules);
    _run.program = vp::gen::generate(_run.plan, gen_ops);
    _run.built   = true;

    return;
}

// read_and_build_ebnf
//   the EBNF front end: parse, check references, compile.
void read_ebnf(
    run_record& _run
)
{
    bool        ok = false;
    std::string error;

    const std::pair<vp::ebnf::rules, std::string> parsed =
        vp::ebnf::parse(_run.request.grammar, &ok, &error);

    if (!ok)
    {
        _run.read_error = error.empty() ? "the EBNF did not parse" : error;

        return;
    }

    for (const auto& r : parsed.first)
    {
        _run.rule_names.push_back(r.first);
        _run.ebnf_rules.push_back(r.first + " = " +
                                  render_ebnf(r.second, false) + " ;");
        _run.ebnf_tree.push_back(r.first);
        dump_ebnf(r.second, 1, _run.ebnf_tree);
    }

    _run.read_ok = true;

    return;
}

/*
build_ebnf
  The frontend parses the text again: ebnf::rules is not kept between the two
steps, and an EBNF grammar is small enough that keeping it would buy nothing.
*/
void build_ebnf(
    run_record& _run
)
{
    bool        ok = false;
    std::string error;

    const std::pair<vp::ebnf::rules, std::string> parsed =
        vp::ebnf::parse(_run.request.grammar, &ok, &error);

    _run.program = vp::ebnf::compile(parsed.first, parsed.second, &ok, &error);

    if (!ok)
    {
        _run.build_error = error.empty() ? "the EBNF did not compile" : error;
        _run.program.clear();

        return;
    }

    _run.built = true;

    return;
}

// ---------------------------------------------------------------- listings

/**
 * @brief Says what the memo did, for the summary.
 *
 * @param[in] _run  the run.
 * @return "none" for a run without it; the setting, for a run whose program
 *         was not built; otherwise the rules it answered for, the results it
 *         kept and the calls it answered, and, where "auto" chose the
 *         rules, the steps the run without it took.
 */
std::string memo_text(
    const run_record& _run
)
{
    const std::string&      setting = _run.request.memoize;
    const vp::peg::profile& counts  = _run.recognition.counts;
    const bool              chosen  = (setting == "auto");

    // no memo was asked for
    if ( (setting.empty()) ||
         (setting == "none") )
    {
        return "none";
    }

    // asked for, and the program never built to use it
    if (!_run.built)
    {
        return "asked for " + setting + ", and not used";
    }

    // asked for, and not used: no rule asked for, or none worth it
    if (_run.memoized.empty())
    {
        return chosen ? "auto: no rule repeats enough work to pay for it"
                      : "none";
    }

    std::string names;

    // the rules, in program order
    for (const std::string& name : _run.memoized)
    {
        names += (names.empty() ? "" : ", ") + name;
    }

    std::string out = (chosen ? "auto: " : "") + names;

    // what it did, where the program ran
    if (_run.recognition.ran)
    {
        const std::string kept     = plural(counts.memo_entries,
                                            "result");
        const std::string answered = plural(counts.memo_hits,
                                            "call");

        out += "; " + kept + " kept, " + answered + " answered";
    }

    // what it saved, where a run without it was counted
    if (_run.unmemoized_steps >= 0)
    {
        out += "; " + plural(static_cast<std::size_t>(_run.unmemoized_steps),
                             "step") +
               " without it";
    }

    return out;
}

/**
 * @brief Says how deep the recognition pass's stacks went, for the summary.
 *
 * @param[in] _pass  the pass.
 * @return the deepest each stack was; "-" for a pass that did not run.
 */
std::string stacks_text(
    const parse_pass& _pass
)
{
    // nothing ran
    if (!_pass.ran)
    {
        return "-";
    }

    const std::string calls = plural(_pass.counts.peak_calls,
                                     "call");
    const std::string back  = plural(_pass.counts.peak_back,
                                     "backtrack point");
    const std::string marks = plural(_pass.counts.peak_marks,
                                     "mark");
    const std::string caps  = plural(_pass.counts.peak_caps,
                                     "capture");

    return "at most " + calls + ", " + back + ", " + marks + ", " + caps;
}

listing summary_listing(
    const run_record& _run
)
{
    listing out;
    out.columns = { "", "" };

    const run_request& req   = _run.request;
    const bool         ebnf  = (req.frontend == "ebnf");
    const auto         lines = [](const std::string& _text)
    {
        return static_cast<std::size_t>(
                   std::count(_text.begin(), _text.end(), '\n')) +
               ((_text.empty() || (_text.back() == '\n')) ? 0 : 1);
    };

    std::vector<std::pair<std::string, std::string>> rows;

    rows.push_back({ "result", headline(_run) });
    rows.push_back({ "front end", ebnf ? "EBNF" : "notation" });
    rows.push_back({ "grammar", plural(lines(req.grammar), "line") + ", " +
                                plural(req.grammar.size(), "character") });
    rows.push_back({ "reading",
                     !_run.read_ok
                         ? "failed: " + _run.read_error
                         : (ebnf ? std::string("parsed")
                                 : "read in " +
                                       plural(static_cast<std::size_t>(
                                                  _run.reading.steps),
                                              "step")) });

    std::string names;

    for (std::size_t i = 0; i < _run.rule_names.size(); ++i)
    {
        names += ((i == 0) ? "" : ", ") + _run.rule_names[i];
    }

    rows.push_back({ "rules", std::to_string(_run.rule_names.size()) +
                              (names.empty() ? "" : ": " + names) });
    rows.push_back({ "plan", ebnf ? "none; EBNF compiles directly"
                                  : plural(_run.plan.size(), "directive") });
    rows.push_back({ "program",
                     _run.built ? plural(_run.program.size(), "instruction")
                                : "not built: " + _run.build_error });
    rows.push_back({ "input", plural(lines(req.input), "line") + ", " +
                              plural(req.input.size(), "character") });

    const parse_pass& rec = _run.recognition;

    rows.push_back({ "steps", rec.ran ? std::to_string(rec.steps) : "-" });
    rows.push_back({ "rules matched",
                     std::to_string(rec.matches.size()) });
    rows.push_back({ "captures", std::to_string(rec.captures.size()) });
    rows.push_back({ "revisits", _run.heat ? heat_summary(*_run.heat)
                                           : std::string("-") });
    rows.push_back({ "memo", memo_text(_run) });
    rows.push_back({ "stacks", stacks_text(rec) });
    rows.push_back({ "trace", req.trace ? "recorded" : "not recorded" });
    rows.push_back({ "step limit", std::to_string(req.step_limit) });
    rows.push_back({ "time", "read " + ms_text(_run.read_ms) +
                             ", build " + ms_text(_run.build_ms) +
                             ", run " + ms_text(rec.ms) });
    rows.push_back({ "origin", req.origin.empty() ? "typed in" : req.origin });
    rows.push_back({ "when", _run.when });

    for (const auto& row : rows)
    {
        out.rows.push_back({ row.first, row.second });
        out.keys.push_back(row.first + ": " + row.second);
        out.marks.push_back(0);
    }

    return out;
}

listing text_listing(
    const std::string& _text,
    const std::string& _empty
)
{
    listing out;
    out.columns = { "line", "text" };
    out.note    = _empty;

    std::size_t start = 0;
    int         line  = 1;

    while (start < _text.size())
    {
        std::size_t end = _text.find('\n', start);

        if (end == std::string::npos)
        {
            end = _text.size();
        }

        const std::string body = escape(_text.substr(start, end - start));

        out.rows.push_back({ std::to_string(line), body });
        out.keys.push_back(body);
        out.marks.push_back(0);

        start = end + 1;
        ++line;
    }

    return out;
}

/**
 * @brief Finds where each line of a text starts.
 *
 * @param[in] _text  the text.
 * @return the offset of each line's first byte, in order: 0, and one past
 *         every newline.
 */
std::vector<int> line_starts(
    const std::string& _text
)
{
    std::vector<int> out(1u,
                         0);

    // every byte of the text, for its newlines
    for (std::size_t at = 0u; at < _text.size(); ++at)
    {
        // a line starts after every newline
        if (_text[at] == '\n')
        {
            out.push_back(static_cast<int>(at + 1u));
        }
    }

    return out;
}

/**
 * @brief Gives the line and column of an offset, as line_col() counts them,
 *        from its text's line starts.
 *
 * @param[in] _starts  the text's line starts, from line_starts().
 * @param[in] _offset  the offset, in bytes; at most the text's length.
 * @return the 1-based line and column; line 1, column 1 for an offset
 *         before the text.
 *
 * A binary search, where line_col() counts from the text's first byte: the
 * reading's listing places every capture of the grammar, and a long grammar
 * has tens of thousands, so counting from the start each time was quadratic
 * in the grammar.
 */
std::pair<int, int> line_col_at(
    const std::vector<int>& _starts,
    int                     _offset
)
{
    const int  offset = std::max(_offset,
                                 0);
    const auto after  = std::upper_bound(_starts.begin(),
                                         _starts.end(),
                                         offset);
    const int  line   = static_cast<int>(after - _starts.begin());

    return { line,
             (offset - _starts[static_cast<std::size_t>(line - 1)]) + 1 };
}

listing reading_listing(
    const run_record& _run
)
{
    listing out;

    if (_run.request.frontend == "ebnf")
    {
        out.columns = { "syntax tree" };
        out.note    = _run.read_ok ? "" : "The EBNF did not parse: " +
                                          _run.read_error + ".";

        for (const std::string& line : _run.ebnf_tree)
        {
            out.rows.push_back({ line });
            out.keys.push_back(line);
            out.marks.push_back((line.find(' ') == 0) ? 0 : 3);
        }

        return out;
    }

    static const char* const tags[] = { "", "NAME", "SETBODY", "REF", "STAR",
                                        "PLUS", "LITCH", "BAR" };

    out.columns = { "#", "tag", "at", "text" };
    out.note    = _run.read_ok ? "The grammar is empty."
                               : "Nothing was read: " + _run.read_error + ".";

    const std::string&     text   = _run.request.grammar;
    const std::vector<int> starts = line_starts(text);

    for (std::size_t i = 0; i < _run.reading.captures.size(); ++i)
    {
        const vp::peg::capture& c   = _run.reading.captures[i];
        const std::string       tag = ( (c.tag >= 1) &&
                                        (c.tag <= 7) ) ? tags[c.tag]
                                                       : std::to_string(c.tag);
        const std::pair<int, int> at = line_col_at(starts,
                                                   c.start);
        const std::string body = escape(text.substr(
            static_cast<std::size_t>(c.start),
            static_cast<std::size_t>(c.end - c.start)));

        out.rows.push_back({ std::to_string(i), tag,
                             std::to_string(at.first) + ":" +
                                 std::to_string(at.second),
                             body });
        out.keys.push_back(tag + " " + body);
        out.marks.push_back((c.tag == 1) ? 3 : 0);
    }

    return out;
}

listing rules_listing(
    const run_record& _run
)
{
    listing out;
    out.columns = { "rule", "definition" };
    out.note    = _run.read_ok ? "The grammar has no rules."
                               : "No rules: the grammar was not read.";

    if (_run.request.frontend == "ebnf")
    {
        for (std::size_t i = 0; i < _run.ebnf_rules.size(); ++i)
        {
            const std::string& line = _run.ebnf_rules[i];
            const std::size_t  eq   = line.find(" = ");

            out.rows.push_back({ line.substr(0, eq), line.substr(eq + 3) });
            out.keys.push_back(line);
            out.marks.push_back(0);
        }

        return out;
    }

    for (const vp::rule& r : _run.rules.rules)
    {
        for (std::size_t a = 0; a < r.alts.size(); ++a)
        {
            std::string body;

            for (const vp::term& t : r.alts[a])
            {
                body += (body.empty() ? "" : " ") + render_term(t);
            }

            if (a + 1 == r.alts.size())
            {
                body += " ;";
            }

            const std::string head = (a == 0) ? r.name + " =" : "    |";

            out.rows.push_back({ head, body.empty() ? "(empty)" : body });
            out.keys.push_back(r.name + " " + std::to_string(a) + " " + body);
            out.marks.push_back((a == 0) ? 3 : 0);
        }
    }

    return out;
}

listing plan_listing(
    const run_record& _run
)
{
    listing out;
    out.columns = { "#", "directive", "operand" };
    out.note    = (_run.request.frontend == "ebnf")
                      ? "EBNF compiles straight to a program, without a plan."
                      : "No plan: the grammar was not read.";

    static const vp::op_set gen_ops = vp::gen::make_ops();

    for (std::size_t i = 0; i < _run.plan.size(); ++i)
    {
        const vp::gen::DGenInstr& g = _run.plan[i];
        std::string               operand;

        if (!g.text.empty())
        {
            operand = escape(g.text);
        }
        else if (g.ch != 0)
        {
            operand = "'" + escape(std::string(1, g.ch)) + "'";
        }

        if (g.tag != 0)
        {
            operand += (operand.empty() ? "" : " ") + std::string("#") +
                       std::to_string(g.tag);
        }

        const std::string name = gen_ops.name(g.op);

        out.rows.push_back({ std::to_string(i), name, operand });
        out.keys.push_back(name + " " + operand);
        out.marks.push_back((g.op == vp::gen::G_RULE_BEGIN) ? 3 : 0);
    }

    return out;
}

listing program_listing(
    const vp::peg::program&         _program,
    const std::vector<std::string>& _entry,
    const std::string&              _empty
)
{
    listing out;
    out.columns = { "addr", "instruction", "rule" };
    out.note    = _empty;

    // the rule each address belongs to, for keys that survive a shift
    std::vector<std::string> owner(_program.size(), "gate");
    std::vector<int>         base(_program.size(), 0);
    std::string              current = "gate";
    int                      start   = 0;

    for (std::size_t i = 0; i < _program.size(); ++i)
    {
        if ( (i < _entry.size()) &&
             (!_entry[i].empty()) )
        {
            current = _entry[i];
            start   = static_cast<int>(i);
        }

        owner[i] = current;
        base[i]  = start;
    }

    const auto where = [&](int _target)
    {
        if ( (_target < 0) ||
             (static_cast<std::size_t>(_target) >= _program.size()) )
        {
            return std::to_string(_target);
        }

        const std::size_t at = static_cast<std::size_t>(_target);

        return owner[at] + "+" + std::to_string(_target - base[at]);
    };

    for (std::size_t i = 0; i < _program.size(); ++i)
    {
        const vp::peg::DInstr& ins   = _program[i];
        const std::string     text  = disassemble(ins);
        const std::string     label = (i < _entry.size()) ? _entry[i] : "";

        // a comparison should see a new alternative, not every address after
        // it: calls are keyed by rule, branches by rule and offset
        std::string key = label + " " + text;

        // a call, by the rule it calls
        if ( (ins.op == vp::peg::CALL) ||
             (ins.op == vp::peg::MEMO) )
        {
            key = label + " " + text.substr(0,
                                            text.find(' ')) + " " +
                  entry_name(_entry,
                             ins.arg);
        }
        else if ( (ins.op == vp::peg::CHOICE) ||
                  (ins.op == vp::peg::COMMIT) ||
                  (ins.op == vp::peg::JUMP) )
        {
            key = label + " " + text.substr(0, text.find(' ')) + " " +
                  where(ins.arg);
        }

        out.rows.push_back({ std::to_string(i), text, label });
        out.keys.push_back(key);
        out.marks.push_back(label.empty() ? 0 : 3);
    }

    return out;
}

listing matches_listing(
    const run_record& _run
)
{
    listing out;
    out.columns = { "rule", "at", "text" };

    const parse_pass&  rec  = _run.recognition;
    const std::string& text = _run.request.input;

    // without a trace there is nothing to say about matches either way
    if (!rec.ran)
    {
        out.note = "The program did not run.";
    }
    else if (!_run.request.trace)
    {
        out.note = "Matches come from the trace, which was not recorded "
                   "for this run.";
    }
    else if (!rec.matched)
    {
        out.note = "Nothing matched. The parser got as far as " +
                   frontier_text(rec.far, text) + ".";

        // what did match is shown, marked as not the whole story
        if (!rec.matches.empty())
        {
            out.columns = { "matched before the parser stopped", "at",
                            "text" };
        }
    }

    for (const span& s : rec.matches)
    {
        const std::string rule =
            std::string(static_cast<std::size_t>(s.depth) * 2, ' ') + s.rule;
        const std::string body = escape(text.substr(
            static_cast<std::size_t>(s.start),
            static_cast<std::size_t>(s.end - s.start)));

        out.rows.push_back({ rule,
                             std::to_string(s.start) + "-" +
                                 std::to_string(s.end),
                             body });
        out.keys.push_back(rule + " " + body);
        out.marks.push_back(0);
    }

    for (std::size_t i = 0; i < rec.captures.size(); ++i)
    {
        const vp::peg::capture& c    = rec.captures[i];
        const std::string       body = escape(text.substr(
            static_cast<std::size_t>(c.start),
            static_cast<std::size_t>(c.end - c.start)));

        out.rows.push_back({ "capture #" + std::to_string(c.tag),
                             std::to_string(c.start) + "-" +
                                 std::to_string(c.end),
                             body });
        out.keys.push_back("capture " + std::to_string(c.tag) + " " + body);
        out.marks.push_back(1);
    }

    return out;
}

/**
 * @brief Trims the spaces and tabs from both ends of a text.
 *
 * @param[in] _text  the text.
 * @return what lies between them; empty for a text of nothing else.
 */
std::string trimmed(
    const std::string& _text
)
{
    const std::size_t first = _text.find_first_not_of(" \t");

    // nothing but spaces
    if (first == std::string::npos)
    {
        return std::string();
    }

    return _text.substr(first,
                        _text.find_last_not_of(" \t") - first + 1u);
}

/**
 * @brief Lays out what the recognition pass counted, rule by rule.
 *
 * @param[in] _run  the run.
 * @return a row a rule, in program order -- its calls, the calls that
 *         repeated one at the same position, the steps those ran, the calls
 *         the memo answered, and its steps, arrivals and revisits -- then a
 *         row for the gate and one for the whole run; a rule with a repeat
 *         is marked, since a repeat is work a memo would not do again.
 */
listing profile_listing(
    const run_record& _run
)
{
    listing                 out;
    const parse_pass&       rec    = _run.recognition;
    const vp::peg::profile& counts = rec.counts;

    out.columns = { "rule", "calls", "repeats", "repeat steps", "memo hits",
                    "steps", "arrivals", "revisits" };
    out.note    = "The program did not run, so it counted nothing.";

    // nothing ran to count
    if (!rec.ran)
    {
        return out;
    }

    std::size_t calls    = 0u;
    std::size_t repeats  = 0u;
    std::size_t repeated = 0u;
    std::size_t hits     = 0u;
    std::size_t arrivals = 0u;

    const auto add = [&out](const std::vector<std::string>& _row,
                            int                             _mark)
    {
        std::string key;

        // the row's text, cell by cell, for comparing two runs
        for (const std::string& cell : _row)
        {
            key += (key.empty() ? "" : " ") + cell;
        }

        out.rows.push_back(_row);
        out.keys.push_back(key);
        out.marks.push_back(_mark);

        return;
    };

    // a row a rule, in program order
    for (const vp::peg::profile::DRuleCounts& each : counts.rules)
    {
        calls    += each.calls;
        repeats  += each.repeats;
        repeated += each.repeat_steps;
        hits     += each.hits;
        arrivals += each.arrivals;

        add({ entry_name(_run.entry,
                         each.start),
              std::to_string(each.calls),
              std::to_string(each.repeats),
              std::to_string(each.repeat_steps),
              std::to_string(each.hits),
              std::to_string(each.steps),
              std::to_string(each.arrivals),
              std::to_string(each.revisits) },
            (each.repeats > 0u) ? 2 : 0);
    }

    add({ "gate", "-", "-", "-", "-", std::to_string(counts.gate_steps), "-",
          "-" },
        1);
    add({ "all", std::to_string(calls), std::to_string(repeats),
          std::to_string(repeated), std::to_string(hits),
          std::to_string(counts.steps), std::to_string(arrivals),
          std::to_string(counts.revisits) },
        1);

    return out;
}

/**
 * @brief Makes the program call the rules the request names through the
 *        memo.
 *
 * @param[in,out] _run  the run, its program built and its entries labelled;
 *                      its program memoized and the rules named, or, for a
 *                      name no rule has, not built and the reason said.
 *
 * For "auto", the program runs over the input once without the memo,
 * counted and untraced, under the request's step limit, and the rules its
 * profile says a memo pays for are the ones chosen (peg::worth_memoizing).
 * The machine is deterministic, so the run the request asked for is the
 * same run, with the memo.
 */
void memoize(
    run_record& _run
)
{
    const std::string& setting = _run.request.memoize;
    std::vector<int>   starts;

    // a counted run without the memo chooses
    if (setting == "auto")
    {
        static const vp::op_set ops = vp::peg::make_ops();

        pr::parse_state<char> state(_run.request.input.data(),
                                    _run.request.input.size(),
                                    0);
        vp::machine           machine;
        vp::peg::profile      counts;
        vp::peg::sinks        sinks;

        machine.step_limit = _run.request.step_limit;
        sinks.counts       = &counts;

        (void)vp::peg::run(machine,
                           state,
                           _run.program,
                           ops,
                           sinks);

        starts                = vp::peg::worth_memoizing(counts);
        _run.unmemoized_steps = machine.steps;
    }
    else
    {
        std::string unknown;

        starts = memo_rules(_run,
                            setting,
                            &unknown);

        // a name no rule has is the request's fault, said as one
        if (!unknown.empty())
        {
            _run.build_error = "no rule is named " + unknown +
                               ", to be called through the memo";
            _run.built       = false;

            return;
        }
    }

    _run.program = vp::peg::memoized(_run.program,
                                     starts);

    // the rules, by name
    for (const int start : starts)
    {
        _run.memoized.push_back(entry_name(_run.entry,
                                           start));
    }

    return;
}

}  // anonymous namespace


/*
execute
  Runs the stages in order, stopping at the first that cannot continue, and
builds every listing whatever happened, so a failed run is as inspectable as
a good one.
*/
run_record execute(
    int                _id,
    const run_request& _request
)
{
    run_record run;
    run.id      = _id;
    run.when    = now_text();
    run.request = _request;

    const bool               ebnf       = (_request.frontend == "ebnf");
    const steady::time_point read_start = steady::now();

    if (ebnf)
    {
        read_ebnf(run);
    }
    else
    {
        read_notation(run);
    }

    run.read_ms = elapsed_ms(read_start);

    // the library's session judges a grammar that was read, so the workbench
    // refuses what any client of the library is refused, in the same words
    const vp::compile_result verdict =
        run.read_ok ? vp::session().load(_request.grammar,
                                         ebnf ? vp::frontend::ebnf
                                              : vp::frontend::metacircular)
                    : vp::compile_result();

    if ( (run.read_ok) &&
         (!verdict.ok) )
    {
        run.build_error = verdict.error;
    }

    if ( (run.read_ok) &&
         (verdict.ok) )
    {
        const steady::time_point build_start = steady::now();

        if (ebnf)
        {
            build_ebnf(run);
        }
        else
        {
            build_notation(run);
        }

        run.build_ms = elapsed_ms(build_start);

        // the program shown is to be the session's; a difference in length
        // is a fault, said aloud rather than shown as if it were not there
        if ( (run.built) &&
             (run.program.size() != verdict.instructions) )
        {
            run.build_error = "the workbench built " +
                              plural(run.program.size(), "instruction") +
                              " where the session built " +
                              std::to_string(verdict.instructions);
            run.built       = false;
        }
    }

    // the entries labelled, then the memo, which may yet refuse the request
    if (run.built)
    {
        run.entry = label_entries(run.program,
                                  run.rule_names);
        memoize(run);
    }

    // a program, memoized or not, to run
    if (run.built)
    {
        run.recognition = run_pass(run.program, run.entry, _request.input,
                                   _request);
    }

    // where the work went: only a traced pass can say
    if (!run.recognition.trace.empty())
    {
        run.heat = std::make_shared<const heat_space>(build_heat(run));
    }

    run.parts["summary"]  = summary_listing(run);
    run.parts["grammar"]  = text_listing(_request.grammar,
                                         "The grammar is empty.");
    run.parts["reading"]  = reading_listing(run);
    run.parts["rules"]    = rules_listing(run);
    run.parts["plan"]     = plan_listing(run);
    run.parts["program"]  = program_listing(
        run.program,
        run.entry,
        run.build_error.empty() ? "No program: the grammar was not read."
                                : "No program: " + run.build_error + ".");
    run.parts["matches"]  = matches_listing(run);
    run.parts["heat"]     = run.heat ? heat_listing(*run.heat, run)
                                     : heat_listing(build_heat(run), run);
    run.parts["profile"]  = profile_listing(run);

    return run;
}

/*
memo_rules
  A name is matched to the rules' labels, so in a program the labels do not
fit -- its calls named by address -- no name names a rule, and "all" alone
reaches them. A name given twice is one rule, and an empty name between two
commas is no name.
*/
std::vector<int> memo_rules(
    const run_record&  _run,
    const std::string& _setting,
    std::string*       _unknown
)
{
    const vp::peg::layout where = vp::peg::layout_of(_run.program);
    std::vector<int>      out;

    // nothing named, or "auto", which names nothing
    if ( (_setting.empty())    ||
         (_setting == "none")  ||
         (_setting == "auto") )
    {
        return out;
    }

    // every rule
    if (_setting == "all")
    {
        return where.starts;
    }

    std::set<int> named;
    std::size_t   from = 0u;

    // each name, between commas
    while (from <= _setting.size())
    {
        const std::size_t comma = std::min(_setting.find(',',
                                                         from),
                                           _setting.size());
        const std::string name  = trimmed(_setting.substr(from,
                                                          comma - from));
        bool              found = false;

        // the rule that has the name
        for (const int start : where.starts)
        {
            // named so
            if (entry_name(_run.entry,
                           start) == name)
            {
                named.insert(start);
                found = true;
            }
        }

        // a name no rule has
        if ( (!found) &&
             (!name.empty()) )
        {
            // said, where the caller asked to hear it
            if (_unknown != nullptr)
            {
                *_unknown = name;
            }

            return std::vector<int>();
        }

        from = comma + 1u;
    }

    // in program order
    for (const int start : where.starts)
    {
        // a rule named
        if (named.count(start) != 0u)
        {
            out.push_back(start);
        }
    }

    return out;
}

/*
frontend_for_file
  The suffix is compared without regard to ASCII case, as Windows names files;
anything not EBNF is the notation, the workbench's own default.
*/
std::string frontend_for_file(
    const std::string& _name
)
{
    const std::string suffix = ".ebnf";

    // too short to end in the suffix
    if (_name.size() < suffix.size())
    {
        return "notation";
    }

    // the name's end, letter by letter, in any case
    for (std::size_t index = 0u; index < suffix.size(); index++)
    {
        const char each  = _name[_name.size() - suffix.size() + index];
        const char lower = ( (each >= 'A') &&
                             (each <= 'Z') )
                               ? static_cast<char>(each - 'A' + 'a')
                               : each;

        // one character off is not the suffix
        if (lower != suffix[index])
        {
            return "notation";
        }
    }

    return "ebnf";
}

/*
headline
  The first failing stage decides the sentence; a run that got to the end is
described by its match or its frontier.
*/
std::string headline(
    const run_record& _run
)
{
    if (!_run.read_ok)
    {
        return "The grammar could not be read: " + _run.read_error + ".";
    }

    if (!_run.built)
    {
        return "The program could not be built: " + _run.build_error + ".";
    }

    const parse_pass& rec = _run.recognition;

    if (!rec.ran)
    {
        return "The program did not run: " + rec.error + ".";
    }

    if (rec.error == "step limit")
    {
        return "The run stopped at the step limit of " +
               std::to_string(_run.request.step_limit) +
               " steps. Left recursion loops like this.";
    }

    if (rec.matched)
    {
        return "Matched all " + plural(rec.consumed, "character") + " in " +
               plural(static_cast<std::size_t>(rec.steps), "step") + ".";
    }

    if (!_run.request.trace)
    {
        return "No match. Record the trace to see how far the parser got.";
    }

    return "No match. The parser got as far as " +
           frontier_text(rec.far, _run.request.input) + ".";
}

/*
part_names
  Display order; also the saved file names.
*/
const std::vector<std::string>& part_names()
{
    static const std::vector<std::string> names = {
        "summary", "grammar", "reading", "reading_trace", "rules", "plan",
        "program", "matches", "heat", "profile", "trace" };

    return names;
}

/*
part_title
  A part's title as shown on its tab.
*/
std::string part_title(
    const std::string& _part
)
{
    if (_part == "summary")       { return "Summary";       }
    if (_part == "grammar")       { return "Grammar";       }
    if (_part == "reading")       { return "Reading";       }
    if (_part == "reading_trace") { return "Reading trace"; }
    if (_part == "rules")         { return "Rules";         }
    if (_part == "plan")          { return "Plan";          }
    if (_part == "program")       { return "Program";       }
    if (_part == "matches")       { return "Matches";       }
    if (_part == "heat")          { return "Heat";          }
    if (_part == "profile")       { return "Profile";       }
    if (_part == "trace")         { return "Trace";         }

    return _part;
}

/*
trace_rows
  A trace part's length without materializing it.
*/
std::size_t trace_rows(
    const run_record&  _run,
    const std::string& _part
)
{
    return (_part == "reading_trace") ? _run.reading.trace.size()
                                      : _run.recognition.trace.size();
}

/*
trace_row
  One row, formatted from the compact step and the program it indexes.
*/
std::vector<std::string> trace_row(
    const run_record&  _run,
    const std::string& _part,
    std::size_t        _row
)
{
    const bool              reading = (_part == "reading_trace");
    const parse_pass&       pass    = reading ? _run.reading : _run.recognition;
    const vp::peg::program& program = reading ? _run.reader : _run.program;
    const std::string&      text    = reading ? _run.request.grammar
                                              : _run.request.input;

    if (_row >= pass.trace.size())
    {
        return {};
    }

    const trace_step& step = pass.trace[_row];
    const std::string instruction =
        ( (step.ip >= 0) &&
          (static_cast<std::size_t>(step.ip) < program.size()) )
            ? disassemble(program[static_cast<std::size_t>(step.ip)])
            : "?";
    const std::string next =
        (static_cast<std::size_t>(step.off) < text.size())
            ? escape(std::string(1, text[static_cast<std::size_t>(step.off)]))
            : "end";

    return { std::to_string(_row), std::to_string(step.ip), instruction,
             std::to_string(step.off), next, std::to_string(step.calls),
             std::to_string(step.back), std::to_string(step.caps) };
}

/*
part_listing
  The trace parts are the only ones not stored; asking for one here
materializes it (used when saving).
*/
listing part_listing(
    const run_record&  _run,
    const std::string& _part
)
{
    if ( (_part == "trace") ||
         (_part == "reading_trace") )
    {
        listing out;
        out.columns = { "step", "ip", "instruction", "at", "next", "calls",
                        "back", "caps" };

        const std::size_t count = trace_rows(_run, _part);

        for (std::size_t i = 0; i < count; ++i)
        {
            out.rows.push_back(trace_row(_run, _part, i));
            out.marks.push_back(0);
        }

        const bool reading = (_part == "reading_trace");

        if (count == 0)
        {
            out.note = !_run.request.trace
                           ? "The trace was not recorded for this run."
                           : ( (reading) &&
                               (_run.request.frontend == "ebnf") )
                                 ? "EBNF is read by a hand-written parser, "
                                   "not the machine, so there is no trace."
                                 : "The program did not run.";
        }

        return out;
    }

    const auto found = _run.parts.find(_part);

    return (found == _run.parts.end()) ? listing{} : found->second;
}

/*
escape
  Control characters and backslashes become visible escapes, so every listing
cell is one line of printable text.
*/
std::string escape(
    const std::string& _text
)
{
    std::string out;
    out.reserve(_text.size());

    for (const char c : _text)
    {
        switch (c)
        {
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\r': out += "\\r";  break;
            case '\0': out += "\\0";  break;
            case '\\': out += "\\\\"; break;
            default:
            {
                const unsigned char u = static_cast<unsigned char>(c);

                if ( (u < 0x20) ||
                     (u == 0x7F) )
                {
                    char hex[8];
                    std::snprintf(hex, sizeof(hex), "\\x%02X", u);
                    out += hex;
                }
                else
                {
                    out += c;
                }

                break;
            }
        }
    }

    return out;
}

/*
disassemble
  fmt is vparse's own disassembly; only SET is rewritten, because its operand
is every member spelled out.
*/
std::string disassemble(
    const vp::peg::DInstr& _instr
)
{
    if (_instr.op == vp::peg::SET)
    {
        return "SET [" + compress_set(_instr.set) + "]";
    }

    return escape(vp::peg::fmt(_instr));
}

/*
line_col
  Counted in bytes; a column is one past the characters before it on its line.
*/
std::pair<int, int> line_col(
    const std::string& _text,
    int                _offset
)
{
    int line   = 1;
    int column = 1;
    const int end = std::min(_offset, static_cast<int>(_text.size()));

    for (int i = 0; i < end; ++i)
    {
        if (_text[static_cast<std::size_t>(i)] == '\n')
        {
            ++line;
            column = 1;
        }
        else
        {
            ++column;
        }
    }

    return { line, column };
}

/*
refusal_of
  Only the notation reader stops at a place: the EBNF reader says what it
expected and not where, a step limit is nowhere in particular, and a program
that was not built was refused by the whole grammar. The sentence is the
headline's with the place taken out, so that the two say the same.
*/
refusal refusal_of(
    const run_record& _run
)
{
    refusal out;

    out.sentence = headline(_run);

    // a reading of the notation that stopped at a place in the grammar
    if ( (!_run.read_ok)                      &&
         (_run.request.frontend != "ebnf")    &&
         (!_run.reading.matched)              &&
         (_run.reading.error != "step limit") &&
         (_run.reading.far.offset >= 0) )
    {
        const std::pair<int, int> at = line_col(_run.request.grammar,
                                                _run.reading.far.offset);

        out.line     = at.first;
        out.column   = at.second;
        out.sentence = "The grammar could not be read: the reader stopped" +
                       frontier_rest(_run.reading.far) + ".";
    }

    return out;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
