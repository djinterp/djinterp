/*******************************************************************************
* djinterp [parsegen]                                parsegen_analysis_tests.cpp
*
* The grammar-analysis suite's sections: nullability and first sets, left
* recursion, a grammar's health, and the routing that analysis and the registry
* close between them.
*   LEFT RECURSION IS A FACT, NOT AN ERROR, and the sections hold analysis to
* that: it is reported as a note, analysis succeeds, and what changes is the
* capability set the registry is then asked about. Each way a rule can and
* cannot be left-recursive is its own section, so a failure names the case.
*
*
* path:      /tests/djinterp/parsegen/parsegen_analysis_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parsegen_analysis_tests.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstring>  // std::strcmp, std::strstr
// djinterp
#include "../../../inc/djinterp/parse/charset.hpp"      // charset
#include "../../../inc/djinterp/parse/diagnostic.hpp"   // fixed_diagnostics
#include "../../../inc/djinterp/parsegen/analysis.hpp"  // facts, fixed_facts
#include "../../../inc/djinterp/parsegen/grammar.hpp"   // the grammar's face
#include "../../../inc/djinterp/parsegen/registry.hpp"  // fixed_registry
// re_std
#include "../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int32_t, uint32_t


namespace
{

using djinterp::parse::charset;
using djinterp::parse::diagnostics;
using djinterp::parse::fixed_diagnostics;
using djinterp::parse::severity;
using namespace djinterp::parsegen;

#if (D_INTERNAL_PARSEGEN_GRAMMAR_HEAP == 1)
// scratch, scratch_facts
//   type: the grammar these sections build into and the facts analysis fills
// for it. With an allocator both grow on demand; without one each carries its
// own storage.
using scratch       = grammar;
using scratch_facts = facts;

// ready
//   function: gives a scratch grammar its storage.
bool
ready(
    scratch& _grammar
)
{
    return _grammar.reserve();
}

// sized
//   function: gives scratch facts storage for a grammar.
bool
sized(
    scratch_facts& _facts,
    const scratch& _grammar
)
{
    return _facts.size_for(_grammar);
}
#else
using scratch       = fixed_grammar<256u, 16u, 2048u, 64u>;
using scratch_facts = fixed_facts<256u, 16u>;

bool
ready(
    scratch& _grammar
)
{
    (void)_grammar;

    return true;
}

bool
sized(
    scratch_facts& _facts,
    const scratch& _grammar
)
{
    (void)_facts;
    (void)_grammar;

    return true;
}
#endif  // D_INTERNAL_PARSEGEN_GRAMMAR_HEAP

// settle
//   function: resolves a grammar and sizes facts for it -- everything analysis
// needs before it runs.
bool
settle(
    scratch&       _grammar,
    scratch_facts& _facts,
    diagnostics*   _diag
)
{
    return ( (_grammar.resolve(_diag))   &&
             (sized(_facts, _grammar))   );
}

// terms
//   struct: the grammar the nullability sections share, analysed, with the
// nodes they ask about.
struct terms
{
    scratch         g;
    scratch_facts   f;
    re_std::int32_t digit   = no_node;
    re_std::int32_t alpha   = no_node;
    re_std::int32_t star    = no_node;
    re_std::int32_t plus    = no_node;
    re_std::int32_t both    = no_node;
    re_std::int32_t guarded = no_node;
    re_std::int32_t pick    = no_node;
};

// prepare
//   function: builds and analyses the shared grammar:
//     S = ( [0-9]* [a-z]+ ) ( !'_' [a-z] ) ( [0-9] / [0-9]* ) ;
bool
prepare(
    terms& _t
)
{
    if (!ready(_t.g))
    {
        return false;
    }

    _t.digit = _t.g.klass(charset::of("0-9"));
    _t.alpha = _t.g.klass(charset::of("a-z"));
    _t.star  = _t.g.star(_t.digit);
    _t.plus  = _t.g.plus(_t.alpha);

    // a sequence whose head can match empty; a predicate before a class; and
    // a choice one of whose alternatives can match empty
    _t.both    = _t.g.sequence({ _t.star, _t.plus });
    _t.guarded = _t.g.sequence({ _t.g.forbid(_t.g.literal("_")), _t.alpha });
    _t.pick    = _t.g.ordered({ _t.digit, _t.star });

    return ( (_t.g.define("S",
                          _t.g.sequence({ _t.both, _t.guarded, _t.pick })))  &&
             (settle(_t.g, _t.f, nullptr))                                   &&
             (_t.f.analyze(_t.g, nullptr))                                   );
}

// define_sum
//   function: defines the left-recursive `E = E '+' T / T ;` and `T = [0-9] ;`
// in a grammar.
bool
define_sum(
    scratch& _grammar
)
{
    const re_std::int32_t body = _grammar.ordered({
        _grammar.sequence({ _grammar.ref("E"),
                            _grammar.literal("+"),
                            _grammar.ref("T") }),
        _grammar.ref("T") });

    return ( (_grammar.define("E", body))                                    &&
             (_grammar.define("T", _grammar.klass(charset::of("0-9"))))     );
}

/*
d_tests_parsegen_analysis_terminals
  What analysis says of a terminal.
  Tests the following:
  - a terminal is not nullable
  - its first set is exactly itself
*/
bool
d_tests_parsegen_analysis_terminals(void)
{
    terms t;

    if (!prepare(t))
    {
        return false;
    }

    const charset first = *t.f.first(t.digit);

    return ( (!t.f.nullable(t.digit))   &&
             (first.test('0'))          &&
             (!first.test('a'))         &&
             (first.count() == 10u)     );
}

