#include "container_copy_merge_concepts_tests.hpp"


#if D_CM_CONCEPTS_ENABLED

NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   test_concept.hpp section III offers `unordered` in both polarities: assert
// it FALSE for a healthy refinement ladder rung, TRUE to pin two concepts as
// siblings that must never overload against each other.  This module has no
// ladder (finding F4), so the second is the applicable form, and it is pinned at
// the build rather than reported: the moment one of these concepts is respelled
// in terms of another, every downstream statement about which overload wins
// changes meaning, and a report from that state would be a report from code
// whose call sites no longer resolve the way the suite assumes.

D_TEST_STATIC(cm_order_merge_elem<std::vector<int>>::unordered);

D_TEST_STATIC(cm_order_over_merge<std::array<int, 4>>::unordered);


/*
tests_ccmc_order_implication
  the satisfaction chain holds: overflow implies mergeable implies
  element-compatible.
  Tests the following:
  - the chain at every type in a battery spanning the bounded shapes, all six
    disciplines, the non-containers, and the full hostile zoo
  - run through holds_for_all, so every cell instantiates and none is skipped
    by a short circuit
  - the chain is STRICT at each rung - a type satisfying the weaker and not the
    stronger is exhibited for both - so the implications are not the vacuous
    implications of coincident predicates
  Both implications hold by construction on the trait side: merge_may_overflow
  tests is_mergeable before its capacity clause, and merge_kind_of tests
  merge_elements_compatible before reading the disciplines. This is the
  statement that the concept face inherits them.
*/
bool
tests_ccmc_order_implication()
{
    // the chain, over the containers
    D_CM_CHECK(( dt::holds_for_all<cm_implication_holds,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::multiset<int>,
                                   std::map<int, int>,
                                   std::multimap<int, int>,
                                   std::array<int, 4>,
                                   std::string,
                                   cm_plain_seq,
                                   cm_extent_seq,
                                   cm_interval_seq,
                                   cm_fixed_capacity_seq,
                                   cm_growable_seq,
                                   cm_bounded_bag,
                                   cm_elementless_range,
                                   cm_positional,
                                   cm_opaque,
                                   int>::value ));

    // and over the zoo, where it holds vacuously but must still COMPILE
    D_CM_CHECK(( dt::holds_for_all<cm_implication_holds,
                                   D_TEST_HOSTILE_TYPES>::value ));

    // each rung is strict - the weaker admits something the stronger does not
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::vector<int>> ));

    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::set<int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::set<int>> ));

    return true;
}

/*
tests_ccmc_order_mergeable_vs_elements
  FINDING F4: mergeable_with does NOT subsume merge_elements_compatible_with,
  despite implying it.
  Tests the following:
  - at std::vector<int>, which satisfies both, the ordering report reads
    unordered: two overloads constrained on these two are AMBIGUOUS
  - both direction bools are false, and `both` is true, so the reading is taken
    at a genuine common witness and is not an unopposed win
  - the same reading at three further common witnesses spanning the
    disciplines, so the result is not an artefact of one type
  - the two are CO-EXTENSIVE on the diagonal - mergeable_with<T,T> and
    merge_elements_compatible_with<T,T> agree for every T - which is why the
    observation code cannot separate "equivalent" from "incomparable" here; and
    they differ OFF the diagonal, which is what makes them two concepts rather
    than one
  Implication is a fact about satisfaction; subsumption is a fact about the
  constraint's SPELLING. Each of these concepts is spelled as a fresh atomic
  constraint - a `::value` expression - rather than in terms of the other, so
  the compiler has no ordering to consult, and only the second of the two facts
  is what overload resolution uses.
*/
bool
tests_ccmc_order_mergeable_vs_elements()
{
    // the common witness, and the verdict
    D_CM_CHECK(cm_order_merge_elem<std::vector<int>>::both);
    D_CM_CHECK(cm_order_merge_elem<std::vector<int>>::unordered);
    D_CM_CHECK(!cm_order_merge_elem<std::vector<int>>::a_subsumes_b);
    D_CM_CHECK(!cm_order_merge_elem<std::vector<int>>::b_subsumes_a);
    D_CM_CHECK(!cm_order_merge_elem<std::vector<int>>::neither);
    D_CM_CHECK(cm_order_merge_elem<std::vector<int>>::code == 0);

    // not an artefact of one witness
    D_CM_CHECK(cm_order_merge_elem<std::set<int>>::unordered);
    D_CM_CHECK(cm_order_merge_elem<std::map<int, int>>::unordered);
    D_CM_CHECK(cm_order_merge_elem<cm_extent_seq>::unordered);

    // co-extensive on the diagonal: for T merged with itself, the discipline
    // clause is trivially satisfied, so the two predicates coincide
    D_CM_CHECK(( dt::holds_for_all<cm_diagonal_coextensive,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::multimap<int, int>,
                                   std::array<int, 4>,
                                   cm_elementless_range,
                                   cm_positional,
                                   int>::value ));
    D_CM_CHECK(( dt::holds_for_all<cm_diagonal_coextensive,
                                   D_TEST_HOSTILE_TYPES>::value ));

    // ...and distinct off it, which is what makes them two concepts
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::set<int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::set<int>> ));

    D_CM_NOTE("F4: mergeable_with and merge_elements_compatible_with are "
              "unordered - two overloads constrained on them are ambiguous "
              "for every type satisfying both");

    return true;
}

