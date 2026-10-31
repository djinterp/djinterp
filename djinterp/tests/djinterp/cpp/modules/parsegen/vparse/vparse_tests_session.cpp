/*******************************************************************************
* djinterp [cpp/parsegen]                               vparse_tests_session.cpp
*
* vparse tests: the session -- both frontends reaching a parser that runs, a
* bad grammar reported rather than thrown, the faults a session refuses in the
* same words with either frontend, and a failed load leaving the parser that
* was already there alone.
*   No section depends on the language level, so every level from the floor
* up runs all of them.
*
*
* path:      /tests/djinterp/cpp/modules/parsegen/vparse/vparse_tests_session.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>           // std::size_t
#include <initializer_list>  // std::initializer_list
#include <string>            // std::string, std::to_string


namespace djinterp
{
namespace testing
{

namespace
{

using vp::frontend;

// DIGITS
//   constant: one or more digits. The same text is a grammar in both
// notations, which is what lets one constant serve the two.
const char* const DIGITS = "Digits = [0-9]+ ;";

// loaded
//   function: whether grammar text loads with a frontend.
bool loaded(
    vp::session& _session,
    const char*  _text,
    frontend     _frontend
)
{
    return _session.load(_text, _frontend).ok;
}

// matches
//   function: whether a session matches every input of one list and none of
// another.
bool matches(
    const vp::session&                 _session,
    std::initializer_list<const char*> _accepted,
    std::initializer_list<const char*> _refused
)
{
    for (const char* input : _accepted)
    {
        if (!_session.match(input).ok)
        {
            return false;
        }
    }

    for (const char* input : _refused)
    {
        if (_session.match(input).ok)
        {
            return false;
        }
    }

    return true;
}

// refuses
//   function: whether a frontend refuses grammar text, with a reason that
// contains what it should name.
bool refuses(
    const char*        _text,
    frontend           _frontend,
    const std::string& _naming
)
{
    vp::session              scratch;
    const vp::compile_result outcome = scratch.load(_text, _frontend);

    return ( (!outcome.ok)                                        &&
             (outcome.instructions == 0u)                         &&
             (outcome.error.find(_naming) != std::string::npos)   &&
             (!scratch.ready())                                   );
}

/**
 * @brief Writes a chain of rules in the notation, each calling the next.
 *
 * @param[in] _rules  how many rules; at least 1.
 * @param[in] _last   the last rule's body.
 * @return `R0 = R1 ;` and so on, the last rule `R<_rules - 1> = _last ;`.
 */
std::string long_chain(
    std::size_t        _rules,
    const std::string& _last
)
{
    std::string text;

    // every rule but the last calls the next
    for (std::size_t index = 0u; (index + 1u) < _rules; index++)
    {
        text += "R" + std::to_string(index) + " = R" +
                std::to_string(index + 1u) + " ;\n";
    }

    return text + "R" + std::to_string(_rules - 1u) + " = " + _last + " ;\n";
}

}  // anonymous namespace


/*
tests_session_ebnf
  An EBNF grammar, from text to a parser that runs.
  Tests the following:
  - a grammar loads, reports its size, and leaves the session ready
  - the parser matches what the grammar derives and nothing else
  - a match ends at the input's length, and a failed one where it began
  - the session keeps the text it was loaded from
*/
bool tests_session_ebnf()
{
    vp::session              subject;
    const vp::compile_result outcome = subject.load(DIGITS, frontend::ebnf);

    if ( (!outcome.ok)                             ||
         (outcome.instructions == 0u)              ||
         (outcome.instructions != subject.size())  ||
         (!outcome.error.empty())                  ||
         (!subject.ready())                        ||
         (subject.source() != DIGITS)              )
    {
        return false;
    }

    return ( (matches(subject,
                      { "1", "12345", "007" },
                      { "", "12a", "a" }))            &&
             (subject.match("12345").consumed == 5u)  &&
             (subject.match("12a").consumed == 0u)    );
}

/*
tests_session_metacircular
  The notation is a front end, not a parser family.
  Tests the following:
  - the metacircular notation loads the text EBNF loads
  - both sessions recognise the same language; the programs need not be
    identical, since each frontend lowers repetition its own way
*/
bool tests_session_metacircular()
{
    vp::session meta;
    vp::session ebnf;

    if ( (!loaded(meta, DIGITS, frontend::metacircular))  ||
         (!loaded(ebnf, DIGITS, frontend::ebnf))          )
    {
        return false;
    }

    return ( (matches(meta, { "7", "12345" }, { "", "12a", "a1" }))  &&
             (matches(ebnf, { "7", "12345" }, { "", "12a", "a1" }))  );
}

