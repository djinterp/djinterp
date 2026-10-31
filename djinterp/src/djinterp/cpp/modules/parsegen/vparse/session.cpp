/*******************************************************************************
* djinterp [cpp/parsegen]                                            session.cpp
*
* Definitions for session.hpp.
*   Both frontends land on the same peg::program, which is the point: the
* notation is a front end, not a parser family. Adding a third frontend is a
* function beside the two here, and a case in load().
*   The two frontends do not refuse the same things. EBNF's compile step
* reports an undefined rule and a rule defined twice; the metacircular
* notation's generator looks for neither, and compiles a reference to nothing
* into a call that never returns. A session refuses both with either
* frontend, in the same words, so the check the generator lacks is made here.
*
* path:      /src/djinterp/cpp/modules/parsegen/vparse/session.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "djinterp/cpp/modules/parsegen/vparse/session.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>        // std::size_t
#include <string>         // std::string, std::to_string
#include <unordered_map>  // std::unordered_map, the rules' name counts
#include <utility>        // std::move
#include <vector>         // std::vector
// djinterp
#include "djinterp/cpp/modules/parse/parse.hpp"               // parse_state
#include "djinterp/cpp/modules/parsegen/vparse/ebnf.hpp"      // the EBNF frontend
#include "djinterp/cpp/modules/parsegen/vparse/gen.hpp"       // gen::compile
#include "djinterp/cpp/modules/parsegen/vparse/machine.hpp"   // op_set
#include "djinterp/cpp/modules/parsegen/vparse/notation.hpp"  // the metacircular notation
#include "djinterp/cpp/modules/parsegen/vparse/peg.hpp"       // peg::run, peg::fmt
#include "djinterp/cpp/modules/parsegen/vparse/ruleset.hpp"   // ruleset, rule, term


NS_DJINTERP
NS_PARSEGEN
NS_VPARSE

namespace
{

namespace pr = ::djinterp::parse;

// NO_RULE
//   constant: what a grammar with nothing in it is told. Both notations read
// empty text without complaint, and neither then has anything to compile.
const char* const NO_RULE = "the grammar defines no rule";

// ADDRESS_WIDTH
//   constant: the column a disassembly's addresses are right-aligned to.
const std::size_t ADDRESS_WIDTH = 4u;

// peg_ops
//   function: the PEG operator registry, built once. It is immutable after
// construction and carries no per-run state, so one copy serves every
// session.
const op_set& peg_ops()
{
    static const op_set ops = peg::make_ops();

    return ops;
}

// name_counts
//   type: how many of a grammar's rules carry each name.
using name_counts = std::unordered_map<std::string, std::size_t>;

/**
 * @brief Counts the rules that carry each name.
 *
 * @param[in] _grammar  the grammar.
 * @return each name a rule carries, and how many rules carry it.
 *
 * One pass over the rules, so that judging a grammar is linear in it: asking
 * every rule how many rules share its name, and every reference whether any
 * rule has it, was a pass over the rules each time, and a grammar of 40,000
 * rules spent six seconds there.
 */
name_counts count_names(
    const ruleset& _grammar
)
{
    name_counts counts;

    counts.reserve(_grammar.rules.size());

    // each rule's name, once for every rule that carries it
    for (const rule& each : _grammar.rules)
    {
        counts[each.name]++;
    }

    return counts;
}

/**
 * @brief Says how many rules carry a name.
 *
 * @param[in] _counts  the grammar's name counts.
 * @param[in] _name    the name.
 * @return the number of rules carrying `_name`; 0 for a name none carries.
 */
std::size_t times_defined(
    const name_counts& _counts,
    const std::string& _name
)
{
    const name_counts::const_iterator found = _counts.find(_name);

    return (found == _counts.end()) ? 0u
                                    : found->second;
}

/**
 * @brief Says what is wrong with one rule of a grammar: it is defined twice,
 *        or it refers to a rule nothing defines.
 *
 * @param[in] _counts  the grammar's name counts.
 * @param[in] _rule    the rule.
 * @return the fault, in the words the EBNF frontend uses for the same
 *         faults; empty when there is none.
 */
std::string first_defect_in(
    const name_counts& _counts,
    const rule&        _rule
)
{
    // a name more than one rule carries
    if (times_defined(_counts,
                      _rule.name) > 1u)
    {
        return "rule defined twice: " + _rule.name;
    }

    // every term of every alternative, in order
    for (const std::vector<term>& alternative : _rule.alts)
    {
        // each term of it
        for (const term& part : alternative)
        {
            // a reference to a name no rule carries
            if ( (part.kind == T_REF) &&
                 (times_defined(_counts,
                                part.ref) == 0u) )
            {
                return "rule not defined: " + part.ref;
            }
        }
    }

    return std::string();
}