/*
d_tests_parsegen_analysis_repetition
  Nullability of a repetition.
  Tests the following:
  - a star is nullable whatever its child is
  - a plus is not
*/
bool
d_tests_parsegen_analysis_repetition(void)
{
    terms t;

    return ( (prepare(t))               &&
             (t.f.nullable(t.star))     &&
             (!t.f.nullable(t.plus))    );
}

/*
d_tests_parsegen_analysis_sequence
  What can begin a sequence.
  Tests the following:
  - a sequence absorbs first sets only while its members can match empty:
    `[0-9]* [a-z]+` can begin with a digit or a letter
  - it is not nullable, because its tail is not
*/
bool
d_tests_parsegen_analysis_sequence(void)
{
    terms t;

    if (!prepare(t))
    {
        return false;
    }

    const charset head = *t.f.first(t.both);

    return ( (head.test('5'))           &&
             (head.test('q'))           &&
             (!t.f.nullable(t.both))    );
}

/*
d_tests_parsegen_analysis_predicate
  What a predicate contributes, which is the subtle one.
  Tests the following:
  - a predicate consumes nothing, so it is nullable
  - it contributes NO symbol to what can begin the sequence it guards:
    `!'_' [a-z]` begins only with a letter, which is what keeps the set tight
    enough to be useful
*/
bool
d_tests_parsegen_analysis_predicate(void)
{
    terms t;

    if (!prepare(t))
    {
        return false;
    }

    const charset after = *t.f.first(t.guarded);

    return ( (after.test('q'))          &&
             (!after.test('_'))         &&
             (after.count() == 26u)     );
}

/*
d_tests_parsegen_analysis_choice
  Nullability of a choice.
  Tests the following:
  - a choice is nullable if any of its alternatives is
*/
bool
d_tests_parsegen_analysis_choice(void)
{
    terms t;

    return ( (prepare(t))               &&
             (t.f.nullable(t.pick))     );
}

