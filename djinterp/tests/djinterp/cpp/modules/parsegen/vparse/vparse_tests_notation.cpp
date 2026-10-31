/*******************************************************************************
* djinterp [cpp/parsegen]                              vparse_tests_notation.cpp
*
* vparse tests: the metacircular notation -- the anchor reached through text,
* alternation, '+' with a character literal, and a grammar too long to read
* under a machine's default step limit.
*
*
* path:      /tests/djinterp/cpp/modules/parsegen/vparse/vparse_tests_notation.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

namespace djinterp
{
namespace testing
{

namespace
{

/**
 * @brief A grammar of a chain of rules, each of which may defer to the next.
 *
 * @param[in] _rules  how many rules the grammar has; at least one.
 * @return the grammar's text, `R0 = [a-z] R1 | [0-9] ;` and on, the last rule
 *         a digit alone: every rule a rule refers to is defined, and the
 *         language is up to `_rules - 1` letters and then a digit.
 */
std::string chain_grammar(
    std::size_t _rules
)
{
    std::string out;

    // every rule but the last defers to the next
    for (std::size_t at = 0u; (at + 1u) < _rules; ++at)
    {
        out += "R" + std::to_string(at) + " = [a-z] R" +
               std::to_string(at + 1u) + " | [0-9] ;\n";
    }

    out += "R" + std::to_string(_rules - 1u) + " = [0-9] ;\n";

    return out;
}

/**
 * @brief A grammar whose start rule is many alternatives of one reference.
 *
 * @param[in] _alternatives  how many alternatives the start rule has.
 * @return the grammar's text, `S=a|a|...|a;` and then the rule `a`: the text
 *         the reader spends the most steps a byte on.
 */
std::string choice_grammar(
    std::size_t _alternatives
)
{
    std::string out = "S=a";

    // one alternative is there; the rest each follow a bar
    for (std::size_t at = 1u; at < _alternatives; ++at)
    {
        out += "|a";
    }

    out += ";\na = 'x' ;\n";

    return out;
}

/**
 * @brief Whether reading a text takes more steps than a machine's default
 *        limit allows.
 *
 * @param[in] _text  the text.
 * @return `true` when the grammar of grammars, run over `_text` on a machine
 *         as it comes, stops at its step limit.
 *
 * What makes the section's texts long enough to test the reader's own limit:
 * a text this returns `false` for would be read under the default one too.
 */
bool past_default_limit(
    const std::string& _text
)
{
    const vp::peg::program reader =
        vp::gen::compile(vp::notation::meta_grammar());

    vp::machine           limited;
    pr::parse_state<char> state(_text.data(),
                                _text.size(),
                                0);

    const bool read = vp::peg::run(limited,
                                   state,
                                   reader,
                                   peg_ops());

    return ( (!read)                         &&
             (limited.error == "step limit") );
}

}  // namespace

// tests_notation_anchor
//   the text front-end (via '+') reproduces the hand-written parser: the whole
// text -> ruleset -> generator pipeline agrees with the oracle.
bool tests_notation_anchor()
{
    bool        ok = false;
    vp::ruleset g  = vp::notation::parse_grammar("S = [0-9]+ ;", &ok);
    if (!ok) { return false; }

    vp::peg::program prog = vp::gen::compile(g);
    return same(prog, digits_program());
}

// tests_notation_alternation
//   '|' in the notation compiles to an ordered choice: the parser recognises
// either alternative and nothing mixed.
bool tests_notation_alternation()
{
    bool        ok = false;
    vp::ruleset g  = vp::notation::parse_grammar("S = [0-9]+ | [a-z]+ ;", &ok);
    if (!ok) { return false; }

    vp::peg::program prog = vp::gen::compile(g);
    if (!accept(prog, "123"))  { return false; }
    if (!accept(prog, "abc"))  { return false; }
    if ( accept(prog, "12a"))  { return false; }
    if ( accept(prog, ""))     { return false; }
    return true;
}

// tests_notation_plus_literal
//   a grammar exercising '|', '+', and a character literal together compiles
// to a working (right-recursive) recognizer.
bool tests_notation_plus_literal()
{
    bool        ok = false;
    vp::ruleset g  = vp::notation::parse_grammar(
        "Expr = Term '+' Expr | Term ;\n"
        "Term = [0-9]+ ;", &ok);
    if (!ok) { return false; }

    vp::peg::program prog = vp::gen::compile(g);
    if (!accept(prog, "1"))     { return false; }
    if (!accept(prog, "1+2"))   { return false; }
    if (!accept(prog, "1+2+3")) { return false; }
    if ( accept(prog, "1+"))    { return false; }
    if ( accept(prog, "+1"))    { return false; }
    if ( accept(prog, ""))      { return false; }
    return true;
}

/*
tests_notation_large_grammar
  The reader is given steps for the text it reads, so a long grammar is read
to its end rather than refused at the limit a match runs under.
  Tests the following:
  - a grammar of 1,000 rules, which takes the reader more steps than a
    machine's default limit, is read whole and compiles to a parser that runs
  - the text the reader spends the most steps a byte on is read whole, at a
    length where the steps it is given for each byte, and not the default
    limit, are what read it
  - a text as long that is not a grammar is still refused, and defines nothing
  - a session loads the 1,000 rules and matches over them
*/
bool tests_notation_large_grammar()
{
    const std::string chain   = chain_grammar(1000u);
    const std::string choices = choice_grammar(10000u);

    bool chain_read  = false;
    bool choice_read = false;
    bool junk_read   = true;

    const vp::ruleset chained = vp::notation::parse_grammar(chain,
                                                            &chain_read);
    const vp::ruleset chosen  = vp::notation::parse_grammar(choices,
                                                            &choice_read);
    const vp::ruleset junk    = vp::notation::parse_grammar(chain + "x",
                                                            &junk_read);

    // each text is past the default limit, and the grammars are read whole
    if ( (!past_default_limit(chain))            ||
         (!past_default_limit(choices))          ||
         (!chain_read)                           ||
         (chained.rules.size() != 1000u)         ||
         (!choice_read)                          ||
         (chosen.rules.size() != 2u)             ||
         (chosen.rules[0].alts.size() != 10000u) ||
         (junk_read)                             ||
         (!junk.rules.empty()) )
    {
        return false;
    }

    const vp::peg::program program = vp::gen::compile(chained);

    // the chain's parser takes letters and then a digit, and nothing else
    if ( (!accept(program, "abc7"))  ||
         (!accept(program, "5"))     ||
         (accept(program, "abc"))    ||
         (accept(program, "7a")) )
    {
        return false;
    }

    vp::session session;

    return ( (session.load(chain, vp::frontend::metacircular).ok)  &&
             (session.match("xyz0").ok)                            &&
             (!session.match("xyz").ok)                            );
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
