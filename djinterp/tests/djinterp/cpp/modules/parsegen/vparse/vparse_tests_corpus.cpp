/*******************************************************************************
* djinterp [cpp/parsegen]                                vparse_tests_corpus.cpp
*
* vparse tests: a corpus made from a grammar, and two programs compared over
* one -- the same corpus from the same seed, walks that end however the rules
* recurse, near-misses one character from a walk, two grammars of one
* language agreeing, and a grammar with an alternative dropped found out by
* the inputs it is named for.
*   Where a verdict could come from the code under test, it does not: each
* difference compare() names is run again here, program by program.
*   No section depends on the language level, so every level from the floor
* up runs all of them.
*
*
* path:      /tests/djinterp/cpp/modules/parsegen/vparse/vparse_tests_corpus.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/
#include "./vparse_tests.hpp"  // helpers, and the section declarations
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <algorithm>  // std::min
#include <cstddef>    // std::size_t
#include <string>     // std::string
#include <vector>     // std::vector
// djinterp
#include "../../../../../../inc/djinterp/cpp/modules/parsegen/vparse/corpus.hpp"  // generate,
                                                                // compare


namespace djinterp
{
namespace testing
{

namespace
{

// EXPRESSIONS
//   constant: expressions, each alternative of Sum and of Prod beginning with
// the same rule.
const char* const EXPRESSIONS =
    "Expr = Sum ;\n"
    "Sum  = Prod '+' Sum | Prod '-' Sum | Prod ;\n"
    "Prod = Atom '*' Prod | Atom '/' Prod | Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// FACTORED_EXPRESSIONS
//   constant: the same language, each rule's shared beginning factored out.
const char* const FACTORED_EXPRESSIONS =
    "Expr = Sum ;\n"
    "Sum  = Prod More* ;\n"
    "More = '+' Prod | '-' Prod ;\n"
    "Prod = Atom Then* ;\n"
    "Then = '*' Atom | '/' Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// NO_MINUS
//   constant: EXPRESSIONS with one alternative dropped, `Prod '-' Sum`.
const char* const NO_MINUS =
    "Expr = Sum ;\n"
    "Sum  = Prod '+' Sum | Prod ;\n"
    "Prod = Atom '*' Prod | Atom '/' Prod | Atom ;\n"
    "Atom = '(' Sum ')' | Num ;\n"
    "Num  = [0-9]+ ;\n";

// ENOUGH
//   constant: a step limit no run here comes near.
const long ENOUGH = 100000000L;

/**
 * @brief Reads grammar text in the notation into rules.
 *
 * @param[in] _text  the grammar.
 * @return its rules; none where the text is not a grammar.
 */
vp::ruleset rules_of(
    const char* _text
)
{
    bool read = false;

    return vp::notation::parse_grammar(_text,
                                       &read);
}

/**
 * @brief Gives a corpus's texts, in order.
 *
 * @param[in] _samples  the corpus.
 * @return each sample's text.
 */
std::vector<std::string> texts_of(
    const std::vector<vp::corpus::sample>& _samples
)
{
    std::vector<std::string> out;

    // each text, in order
    for (const vp::corpus::sample& each : _samples)
    {
        out.push_back(each.text);
    }

    return out;
}

/**
 * @brief Counts the characters two texts differ by, edit for edit.
 *
 * @param[in] _one    a text.
 * @param[in] _other  another.
 * @return the Levenshtein distance between them.
 */
std::size_t distance(
    const std::string& _one,
    const std::string& _other
)
{
    std::vector<std::size_t> row(_other.size() + 1u);

    // the first row: insertions alone
    for (std::size_t at = 0u; at < row.size(); at++)
    {
        row[at] = at;
    }

    // a row for each character of the first
    for (std::size_t i = 1u; i <= _one.size(); i++)
    {
        std::size_t diagonal = row[0];

        row[0] = i;

        // a cell for each character of the second
        for (std::size_t j = 1u; j <= _other.size(); j++)
        {
            const std::size_t above = row[j];
            const std::size_t cost  = (_one[i - 1u] == _other[j - 1u]) ? 0u
                                                                       : 1u;

            row[j]   = std::min(std::min(row[j] + 1u,
                                         row[j - 1u] + 1u),
                                diagonal + cost);
            diagonal = above;
        }
    }

    return row[_other.size()];
}

}  // anonymous namespace


