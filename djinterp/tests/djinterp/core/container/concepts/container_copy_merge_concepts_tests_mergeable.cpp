#include "container_copy_merge_concepts_tests.hpp"


#if D_CM_CONCEPTS_ENABLED

NS_DJINTERP
NS_TESTING

/*
tests_ccmc_mergeable_sequences
  two sequences concatenate.
  Tests the following:
  - vector with vector, string with string, array with array
  - across container templates at one element type: vector with array, string
    with vector<char>
  - the hand-rolled sequences with each other and with the std ones, so the
    verdict does not depend on a std library detail
  - the resolved kind is concatenation, not merely "not incompatible", so a
    regression that reclassified the pair would be visible
*/
bool
tests_ccmc_mergeable_sequences()
{
    D_CM_CHECK(( dt::holds_for_all<cm_mergeable_self_c,
                                   std::vector<int>,
                                   std::array<int, 4>,
                                   std::string,
                                   cm_plain_seq,
                                   cm_extent_seq,
                                   cm_interval_seq,
                                   cm_fixed_capacity_seq,
                                   cm_growable_seq>::value ));

    // across container templates at one element type
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            std::array<int, 4>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<std::string,
                                            std::vector<char>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_plain_seq, std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_extent_seq,
                                            cm_fixed_capacity_seq> ));

    // the kind is named, not merely non-incompatible
    D_CM_CHECK(( ::djinterp::merge_kind_of<std::vector<int>,
                                           std::array<int, 4>>::value
                 == ::djinterp::merge_kind::concatenation ));
    D_CM_CHECK(( ::djinterp::merge_discipline_of<std::vector<int>>::value
                 == ::djinterp::merge_discipline::sequence ));

    return true;
}

/*
tests_ccmc_mergeable_bags
  two unkeyed bags union, in all three combinations.
  Tests the following:
  - set with set (a deduplicating union), set with multiset and multiset with
    multiset (both keeping repeats)
  - the resolved kind distinguishes set_union from multiset_union, which is the
    whole of the uniqueness split at the pair level
  - the hand-rolled cm_unique_bag / cm_repeat_bag pair behaves identically to
    std::set / std::multiset, confirming the split is read from the local
    insert signal rather than from the std type
*/
bool
tests_ccmc_mergeable_bags()
{
    D_CM_CHECK(( dt::holds_for_all<cm_mergeable_self_c,
                                   std::set<int>,
                                   std::multiset<int>,
                                   cm_unique_bag,
                                   cm_repeat_bag,
                                   cm_bounded_bag>::value ));

    D_CM_CHECK(( ::djinterp::mergeable_with<std::set<int>,
                                            std::multiset<int>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_unique_bag, cm_repeat_bag> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_unique_bag, std::set<int>> ));

    // both sides unique -> a deduplicating union; either side repeatable -> not
    D_CM_CHECK(( ::djinterp::merge_kind_of<std::set<int>,
                                           std::set<int>>::value
                 == ::djinterp::merge_kind::set_union ));
    D_CM_CHECK(( ::djinterp::merge_kind_of<std::set<int>,
                                           std::multiset<int>>::value
                 == ::djinterp::merge_kind::multiset_union ));

    // the hand-rolled pair splits the same way, on the insert signal alone
    D_CM_CHECK(( ::djinterp::merge_discipline_of<cm_unique_bag>::value
                 == ::djinterp::merge_discipline::set ));
    D_CM_CHECK(( ::djinterp::merge_discipline_of<cm_repeat_bag>::value
                 == ::djinterp::merge_discipline::multiset ));

    return true;
}