/*
d_tests_parsegen_analysis_left_direct
  A rule that references itself leftmost: E = E '+' T / T.
  Tests the following:
  - it is found, and recorded as direct; the rule it calls is not
  - analysis succeeds: an error would be wrong, since whether left recursion
    matters is the family's business
  - it is reported as a note, identified by domain and code, naming the rule
  - it is folded into what the grammar reports it uses
*/
bool
d_tests_parsegen_analysis_left_direct(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    scratch                       g;
    scratch_facts                 f;

    if ( (!ready(g))                ||
         (!define_sum(g))           ||
         (!settle(g, f, &diag))     ||
         (!f.analyze(g, &diag))     )
    {
        return false;
    }

    const re_std::int32_t e = g.index_of("E");

    if ( (!f.left_recursive(e))                                          ||
         ((f.of_rule(e)->flags & D_PARSEGEN_FACT_DIRECT_LEFT) == 0u)     ||
         (f.left_recursive(g.index_of("T")))                             )
    {
        return false;
    }

    return ( (!diag.failed())                                           &&
             (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                         D_PARSEGEN_DIAG_ANALYSIS_LEFT_RECURSIVE))      &&
             (diag[0].level() == severity::note)                        &&
             (std::strstr(diag[0].text(), "'E'") != nullptr)            &&
             (f.derived().any(feature::left_recursion))                 &&
             (g.uses().any(feature::left_recursion))                    );
}

/*
d_tests_parsegen_analysis_left_indirect
  A cycle through another rule: A = B 'x' / 'a' ; B = A 'y' / 'b'.
  Tests the following:
  - both rules are found to be left-recursive
  - each has a way to finish, so the cycle is all there is to say: analysis
    succeeds and nothing is reported above a note
  - the witness -- the rule the cycle runs through -- is data in the facts
  - the note names it too, since naming it is the point of the note
*/
bool
d_tests_parsegen_analysis_left_indirect(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    scratch                       g;
    scratch_facts                 f;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t via_b = g.ordered({
        g.sequence({ g.ref("B"), g.literal("x") }),
        g.literal("a") });

    const re_std::int32_t via_a = g.ordered({
        g.sequence({ g.ref("A"), g.literal("y") }),
        g.literal("b") });

    if ( (!g.define("A", via_b))    ||
         (!g.define("B", via_a))    ||
         (!settle(g, f, &diag))     ||
         (!f.analyze(g, &diag))     )
    {
        return false;
    }

    const re_std::int32_t a = g.index_of("A");
    const re_std::int32_t b = g.index_of("B");

    return ( (f.left_recursive(a))                                      &&
             (f.left_recursive(b))                                      &&
             (f.of_rule(a)->left_witness == b)                          &&
             (!diag.failed())                                           &&
             (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                         D_PARSEGEN_DIAG_ANALYSIS_LEFT_RECURSIVE))      &&
             (std::strstr(diag[0].text(), "'B'") != nullptr)            );
}

/*
d_tests_parsegen_analysis_left_nullable_prefix
  Recursion behind a prefix that can match empty: N = ' '? N 'z' / 'n'.
  Tests the following:
  - it is still left recursion, which is the case a check of the leftmost
    child alone gets wrong
  - the rule has a way to finish, so analysis succeeds
*/
bool
d_tests_parsegen_analysis_left_nullable_prefix(void)
{
    scratch       g;
    scratch_facts f;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t body = g.ordered({
        g.sequence({ g.optional(g.literal(" ")),
                     g.ref("N"),
                     g.literal("z") }),
        g.literal("n") });

    return ( (g.define("N", body))                  &&
             (settle(g, f, nullptr))                &&
             (f.analyze(g, nullptr))                &&
             (f.left_recursive(g.index_of("N")))    );
}

/*
d_tests_parsegen_analysis_left_consuming_prefix
  Recursion behind a prefix that consumes: C = 'k' C / 'k'.
  Tests the following:
  - it is ordinary recursion, not left recursion
  - so nothing is added to what the grammar reports it uses
*/
bool
d_tests_parsegen_analysis_left_consuming_prefix(void)
{
    scratch       g;
    scratch_facts f;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t body = g.ordered({
        g.sequence({ g.literal("k"), g.ref("C") }),
        g.literal("k") });

    return ( (g.define("C", body))                          &&
             (settle(g, f, nullptr))                        &&
             (f.analyze(g, nullptr))                        &&
             (!f.left_recursive(g.index_of("C")))           &&
             (!g.uses().any(feature::left_recursion))       );
}