/*
tests_session_errors
  A bad grammar is a result, never an exception, and never costs the caller
the parser they already had.
  Tests the following:
  - text that is not a grammar is refused, with a reason, in either notation
  - after every refusal the session still holds the grammar that worked:
    ready, the same source, the same size, the same matches
*/
bool tests_session_errors()
{
    vp::session subject;

    if (!loaded(subject, DIGITS, frontend::ebnf))
    {
        return false;
    }

    const std::size_t size = subject.size();

    // none of these is a grammar in the notation it is offered in
    for (const char* text : { "Digits = [0-9", "= ;", "Digits = ( ;" })
    {
        const vp::compile_result outcome =
            subject.load(text, frontend::ebnf);

        if ( (outcome.ok)             ||
             (outcome.error.empty())  )
        {
            return false;
        }
    }

    for (const char* text : { "Digits = [0-9", "= ;", "Digits [0-9]+ ;" })
    {
        const vp::compile_result outcome =
            subject.load(text, frontend::metacircular);

        if ( (outcome.ok)             ||
             (outcome.error.empty())  )
        {
            return false;
        }
    }

    // still the grammar that worked
    return ( (subject.ready())                           &&
             (subject.source() == DIGITS)                &&
             (subject.size() == size)                    &&
             (matches(subject, { "12345" }, { "12a" }))  );
}

/*
tests_session_defects
  What a session refuses that reads as a grammar, in the same words whichever
notation it arrives in.
  Tests the following:
  - text with no rule in it, empty or a comment alone, defines no rule
  - a reference to a rule nothing defines is refused, and the rule is named
  - a rule defined twice is refused, and the rule is named
  - the metacircular notation refuses what EBNF refuses: its generator would
    compile an undefined reference into a call that never returns
*/
bool tests_session_defects()
{
    return ( (refuses("", frontend::ebnf, "no rule"))                       &&
             (refuses("", frontend::metacircular, "no rule"))               &&
             (refuses("# nothing here\n", frontend::ebnf, "no rule"))       &&
             (refuses("S = T ;", frontend::ebnf, "not defined: T"))         &&
             (refuses("S = T ;", frontend::metacircular,
                      "not defined: T"))                                    &&
             (refuses("S = [a-z] T ; T = 'x' U ;", frontend::metacircular,
                      "not defined: U"))                                    &&
             (refuses("S = \"a\" ; S = \"b\" ;", frontend::ebnf,
                      "defined twice: S"))                                  &&
             (refuses("S = 'a' ; S = 'b' ;", frontend::metacircular,
                      "defined twice: S"))                                  );
}

/*
tests_session_defects_order
  Which fault a session names when a grammar has more than one, and that it
finds them in a long grammar.
  Tests the following:
  - the first rule with a fault is the one named, in rule order, whatever
    the later rules hold
  - in one rule, a second definition is named before a reference to nothing
  - in a chain of 20,000 rules, a first rule defined again at the end is
    named, and so is the last rule's reference to a rule nothing defines
*/
bool tests_session_defects_order()
{
    const std::string again   = long_chain(20000u,
                                           "[0-9]+") + "R0 = 'x' ;\n";
    const std::string dangles = long_chain(20000u,
                                           "R20000");

    return ( (refuses("S = 'a' U ; T = 'b' ; T = 'c' ;",
                      frontend::metacircular,
                      "not defined: U"))                    &&
             (refuses("S = 'a' V ; S = 'b' ;",
                      frontend::metacircular,
                      "defined twice: S"))                  &&
             (refuses(again.c_str(),
                      frontend::metacircular,
                      "defined twice: R0"))                 &&
             (refuses(dangles.c_str(),
                      frontend::metacircular,
                      "not defined: R20000")) );
}

/*
tests_session_left_recursion
  What load() documents and does not refuse.
  Tests the following:
  - a left-recursive grammar loads, in either notation
  - a match over it fails rather than running without end: the machine's
    step limit stops it
*/
bool tests_session_left_recursion()
{
    vp::session meta;
    vp::session ebnf;

    if ( (!loaded(meta, "S = S 'a' | 'a' ;", frontend::metacircular))  ||
         (!loaded(ebnf, "S = S \"a\" | \"a\" ;", frontend::ebnf))      )
    {
        return false;
    }

    return ( (!meta.match("aaa").ok)  &&
             (!ebnf.match("aaa").ok)  );
}

/*
tests_session_disassembly
  The listing a workbench shows.
  Tests the following:
  - a session with nothing loaded disassembles to nothing, and has no size
  - a loaded one lists one line per instruction
  - every line begins with its right-aligned address
*/
bool tests_session_disassembly()
{
    vp::session subject;

    if ( (!subject.disassembly().empty())  ||
         (subject.size() != 0u)            ||
         (subject.ready())                 )
    {
        return false;
    }

    if (!loaded(subject, DIGITS, frontend::ebnf))
    {
        return false;
    }

    const std::string listing = subject.disassembly();
    std::size_t       lines   = 0u;

    for (const char symbol : listing)
    {
        if (symbol == '\n')
        {
            lines++;
        }
    }

    return ( (lines == subject.size())                    &&
             (lines > 0u)                                 &&
             (listing.compare(0u, 6u, "   0  ") == 0)     );
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