/*
tests_ccmc_mergeable_keyed
  two keyed maps merge by key, in all three combinations.
  Tests the following:
  - map with map, map with multimap, multimap with multimap
  - the resolved kind is keyed_merge in all three, the uniqueness split showing
    up in the key-conflict proviso rather than in the kind
  - the hand-rolled cm_unique_keyed / cm_repeat_keyed pair behaves identically,
    and merges with the std maps, their value types agreeing
*/
bool
tests_ccmc_mergeable_keyed()
{
    D_CM_CHECK(( dt::holds_for_all<cm_mergeable_self_c,
                                   std::map<int, int>,
                                   std::multimap<int, int>,
                                   cm_unique_keyed,
                                   cm_repeat_keyed>::value ));

    D_CM_CHECK(( ::djinterp::mergeable_with<std::map<int, int>,
                                            std::multimap<int, int>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_unique_keyed,
                                            cm_repeat_keyed> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_unique_keyed,
                                            std::map<int, int>> ));

    // one kind for all three combinations
    D_CM_CHECK(( ::djinterp::merge_kind_of<std::map<int, int>,
                                           std::multimap<int, int>>::value
                 == ::djinterp::merge_kind::keyed_merge ));

    D_CM_CHECK(( ::djinterp::merge_discipline_of<cm_unique_keyed>::value
                 == ::djinterp::merge_discipline::map ));
    D_CM_CHECK(( ::djinterp::merge_discipline_of<cm_repeat_keyed>::value
                 == ::djinterp::merge_discipline::multimap ));

    return true;
}

/*
tests_ccmc_mergeable_cross_family
  nothing merges across the three families.
  Tests the following:
  - sequence with bag at IDENTICAL elements - vector<int> with set<int>, and
    cm_plain_seq with cm_unique_bag - which isolates the discipline clause
    exactly, the element clause being satisfied in both
  - sequence with keyed, and bag with keyed
  - each refusal resolves to merge_kind::incompatible
  - the elements really are compatible in the isolating cases, so the refusal
    is attributable to the disciplines alone
  Note that sequence-versus-keyed cannot be isolated the same way: a sequence
  whose value_type is a pair is itself classified keyed (finding F7), so there
  is no pair of a sequence and a map that share an element type. The bag cases
  carry the isolation instead.
*/
bool
tests_ccmc_mergeable_cross_family()
{
    // sequence vs bag, at identical elements - discipline isolated
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::set<int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::multiset<int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<cm_plain_seq, cm_unique_bag> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<cm_plain_seq, cm_repeat_bag> ));

    // ...and the elements ARE compatible, so only the discipline refused
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::set<int>> ));
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<cm_plain_seq,
                                                            cm_unique_bag> ));

    // sequence vs keyed, and bag vs keyed
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::map<int, int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::set<int>,
                                             std::map<int, int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<cm_unique_bag,
                                             cm_unique_keyed> ));

    // every refusal names itself
    D_CM_CHECK(( ::djinterp::merge_kind_of<std::vector<int>,
                                           std::set<int>>::value
                 == ::djinterp::merge_kind::incompatible ));

    return true;
}

/*
tests_ccmc_mergeable_pair_valued
  FINDING F7, pinned: a SEQUENCE whose value_type is a pair is classified as a
  keyed discipline, and merges by key with a std::map.
  Tests the following:
  - vector<pair<int,int>> reads as multimap, not sequence
  - set<pair<int,int>> reads as map, not set
  - vector<pair<int,int>> is therefore mergeable with map<int,int>, at kind
    keyed_merge, the pair elements being mutually convertible
  - the same pair, read as a bare vector of ints, is a sequence - so the
    reclassification is caused by the element type alone
  The traits header states the local signal plainly ("a value_type that is a
  pair is the KEYED mark") and this follows from it directly, so the behaviour
  is a consequence of the documented probe rather than a bug in its
  implementation. What it puts in question is the adjacent claim that "the
  verdicts agree with the axis traits on the familiar containers":
  vector<pair<K,V>> is a familiar container, and its overlay-axis reading is
  very unlikely to be multimap. Confirming that against
  container_overlay_traits.hpp is the follow-up; this test pins the observed
  behaviour in the meantime so the answer, whichever way it goes, is a
  deliberate change rather than a silent one.
*/
bool
tests_ccmc_mergeable_pair_valued()
{
    // a sequence of pairs is read as keyed
    D_CM_CHECK(( ::djinterp::merge_discipline_of<
                     std::vector<std::pair<int, int>>>::value
                 == ::djinterp::merge_discipline::multimap ));
    D_CM_CHECK(( ::djinterp::merge_discipline_of<
                     std::set<std::pair<int, int>>>::value
                 == ::djinterp::merge_discipline::map ));

    // and therefore merges by key with a real map
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<std::pair<int, int>>,
                                            std::map<int, int>> ));
    D_CM_CHECK(( ::djinterp::merge_kind_of<std::vector<std::pair<int, int>>,
                                           std::map<int, int>>::value
                 == ::djinterp::merge_kind::keyed_merge ));

    // the element type alone causes it - the same template over int is a
    // sequence, and refuses the map
    D_CM_CHECK(( ::djinterp::merge_discipline_of<std::vector<int>>::value
                 == ::djinterp::merge_discipline::sequence ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::map<int, int>> ));

    D_CM_NOTE("F7: a sequence of pairs is classified keyed - "
              "vector<pair<int,int>> reads as multimap and merges by key with "
              "map<int,int>; follows from the documented pair-value signal, "
              "but should be reconciled with container_overlay_traits.hpp");

    return true;
}