/*
d_tests_parsegen_analysis_unreachable
  A rule nothing leads to.
  Tests the following:
  - reachability follows references through nesting, not just top-level ones
  - an unreachable rule is a warning, not a failure: analysis succeeds and
    the grammar is still usable
  - the warning is identified by domain and code, and names the rule
*/
bool
d_tests_parsegen_analysis_unreachable(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    scratch                       g;
    scratch_facts                 f;

    if ( (!ready(g))                                                    ||
         (!g.define("Start", g.sequence({ g.literal("a"),
                                          g.ref("Used") })))            ||
         (!g.define("Used", g.klass(charset::of("0-9"))))               ||
         (!g.define("Orphan", g.literal("never")))                      ||
         (!settle(g, f, &diag))                                         ||
         (!f.analyze(g, &diag))                                         )
    {
        return false;
    }

    if ( (!f.reachable(g.index_of("Start")))   ||
         (!f.reachable(g.index_of("Used")))    ||
         (f.reachable(g.index_of("Orphan")))   )
    {
        return false;
    }

    return ( (!diag.failed())                                       &&
             (diag.tally_of(severity::warning) == 1u)               &&
             (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                         D_PARSEGEN_DIAG_ANALYSIS_UNREACHABLE))     &&
             (std::strstr(diag[0].text(), "Orphan") != nullptr)     );
}

/*
d_tests_parsegen_analysis_unproductive
  A rule that can never match.
  Tests the following:
  - it is an error, because no family could run it: analysis fails and the
    facts are left incomplete
  - the error is identified by domain and code, and names the rule
*/
bool
d_tests_parsegen_analysis_unproductive(void)
{
    fixed_diagnostics<32u, 2048u> diag;
    scratch                       g;
    scratch_facts                 f;
    const charset                 nothing;

    // an empty class accepts no symbol at all
    if ( (!ready(g))                            ||
         (!g.define("Void", g.klass(nothing)))  ||
         (!settle(g, f, &diag))                 )
    {
        return false;
    }

    return ( (!f.analyze(g, &diag))                                     &&
             (diag.failed())                                            &&
             (!f.complete())                                            &&
             (diag[0].is(D_PARSEGEN_DIAG_DOMAIN_ANALYSIS,
                         D_PARSEGEN_DIAG_ANALYSIS_UNPRODUCTIVE))        &&
             (std::strstr(diag[0].text(), "Void") != nullptr)           );
}

/*
d_tests_parsegen_analysis_disjoint
  Whether a choice can be decided on one symbol.
  Tests the following:
  - a choice whose alternatives start differently is disjoint, so it can be
    lowered or dispatched
  - one with an overlapping alternative is not, and the offender is named
  - one with a NULLABLE alternative is not, however tidy its first sets look
    -- the half a caller working from first sets alone would miss
*/
bool
d_tests_parsegen_analysis_disjoint(void)
{
    scratch         g;
    scratch_facts   f;
    re_std::int32_t conflict = no_node;

    if (!ready(g))
    {
        return false;
    }

    const re_std::int32_t clean = g.ordered({
        g.klass(charset::of("0-9")),
        g.klass(charset::of("a-z")) });

    const re_std::int32_t overlapping = g.ordered({
        g.klass(charset::of("0-9")),
        g.klass(charset::of("5-9A-F")) });

    const re_std::int32_t nullable_alt = g.ordered({
        g.star(g.klass(charset::of("0-9"))),
        g.klass(charset::of("a-z")) });

    if ( (!g.define("S", g.sequence({ clean, overlapping, nullable_alt })))  ||
         (!settle(g, f, nullptr))                                            ||
         (!f.analyze(g, nullptr))                                            )
    {
        return false;
    }

    return ( (f.disjoint(g, clean, &conflict))              &&
             (!f.disjoint(g, overlapping, &conflict))       &&
             (conflict != no_node)                          &&
             (!f.disjoint(g, nullable_alt, &conflict))      );
}