/*
tests_ccmc_order_overflow_vs_mergeable
  the same result one rung up: merge_may_overflow_into does not subsume
  mergeable_with either.
  Tests the following:
  - at std::array<int,4>, which satisfies both, the report reads unordered with
    both direction bools false
  - the same at the hand-rolled bounded shapes
  - here the extension is STRICTLY nested rather than co-extensive - every
    overflowing pair is mergeable and std::vector<int> is mergeable without
    overflowing - so this is the cleanest statement of the finding: a properly
    nested pair of predicates, and still no subsumption between them
  - the `both` gate is shown to work: at std::vector<int>, which satisfies
    mergeable_with and not merge_may_overflow_into, the B-constrained overload
    wins UNOPPOSED (code 2), and b_subsumes_a is nonetheless false, because a
    win at a witness satisfying only one concept observes no ordering at all
*/
bool
tests_ccmc_order_overflow_vs_mergeable()
{
    // the common witness
    D_CM_CHECK(cm_order_over_merge<std::array<int, 4>>::both);
    D_CM_CHECK(cm_order_over_merge<std::array<int, 4>>::unordered);
    D_CM_CHECK(!cm_order_over_merge<std::array<int, 4>>::a_subsumes_b);
    D_CM_CHECK(!cm_order_over_merge<std::array<int, 4>>::b_subsumes_a);
    D_CM_CHECK(cm_order_over_merge<std::array<int, 4>>::code == 0);

    D_CM_CHECK(cm_order_over_merge<cm_extent_seq>::unordered);
    D_CM_CHECK(cm_order_over_merge<cm_fixed_capacity_seq>::unordered);
    D_CM_CHECK(cm_order_over_merge<cm_bounded_bag>::unordered);

    // strictly nested extension
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                     std::array<int, 4>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<std::array<int, 4>,
                                            std::array<int, 4>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::vector<int>> ));

    // the `both` gate: an UNOPPOSED win is not a subsumption result
    D_CM_CHECK(!cm_order_over_merge<std::vector<int>>::both);
    D_CM_CHECK(cm_order_over_merge<std::vector<int>>::code == 2);
    D_CM_CHECK(!cm_order_over_merge<std::vector<int>>::b_subsumes_a);
    D_CM_CHECK(!cm_order_over_merge<std::vector<int>>::a_subsumes_b);
    D_CM_CHECK(!cm_order_over_merge<std::vector<int>>::unordered);
    D_CM_CHECK(!cm_order_over_merge<std::vector<int>>::neither);

    return true;
}