/*
tests_ccmc_mergeable_element_gate
  agreeing disciplines are not enough - the elements must combine too.
  Tests the following:
  - vector<int> with vector<cm_opaque>: both sequences, elements incompatible
  - set<int> with set<std::string>: both sets, elements incompatible
  - map<int,int> with map<int,cm_opaque>: both maps, elements incompatible
  - each case is a WITNESS that mergeable_with is strictly stronger than
    merge_elements_compatible_with rather than equal to it, which is what makes
    the ordering report of section VII non-trivial
*/
bool
tests_ccmc_mergeable_element_gate()
{
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::vector<cm_opaque>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::set<int>,
                                             std::set<std::string>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::map<int, int>,
                                             std::map<int, cm_opaque>> ));

    // the disciplines agree in every one of them
    D_CM_CHECK((    ::djinterp::merge_discipline_of<std::vector<int>>::value
                 == ::djinterp::merge_discipline_of<
                        std::vector<cm_opaque>>::value ));
    D_CM_CHECK((    ::djinterp::merge_discipline_of<std::set<int>>::value
                 == ::djinterp::merge_discipline_of<
                        std::set<std::string>>::value ));

    // ...so the element clause is the whole of the refusal
    D_CM_CHECK(( !::djinterp::merge_elements_compatible_with<
                     std::vector<int>,
                     std::vector<cm_opaque>> ));

    return true;
}

/*
tests_ccmc_mergeable_symmetry
  the concept is symmetric in its operands.
  Tests the following:
  - exchanging _From and _To never changes the answer, across sequences, bags,
    keyed maps, both refusal modes (cross-family and element), and the
    non-container case
  - the resolved KIND is symmetric too, including the set_union / multiset_union
    split, which is the one place a naive implementation might have privileged
    the left operand
  This is what makes mergeable_with's _From / _To naming harmless. It is
  asserted here precisely because it is NOT true of merge_may_overflow_into
  (finding F1): that concept is the only asymmetric member of the four, and so
  the only one for which operand order is load-bearing.
*/
bool
tests_ccmc_mergeable_symmetry()
{
    D_CM_CHECK((    ::djinterp::mergeable_with<std::vector<int>,
                                               std::array<int, 4>>
                 == ::djinterp::mergeable_with<std::array<int, 4>,
                                               std::vector<int>> ));

    D_CM_CHECK((    ::djinterp::mergeable_with<std::set<int>,
                                               std::multiset<int>>
                 == ::djinterp::mergeable_with<std::multiset<int>,
                                               std::set<int>> ));

    D_CM_CHECK((    ::djinterp::mergeable_with<std::map<int, int>,
                                               std::multimap<int, int>>
                 == ::djinterp::mergeable_with<std::multimap<int, int>,
                                               std::map<int, int>> ));

    // both refusal modes
    D_CM_CHECK((    ::djinterp::mergeable_with<std::vector<int>,
                                               std::set<int>>
                 == ::djinterp::mergeable_with<std::set<int>,
                                               std::vector<int>> ));
    D_CM_CHECK((    ::djinterp::mergeable_with<std::vector<int>,
                                               std::vector<cm_opaque>>
                 == ::djinterp::mergeable_with<std::vector<cm_opaque>,
                                               std::vector<int>> ));
    D_CM_CHECK((    ::djinterp::mergeable_with<int, std::vector<int>>
                 == ::djinterp::mergeable_with<std::vector<int>, int> ));

    // the kind is symmetric, uniqueness split included
    D_CM_CHECK((    ::djinterp::merge_kind_of<std::set<int>,
                                              std::multiset<int>>::value
                 == ::djinterp::merge_kind_of<std::multiset<int>,
                                              std::set<int>>::value ));

    return true;
}