/*
d_tests_parsegen_analysis_routing
  The loop that grammar, analysis and registry close between them.
  Tests the following:
  - before analysis a left-recursive grammar reports only its syntax, and the
    PEG family looks fine for it
  - analysis discovers what the syntax did not show
  - the very same query then routes to the LR family, with nothing in between
    having been told that either family exists
*/
bool
d_tests_parsegen_analysis_routing(void)
{
    fixed_registry<4u> reg;
    scratch            g;
    scratch_facts      f;
    stage              peg = {};
    stage              lr  = {};

    peg.name    = "peg";
    peg.kind    = static_cast<re_std::uint32_t>(stage_kind::family);
    peg.rejects = feature::left_recursion | feature::unordered_choice;

    // a single capability needs the set spelling: `feature` is a scoped enum
    // precisely so that it cannot drift into an integer field by accident
    lr.name     = "lr";
    lr.kind     = static_cast<re_std::uint32_t>(stage_kind::family);
    lr.provides = feature_set(feature::left_recursion);
    lr.rejects  = feature_set(feature::syntactic_predicate);

    if ( (!ready(g))                ||
         (!define_sum(g))           ||
         (!g.resolve(nullptr))      ||
         (!reg.add(peg))            ||
         (!reg.add(lr))             )
    {
        return false;
    }

    const stage* const before = reg.select(stage_kind::family,
                                           g.uses(),
                                           nullptr);

    if ( (!before)                                  ||
         (std::strcmp(before->name, "peg") != 0)    ||
         (!sized(f, g))                             ||
         (!f.analyze(g, nullptr))                   )
    {
        return false;
    }

    const stage* const after = reg.select(stage_kind::family,
                                          g.uses(),
                                          nullptr);

    return ( (after != nullptr)                      &&
             (std::strcmp(after->name, "lr") == 0)   );
}

}  // namespace

// d_tests_parsegen_analysis
//   constant: the suite -- every section this build has, the facts about a
// node before the facts about a rule, and those before what is routed on them.
const d_tests_section d_tests_parsegen_analysis[] =
{
    { "parsegen_analysis_terminals",
      &d_tests_parsegen_analysis_terminals },
    { "parsegen_analysis_repetition",
      &d_tests_parsegen_analysis_repetition },
    { "parsegen_analysis_sequence",
      &d_tests_parsegen_analysis_sequence },
    { "parsegen_analysis_predicate",
      &d_tests_parsegen_analysis_predicate },
    { "parsegen_analysis_choice",
      &d_tests_parsegen_analysis_choice },
    { "parsegen_analysis_left_direct",
      &d_tests_parsegen_analysis_left_direct },
    { "parsegen_analysis_left_indirect",
      &d_tests_parsegen_analysis_left_indirect },
    { "parsegen_analysis_left_nullable_prefix",
      &d_tests_parsegen_analysis_left_nullable_prefix },
    { "parsegen_analysis_left_consuming_prefix",
      &d_tests_parsegen_analysis_left_consuming_prefix },
    { "parsegen_analysis_unreachable",
      &d_tests_parsegen_analysis_unreachable },
    { "parsegen_analysis_unproductive",
      &d_tests_parsegen_analysis_unproductive },
    { "parsegen_analysis_disjoint",
      &d_tests_parsegen_analysis_disjoint },
    { "parsegen_analysis_routing",
      &d_tests_parsegen_analysis_routing }
};

// d_tests_parsegen_analysis_count
//   constant: how many sections the table above holds in this build.
const std::size_t d_tests_parsegen_analysis_count =
    sizeof(d_tests_parsegen_analysis) / sizeof(d_tests_parsegen_analysis[0]);

#endif  // floor, for now