/*
tests_ccmc_order_restatement_trap
  WHY the two results above hold, on this module's own terms.
  Tests the following:
  - cm_mergeable_restated - a constraint spelled by reaching past
    mergeable_with to the trait it forwards to - is true of exactly the same
    types as mergeable_with, checked across the containers and the whole zoo
  - and is nonetheless UNORDERED with it, at every common witness
  - so identical satisfaction, and even a textually equivalent constraint
    expression, buys nothing in the partial order
  Atomic constraints are identified by their source expression and their
  parameter mapping, not by what they compute. Any respelling of a concept,
  however faithful, is a NEW atomic constraint and forfeits its position in the
  order. This is the trap test_concept.hpp section V's restates_a fixture
  exists to demonstrate, reproduced here on the module actually under test - and
  it is the reason a refinement ladder must be built by NAMING the weaker
  concept rather than by copying its requirements.
*/
bool
tests_ccmc_order_restatement_trap()
{
    // identical satisfaction, everywhere
    D_CM_CHECK(( dt::holds_for_all<cm_restatement_faithful,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::map<int, int>,
                                   std::array<int, 4>,
                                   cm_extent_seq,
                                   cm_elementless_range,
                                   cm_positional,
                                   int>::value ));
    D_CM_CHECK(( dt::holds_for_all<cm_restatement_faithful,
                                   D_TEST_HOSTILE_TYPES>::value ));

    // and still unordered with the concept it restates
    D_CM_CHECK(cm_order_restatement<std::vector<int>>::both);
    D_CM_CHECK(cm_order_restatement<std::vector<int>>::unordered);
    D_CM_CHECK(!cm_order_restatement<std::vector<int>>::a_subsumes_b);
    D_CM_CHECK(!cm_order_restatement<std::vector<int>>::b_subsumes_a);
    D_CM_CHECK(cm_order_restatement<std::vector<int>>::code == 0);

    D_CM_CHECK(cm_order_restatement<std::map<int, int>>::unordered);
    D_CM_CHECK(cm_order_restatement<cm_extent_seq>::unordered);

    D_CM_NOTE("a faithful restatement of a concept is a fresh atomic "
              "constraint and is unordered with the original - a ladder must "
              "NAME the weaker concept, never copy its requirements");

    return true;
}

/*
tests_ccmc_order_neither_witness
  the report is honest about a witness at which nothing is observed.
  Tests the following:
  - at int, which satisfies neither concept of either pair, `neither` is true
    and `both` is false
  - all three of a_subsumes_b, b_subsumes_a and unordered are false, so an
    absence of observation is never reported as an ordering
  - the raw code is 3, the unconstrained fallback having been selected because
    no constrained candidate was viable
  - the same at a non-container that looks like one (cm_positional) and at a
    hostile fixture
  Together with the unopposed-win case in tests_ccmc_order_overflow_vs_mergeable,
  this closes both ways the ordering report could otherwise mislead: a witness
  satisfying one concept, and a witness satisfying none.
*/
bool
tests_ccmc_order_neither_witness()
{
    // neither concept satisfied
    D_CM_CHECK(cm_order_merge_elem<int>::neither);
    D_CM_CHECK(!cm_order_merge_elem<int>::both);
    D_CM_CHECK(!cm_order_merge_elem<int>::a_subsumes_b);
    D_CM_CHECK(!cm_order_merge_elem<int>::b_subsumes_a);
    D_CM_CHECK(!cm_order_merge_elem<int>::unordered);
    D_CM_CHECK(cm_order_merge_elem<int>::code == 3);

    D_CM_CHECK(cm_order_over_merge<int>::neither);
    D_CM_CHECK(!cm_order_over_merge<int>::both);
    D_CM_CHECK(cm_order_over_merge<int>::code == 3);

    // a type that looks like a container and is not
    D_CM_CHECK(cm_order_merge_elem<cm_positional>::neither);
    D_CM_CHECK(!cm_order_merge_elem<cm_positional>::unordered);

    // and a hostile fixture
    D_CM_CHECK(cm_order_merge_elem<dt::fixtures::greedy>::neither);
    D_CM_CHECK(cm_order_over_merge<dt::fixtures::evil>::neither);

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_CM_CONCEPTS_ENABLED