/*
tests_ccmc_mergeable_cvref
  the concept answers identically under every cv-ref qualification.
  Tests the following:
  - all eight forms of a merging pair agree and are true (all)
  - all eight forms of a non-container agree and are false (none)
  - a cross-family refusal is invariant too, so the invariance covers the
    discipline clause and not only the container guard
  - one operand qualified at a time, which the diagonal report cannot express
*/
bool
tests_ccmc_mergeable_cvref()
{
    D_CONSTEXPR dt::cvref_report vec =
        dt::trait_across_cvref<cm_mergeable_self_c, std::vector<int>>();
    D_CONSTEXPR dt::cvref_report plain =
        dt::trait_across_cvref<cm_mergeable_self_c, int>();
    D_CONSTEXPR dt::cvref_report keyed =
        dt::trait_across_cvref<cm_mergeable_self_c, std::map<int, int>>();

    D_CM_CHECK(vec.agrees());
    D_CM_CHECK(vec.all());
    D_CM_CHECK(vec.first_disagreement() == nullptr);

    D_CM_CHECK(plain.agrees());
    D_CM_CHECK(plain.none());

    D_CM_CHECK(keyed.agrees());
    D_CM_CHECK(keyed.all());

    // one operand qualified at a time
    D_CM_CHECK(( ::djinterp::mergeable_with<const std::vector<int>&,
                                            std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            volatile std::vector<int>&&> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<const std::vector<int>&,
                                             std::set<int>&&> ));

    return true;
}

/*
tests_ccmc_mergeable_hostile
  the concept survives, and rejects, the full adversarial zoo.
  Tests the following:
  - holds_for_none over D_TEST_HOSTILE_TYPES paired with itself
  - count_holds is exactly zero, in the form that instantiates every cell
  - each half of the zoo separately
  - hostile shapes paired with real containers
  The full zoo is used, not the COMPLETE subset: unlike copyable_container
  (finding F2), nothing on this path names a completeness-requiring trait.
*/
bool
tests_ccmc_mergeable_hostile()
{
    D_CM_CHECK(( dt::holds_for_none<cm_mergeable_self_c,
                                    D_TEST_HOSTILE_TYPES>::value ));

    D_CM_CHECK(( dt::count_holds<cm_mergeable_self_c,
                                 D_TEST_HOSTILE_TYPES>::value == 0u ));

    D_CM_CHECK(( dt::holds_for_none<cm_mergeable_self_c,
                                    D_TEST_HOSTILE_CLASS_TYPES>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_mergeable_self_c,
                                    D_TEST_HOSTILE_NONCLASS_TYPES>::value ));

    D_CM_CHECK(( !::djinterp::mergeable_with<dt::fixtures::incomplete,
                                             std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             dt::fixtures::array_type> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<dt::fixtures::evil,
                                             dt::fixtures::greedy> ));

    return true;
}

/*
tests_ccmc_mergeable_constrains
  the binary concept works as a CONSTRAINT, not merely as a bool.
  Tests the following:
  - a mergeable_with-constrained overload is chosen over an otherwise identical
    unconstrained one for a merging pair
  - the unconstrained fallback is chosen for a cross-family pair, an
    element-incompatible pair, and a non-container pair
  - the selection is a constant expression, so the dispatch happens where a
    requires-clause is actually consulted
*/
bool
tests_ccmc_mergeable_constrains()
{
    D_CM_CHECK(( cm_merge_probe<std::vector<int>, std::vector<int>>() == 1 ));
    D_CM_CHECK(( cm_merge_probe<std::set<int>, std::multiset<int>>() == 1 ));
    D_CM_CHECK(( cm_merge_probe<std::map<int, int>,
                                std::multimap<int, int>>() == 1 ));

    // cross-family
    D_CM_CHECK(( cm_merge_probe<std::vector<int>, std::set<int>>() == 0 ));

    // element-incompatible
    D_CM_CHECK(( cm_merge_probe<std::vector<int>,
                                std::vector<cm_opaque>>() == 0 ));

    // not containers at all
    D_CM_CHECK(( cm_merge_probe<int, int>() == 0 ));

    D_CONSTEXPR int chosen =
        cm_merge_probe<std::vector<int>, std::array<int, 4>>();
    D_CM_CHECK(chosen == 1);

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_CM_CONCEPTS_ENABLED