/**
 * @brief Finds the first thing wrong with a grammar that the generator would
 *        not notice.
 *
 * @param[in] _grammar  the grammar.
 * @return the first fault, in rule order; empty when there is none.
 *
 * The generator compiles whatever it is given: a reference to an undefined
 * rule becomes a call that never returns, which only the machine's step limit
 * then stops.
 */
std::string first_defect(
    const ruleset& _grammar
)
{
    // nothing to judge, and nothing to start from
    if (_grammar.rules.empty())
    {
        return NO_RULE;
    }

    const name_counts counts = count_names(_grammar);

    // the rules in order: the first fault found is the one reported
    for (const rule& each : _grammar.rules)
    {
        const std::string defect = first_defect_in(counts,
                                                   each);

        // a rule with something wrong with it
        if (!defect.empty())
        {
            return defect;
        }
    }

    return std::string();
}

// compile_ebnf
//   function: EBNF text to a program, or the reason there is none. Both of
// the frontend's steps report through a flag and may leave their message
// empty, so each has a message to fall back on.
bool compile_ebnf(
    const std::string& _text,
    peg::program&      _program,
    std::string&       _error
)
{
    bool       parsed  = false;
    const auto grammar = ebnf::parse(_text, &parsed, &_error);

    if (!parsed)
    {
        if (_error.empty())
        {
            _error = "could not read the grammar";
        }

        return false;
    }

    // text with no rule in it parses, and has no start rule to name
    if (grammar.second.empty())
    {
        _error = NO_RULE;

        return false;
    }

    bool compiled = false;

    _program = ebnf::compile(grammar.first,
                                 grammar.second,
                                 &compiled,
                                 &_error);

    if ( (!compiled)      &&
         (_error.empty()) )
    {
        _error = "could not compile the grammar";
    }

    return compiled;
}

// compile_metacircular
//   function: text in djinterp's own notation to a program, or the reason
// there is none. The notation reports through a flag alone and the generator
// not at all, so every message is this function's.
bool compile_metacircular(
    const std::string& _text,
    peg::program&      _program,
    std::string&       _error
)
{
    bool              read    = false;
    const ruleset grammar = notation::parse_grammar(_text, &read);

    if (!read)
    {
        _error = "could not read the grammar";

        return false;
    }

    _error = first_defect(grammar);

    if (!_error.empty())
    {
        return false;
    }

    _program = gen::compile(grammar);

    return true;
}

}  // namespace

/*
session::load
  The program is compiled into a local and moved in only once it is whole, so
a failed load cannot leave the session holding half of one: the parser a
previous load produced, and its source, stay as they were. A workbench user
editing a working grammar into a broken one keeps the parser they had.
*/
compile_result session::load(
    const std::string& _text,
    frontend           _frontend
)
{
    compile_result   result;
    peg::program program;

    result.ok = (_frontend == frontend::ebnf)
                    ? compile_ebnf(_text, program, result.error)
                    : compile_metacircular(_text, program, result.error);

    // a grammar that did not compile is reported, and changes nothing
    if (!result.ok)
    {
        return result;
    }

    m_program           = std::move(program);
    m_source            = _text;
    m_ready             = true;
    result.instructions = m_program.size();

    return result;
}

/*
session::match
  A fresh parse state per call, so a session is safe to match against from
anywhere it is safe to read. A session with nothing loaded answers without
running: an empty program is a failed run, and saying so here keeps the
unloaded case out of the machine.
*/
DMatchResult session::match(
    const std::string& _input
) const
{
    DMatchResult result;

    if (!m_ready)
    {
        return result;
    }

    pr::parse_state<char> state(_input.data(), _input.size(), 0);

    result.ok       = peg::run(state, m_program, peg_ops());
    result.consumed = state.offset;

    return result;
}

/*
session::disassembly
  One line per instruction, each prefixed with its address so that a branch
operand can be followed by eye. Addresses are right-aligned, which keeps the
mnemonics in one column up to a program of ten thousand instructions.
*/
std::string session::disassembly() const
{
    std::string listing;

    for (std::size_t index = 0u; index < m_program.size(); index++)
    {
        const std::string address = std::to_string(index);

        if (address.size() < ADDRESS_WIDTH)
        {
            listing.append(ADDRESS_WIDTH - address.size(), ' ');
        }

        listing += address;
        listing += "  ";
        listing += peg::fmt(m_program[index]);
        listing += "\n";
    }

    return listing;
}

/*
session::ready
  Whether a load has succeeded. It stays true through later failed loads,
since those leave the loaded parser in place.
*/
bool session::ready() const
{
    return m_ready;
}

/*
session::size
  The loaded program's length in instructions; zero until a load succeeds.
*/
std::size_t session::size() const
{
    return m_program.size();
}

/*
session::source
  The text of the grammar that is loaded -- of the last load that succeeded,
not of the last one attempted.
*/
const std::string& session::source() const
{
    return m_source;
}

NS_END  // vparse
NS_END  // parsegen
NS_END  // djinterp

#endif  // floor, for now