/*
tests_corpus_generate
  A corpus is the grammar's, and the seed's.
  Tests the following:
  - one seed gives one corpus, twice over; another seed gives another
  - each walk comes before its near-miss, every text is in the corpus once,
    and a near-miss, of which there are some, is one character from a text
    before it
  - every walk of the expression grammar is accepted by its program, and the
    length a walk grows to before it takes the shortest way out bounds it
  - at a depth of nothing, every walk takes each rule's shortest way out:
    one digit, for the expression grammar
  - a grammar whose start rule reaches itself before any text has no walk
    that ends, and no corpus; a left-recursive one with a way out has one
*/
bool tests_corpus_generate()
{
    const vp::ruleset    grammar = rules_of(EXPRESSIONS);
    vp::corpus::settings how;

    how.walks  = 40u;
    how.length = 64u;

    const std::vector<vp::corpus::sample> first  = vp::corpus::generate(grammar,
                                                                         how);
    const std::vector<vp::corpus::sample> again  = vp::corpus::generate(grammar,
                                                                         how);

    how.seed = 2u;

    const std::vector<vp::corpus::sample> other  = vp::corpus::generate(grammar,
                                                                         how);
    const vp::peg::program                parser = vp::gen::compile(grammar);

    // one seed, one corpus; two seeds, two
    if ( (first.empty())                         ||
         (texts_of(first) != texts_of(again))    ||
         (texts_of(first) == texts_of(other))    ||
         (!first[0].derived) )
    {
        return false;
    }

    std::size_t misses = 0u;

    // each text once, each walk accepted and bounded, each miss near a text
    for (std::size_t at = 0u; at < first.size(); at++)
    {
        const vp::corpus::sample& each = first[at];
        bool                      near = each.derived;

        misses += each.derived ? 0u : 1u;

        // a text the corpus holds twice
        for (std::size_t before = 0u; before < at; before++)
        {
            // the same text
            if (first[before].text == each.text)
            {
                return false;
            }

            near = ( (near) ||
                     (distance(first[before].text,
                               each.text) == 1u) );
        }

        // a walk the program refuses, or one grown far past the length
        if ( (each.derived) &&
             ( (!vp::corpus::judge(parser,
                                   each.text,
                                   ENOUGH).ok) ||
               (each.text.size() > (4u * how.length)) ) )
        {
            return false;
        }

        // a near-miss further than one edit from every text before it
        if (!near)
        {
            return false;
        }
    }

    vp::corpus::settings shallow;

    shallow.depth = 0u;

    // at no depth, the shortest way out of every rule
    for (const vp::corpus::sample& each : vp::corpus::generate(grammar,
                                                               shallow))
    {
        // a walk longer than one digit
        if ( (each.derived) &&
             ( (each.text.size() != 1u) ||
               (each.text[0] < '0') ||
               (each.text[0] > '9') ) )
        {
            return false;
        }
    }

    return ( (misses > 0u)                                           &&
             (vp::corpus::generate(rules_of("S = S 'a' ;"),
                                   how).empty())                     &&
             (!vp::corpus::generate(rules_of("S = S 'a' | 'b' ;"),
                                    how).empty()) );
}

/*
tests_corpus_compare
  Two programs over one corpus: agreement, and every difference named.
  Tests the following:
  - the nested and the factored expression grammars agree over a corpus of
    the nested one, and both match the same inputs
  - with `Prod '-' Sum` dropped, the corpus finds the difference, and each
    input it names is one the two programs, run again here, say different
    things of: the full grammar matches it, the other does not
  - a step limit one program runs out under and the other does not is a
    difference too, the one that did not finish said to be so, even where
    neither matched
  - a program and its memoized self agree, the memoized taking fewer steps
*/
bool tests_corpus_compare()
{
    const vp::ruleset    grammar = rules_of(EXPRESSIONS);
    vp::corpus::settings how;

    how.walks = 30u;

    const std::vector<std::string> inputs   =
        texts_of(vp::corpus::generate(grammar,
                                      how));
    const vp::peg::program         nested   = vp::gen::compile(grammar);
    const vp::peg::program         factored =
        vp::gen::compile(rules_of(FACTORED_EXPRESSIONS));
    const vp::peg::program         dropped  =
        vp::gen::compile(rules_of(NO_MINUS));
    const vp::corpus::comparison   same     = vp::corpus::compare(nested,
                                                                  factored,
                                                                  inputs,
                                                                  ENOUGH);
    const vp::corpus::comparison   found    = vp::corpus::compare(nested,
                                                                  dropped,
                                                                  inputs,
                                                                  ENOUGH);

    // one language, and an alternative short of it
    if ( (same.inputs != inputs.size())     ||
         (!same.differences.empty())        ||
         (same.matched == 0u)               ||
         (found.differences.empty()) )
    {
        return false;
    }

    // each difference, run again
    for (const vp::corpus::difference& each : found.differences)
    {
        const std::string&        input = inputs[each.index];
        const vp::corpus::verdict full  = vp::corpus::judge(nested,
                                                            input,
                                                            ENOUGH);
        const vp::corpus::verdict less  = vp::corpus::judge(dropped,
                                                            input,
                                                            ENOUGH);

        // the input named is not one the two disagree on
        if ( (!full.ok)        ||
             (less.ok)         ||
             (!each.first.ok)  ||
             (each.second.ok) )
        {
            return false;
        }
    }

    const std::vector<std::string> deep     = { "((((1))))" };
    const vp::corpus::comparison   short_of = vp::corpus::compare(nested,
                                                                  factored,
                                                                  deep,
                                                                  10000L);
    const vp::peg::layout          where    = vp::peg::layout_of(nested);
    const vp::corpus::comparison   memo     =
        vp::corpus::compare(nested,
                            vp::peg::memoized(nested,
                                              where.starts),
                            inputs,
                            ENOUGH);

    const std::vector<std::string> a        = { "a" };
    const vp::corpus::comparison   loops    =
        vp::corpus::compare(vp::gen::compile(rules_of("S = S 'a' | 'a' ;")),
                            vp::gen::compile(rules_of("S = 'b' ;")),
                            a,
                            10000L);

    return ( (short_of.differences.size() == 1u)            &&
             (!short_of.differences[0].first.finished)      &&
             (short_of.differences[0].second.finished)      &&
             (short_of.differences[0].second.ok)            &&
             (loops.differences.size() == 1u)               &&
             (!loops.differences[0].first.ok)               &&
             (!loops.differences[0].second.ok)              &&
             (!loops.differences[0].first.finished)         &&
             (memo.differences.empty())                     &&
             (memo.second_steps < memo.first_steps) );
}

}  // namespace testing
}  // namespace djinterp

#endif  // floor, for now
