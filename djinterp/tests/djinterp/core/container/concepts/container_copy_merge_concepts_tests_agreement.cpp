#include "container_copy_merge_concepts_tests.hpp"


#if D_CM_CONCEPTS_ENABLED

NS_DJINTERP
NS_TESTING

///////////////////////////////////////////////////////////////////////////////
///                BUILD-TIME PINS                                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   The framework's default is a REPORTED failure, and every check below is one.
// These four are the exception test_concept.hpp section VI reserves D_TEST_STATIC
// for: that a trait and its concept face are still the SAME predicate on the
// types they are most likely to have drifted on.  A stale concept face silently
// inverts overload resolution at every site that prefers the concept, and no
// downstream test of either face ALONE catches it - so this regression should
// stop the line rather than colour a report line red.
//
//   The copy pin runs over the COMPLETE list; the merge pins over the full zoo.
// See finding F2 for why the two lists differ.

D_TEST_STATIC(dt::holds_for_all<cm_copyable_agree,
                                D_TEST_HOSTILE_TYPES_COMPLETE>::value);

D_TEST_STATIC(dt::holds_for_all<cm_elements_agree_self,
                                D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(dt::holds_for_all<cm_mergeable_agree_self,
                                D_TEST_HOSTILE_TYPES>::value);

D_TEST_STATIC(dt::holds_for_all<cm_overflow_agree_self,
                                D_TEST_HOSTILE_TYPES>::value);


/*
tests_ccmc_agree_copyable
  is_copyable_container and copyable_container are the same predicate.
  Tests the following:
  - agreement at each container fixture and each refuting type, spelled through
    the single-point primitive so the report names the type
  - agreement across a battery of containers, and across the refusals
  - agreement across D_TEST_HOSTILE_TYPES_COMPLETE, which is where a trait and
    its concept actually drift: the type nobody re-checked
  The completeness restriction of finding F2 applies to the agreement trait
  exactly as it does to the concept, since the trait is one of its two operands.
*/
bool
tests_ccmc_agree_copyable()
{
    // at a point
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_copyable_container,
                                          ::djinterp::copyable_container,
                                          std::vector<int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_copyable_container,
                                          ::djinterp::copyable_container,
                                          cm_noncopyable_seq));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_copyable_container,
                                          ::djinterp::copyable_container,
                                          int));

    // across a battery of both polarities
    D_CM_CHECK(( dt::holds_for_all<cm_copyable_agree,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::map<int, int>,
                                   std::array<int, 4>,
                                   std::string,
                                   cm_plain_seq,
                                   cm_elementless_range,
                                   cm_noncopyable_seq,
                                   cm_positional,
                                   cm_opaque,
                                   int>::value ));

    // and across the zoo, where drift actually happens
    D_CM_CHECK(( dt::holds_for_all<cm_copyable_agree,
                                   D_TEST_HOSTILE_TYPES_COMPLETE>::value ));

    return true;
}

/*
tests_ccmc_agree_elements
  merge_elements_compatible and merge_elements_compatible_with are the same
  predicate.
  Tests the following:
  - agreement at the compatible, convertible, incompatible, non-container and
    elementless cases, spelled at a point
  - agreement across the diagonal battery and the full hostile zoo
  - agreement in the asymmetric cases too, where the two operands differ
*/
bool
tests_ccmc_agree_elements()
{
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(
                   ::djinterp::merge_elements_compatible,
                   ::djinterp::merge_elements_compatible_with,
                   std::vector<int>, std::set<int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(
                   ::djinterp::merge_elements_compatible,
                   ::djinterp::merge_elements_compatible_with,
                   std::vector<int>, std::vector<cm_from_int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(
                   ::djinterp::merge_elements_compatible,
                   ::djinterp::merge_elements_compatible_with,
                   std::vector<int>, std::vector<cm_opaque>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(
                   ::djinterp::merge_elements_compatible,
                   ::djinterp::merge_elements_compatible_with,
                   int, int));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(
                   ::djinterp::merge_elements_compatible,
                   ::djinterp::merge_elements_compatible_with,
                   cm_elementless_range, cm_elementless_range));

    D_CM_CHECK(( dt::holds_for_all<cm_elements_agree_self,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::map<int, int>,
                                   cm_elementless_range,
                                   cm_positional,
                                   int>::value ));

    D_CM_CHECK(( dt::holds_for_all<cm_elements_agree_self,
                                   D_TEST_HOSTILE_TYPES>::value ));

    return true;
}

/*
tests_ccmc_agree_mergeable
  is_mergeable and mergeable_with are the same predicate.
  Tests the following:
  - agreement across all six disciplines on the diagonal
  - agreement at every cross-family refusal and at the element refusals
  - agreement across the full hostile zoo
  - agreement at the pair-valued sequence of finding F7, so the two faces share
    that classification rather than differing over it
*/
bool
tests_ccmc_agree_mergeable()
{
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_mergeable,
                                          ::djinterp::mergeable_with,
                                          std::vector<int>, std::vector<int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_mergeable,
                                          ::djinterp::mergeable_with,
                                          std::set<int>, std::multiset<int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_mergeable,
                                          ::djinterp::mergeable_with,
                                          std::vector<int>, std::set<int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::is_mergeable,
                                          ::djinterp::mergeable_with,
                                          std::vector<int>,
                                          std::vector<cm_opaque>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(
                   ::djinterp::is_mergeable,
                   ::djinterp::mergeable_with,
                   std::vector<std::pair<int, int>>, std::map<int, int>));

    D_CM_CHECK(( dt::holds_for_all<cm_mergeable_agree_self,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::multiset<int>,
                                   std::map<int, int>,
                                   std::multimap<int, int>,
                                   std::array<int, 4>,
                                   cm_unique_bag,
                                   cm_repeat_keyed,
                                   cm_elementless_range,
                                   int>::value ));

    D_CM_CHECK(( dt::holds_for_all<cm_mergeable_agree_self,
                                   D_TEST_HOSTILE_TYPES>::value ));

    return true;
}

/*
tests_ccmc_agree_overflow
  merge_may_overflow and merge_may_overflow_into are the same predicate.
  Tests the following:
  - agreement across all four boundedness signals and the reserve anti-signal
  - agreement at the non-mergeable cases
  - agreement across the full hostile zoo
  - agreement in BOTH operand orderings of the asymmetric pair
  The last is the point of this test. The concept forwards finding F1's operand
  order faithfully, so the two faces SHARE the defect rather than differing over
  it - which is why the agreement holds and should. Agreement is a statement
  about the forwarding, not about the semantics; F1 is a defect in what the
  concept's operand names promise, and no agreement test can see it. That is
  what tests_ccmc_overflow_reads_left_operand is for.
*/
bool
tests_ccmc_agree_overflow()
{
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::merge_may_overflow,
                                          ::djinterp::merge_may_overflow_into,
                                          std::array<int, 4>,
                                          std::vector<int>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::merge_may_overflow,
                                          ::djinterp::merge_may_overflow_into,
                                          std::vector<int>,
                                          std::array<int, 4>));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::merge_may_overflow,
                                          ::djinterp::merge_may_overflow_into,
                                          cm_growable_seq, cm_growable_seq));
    D_CM_CHECK(D_TEST_TRAIT_CONCEPT_AGREE(::djinterp::merge_may_overflow,
                                          ::djinterp::merge_may_overflow_into,
                                          cm_extent_seq, cm_unique_bag));

    D_CM_CHECK(( dt::holds_for_all<cm_overflow_agree_self,
                                   std::array<int, 4>,
                                   std::vector<int>,
                                   std::string,
                                   cm_extent_seq,
                                   cm_interval_seq,
                                   cm_fixed_capacity_seq,
                                   cm_growable_seq,
                                   cm_bounded_bag,
                                   int>::value ));

    D_CM_CHECK(( dt::holds_for_all<cm_overflow_agree_self,
                                   D_TEST_HOSTILE_TYPES>::value ));

    return true;
}

/*
tests_ccmc_agree_cvref
  FINDING F6, pinned: the two faces agree under every cv-ref qualification, not
  merely at the bare type.
  Tests the following:
  - each of the four agreement traits holds across all eight cv-ref forms, with
    all() rather than merely agrees(), so agreement is pinned TRUE everywhere
    rather than pinned merely CONSISTENT
  - trait_ignores_cvref for each, so the fact folds into a larger expression
  The dependency this guards is unstated in either header.
  merge_elements_compatible cleans its two container arguments before the
  is_iterable_container guard but passes _Left and _Right RAW to
  elements_same_type / elements_convertible. It is cv-ref agnostic anyway -
  solely because those three traits clean internally. The concept, which cleans
  up front, would silently diverge from its trait on `const V&` the moment any
  of them stopped doing so, and nothing but this test would say so.
*/
bool
tests_ccmc_agree_cvref()
{
    D_CONSTEXPR dt::cvref_report copy_r =
        dt::trait_across_cvref<cm_copyable_agree, std::vector<int>>();
    D_CONSTEXPR dt::cvref_report elem_r =
        dt::trait_across_cvref<cm_elements_agree_self, std::vector<int>>();
    D_CONSTEXPR dt::cvref_report merge_r =
        dt::trait_across_cvref<cm_mergeable_agree_self, std::map<int, int>>();
    D_CONSTEXPR dt::cvref_report over_r =
        dt::trait_across_cvref<cm_overflow_agree_self, std::array<int, 4>>();

    // agreement holds in all eight cells, for each of the four
    D_CM_CHECK(copy_r.all());
    D_CM_CHECK(elem_r.all());
    D_CM_CHECK(merge_r.all());
    D_CM_CHECK(over_r.all());

    D_CM_CHECK(copy_r.first_disagreement() == nullptr);
    D_CM_CHECK(elem_r.first_disagreement() == nullptr);
    D_CM_CHECK(merge_r.first_disagreement() == nullptr);
    D_CM_CHECK(over_r.first_disagreement() == nullptr);

    // and on the refuting side, so agreement is not read only where both are
    // true
    D_CONSTEXPR dt::cvref_report copy_neg =
        dt::trait_across_cvref<cm_copyable_agree, int>();
    D_CONSTEXPR dt::cvref_report merge_neg =
        dt::trait_across_cvref<cm_mergeable_agree_self, cm_positional>();

    D_CM_CHECK(copy_neg.all());
    D_CM_CHECK(merge_neg.all());

    // as traits, foldable into a larger constant expression
    D_CM_CHECK(( dt::trait_ignores_cvref<cm_copyable_agree,
                                         std::vector<int>>::value ));
    D_CM_CHECK(( dt::trait_ignores_cvref<cm_elements_agree_self,
                                         std::vector<int>>::value ));
    D_CM_CHECK(( dt::trait_ignores_cvref<cm_mergeable_agree_self,
                                         std::map<int, int>>::value ));
    D_CM_CHECK(( dt::trait_ignores_cvref<cm_overflow_agree_self,
                                         std::array<int, 4>>::value ));

    return true;
}

/*
tests_ccmc_agree_nonvacuous
  the agreements above are not the trivial agreement of two predicates that are
  never true.
  Tests the following:
  - each of the four lifted concepts is TRUE for at least one type
    (holds_for_any over known-satisfying types)
  - each is FALSE for every type in the hostile zoo (holds_for_none)
  - so each is a live, two-valued predicate, and every agreement result above
    carries information
  Agreement is vacuously satisfied by two predicates that are both uniformly
  false - "they never disagree" is trivially true when neither is ever true - so
  an agreement test that does not also pin polarity proves nothing. This is that
  pin.
*/
bool
tests_ccmc_agree_nonvacuous()
{
    // each concept is true somewhere...
    D_CM_CHECK(( dt::holds_for_any<cm_copyable_c,
                                   std::vector<int>,
                                   std::map<int, int>>::value ));
    D_CM_CHECK(( dt::holds_for_any<cm_elements_self_c,
                                   std::vector<int>,
                                   std::set<int>>::value ));
    D_CM_CHECK(( dt::holds_for_any<cm_mergeable_self_c,
                                   std::vector<int>,
                                   std::multimap<int, int>>::value ));
    D_CM_CHECK(( dt::holds_for_any<cm_overflow_self_c,
                                   std::array<int, 4>,
                                   cm_extent_seq>::value ));

    // ...and false everywhere it should be
    D_CM_CHECK(( dt::holds_for_none<cm_copyable_c,
                                    D_TEST_HOSTILE_TYPES_COMPLETE>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_elements_self_c,
                                    D_TEST_HOSTILE_TYPES>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_mergeable_self_c,
                                    D_TEST_HOSTILE_TYPES>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_overflow_self_c,
                                    D_TEST_HOSTILE_TYPES>::value ));

    // the four are not all the SAME predicate either - each pair differs
    // somewhere, so the agreements are four separate facts and not one
    D_CM_CHECK(( ::djinterp::merge_elements_compatible_with<std::vector<int>,
                                                            std::set<int>> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::vector<int>,
                                             std::set<int>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::vector<int>> ));

    return true;
}

/*
tests_ccmc_agree_trait_shape
  half of FINDING F5: the traits behind the concepts are standard-shaped bool
  traits, with one exception.
  Tests the following:
  - is_copyable_container, copy_preserves_all_axes, merge_elements_compatible,
    is_mergeable and merge_has_key_conflict all satisfy is_bool_trait: a
    value_type of exactly bool, a ::value usable as a constant expression, a
    nested type that is bool_constant<value>, and public unambiguous derivation
    from it
  - merge_may_overflow does NOT: it is a plain struct carrying only a static
    constexpr bool, with no value_type, no nested type and no bool_constant
    base
  - the exception is benign for the quantifiers, which need only ::value, and
    it is recorded rather than asserted away: merge_may_overflow is the one
    trait of the six that cannot be used where a std-shaped bool trait is
    expected
*/
bool
tests_ccmc_agree_trait_shape()
{
    // the five that are standard-shaped
    D_CM_CHECK(( dt::is_bool_trait<
                     ::djinterp::is_copyable_container<
                         std::vector<int>>>::value ));
    D_CM_CHECK(( dt::is_bool_trait<
                     ::djinterp::copy_preserves_all_axes<
                         std::vector<int>>>::value ));
    D_CM_CHECK(( dt::is_bool_trait<
                     ::djinterp::merge_elements_compatible<
                         std::vector<int>, std::vector<int>>>::value ));
    D_CM_CHECK(( dt::is_bool_trait<
                     ::djinterp::is_mergeable<
                         std::vector<int>, std::vector<int>>>::value ));
    D_CM_CHECK(( dt::is_bool_trait<
                     ::djinterp::merge_has_key_conflict<
                         std::map<int, int>, std::map<int, int>>>::value ));

    // the one that is not
    D_CM_CHECK(( !dt::is_bool_trait<
                      ::djinterp::merge_may_overflow<
                          std::array<int, 4>, std::vector<int>>>::value ));

    // it still answers, which is why the quantifiers accept it
    D_CM_CHECK(( ::djinterp::merge_may_overflow<std::array<int, 4>,
                                                std::vector<int>>::value ));

    D_CM_NOTE("F5a: merge_may_overflow is not a standard-shaped bool trait - "
              "no value_type, no nested type, no bool_constant base - alone "
              "among the six bool predicates of the traits module");

    return true;
}

/*
tests_ccmc_agree_face_parity
  the other half of FINDING F5: the concept module covers four of the trait
  module's six bool predicates.
  Tests the following:
  - the four WITH a concept face are exercised through both faces, above
  - copy_preserves_all_axes is live and has no concept face: it is a spec note
    rather than a detection, true even of types that are not containers at all,
    which this test demonstrates by running it over the entire hostile zoo
  - merge_has_key_conflict is live, non-vacuous and has no concept face: true
    for a keyed merge whose result admits each key once, false for a merge of
    multimaps, false for a non-keyed merge
  The second is the substantive gap. Key conflict is one of the three provisos
  the traits preamble names, and it is the only one with no requires-facing
  form - so a template wanting to constrain on it must still reach for the
  trait, which is precisely the enable_if-shaped situation the concepts header
  exists to avoid.
*/
bool
tests_ccmc_agree_face_parity()
{
    // copy_preserves_all_axes: a note, not a detection - true of everything,
    // containers and hostile shapes alike
    D_CM_CHECK(::djinterp::copy_preserves_all_axes<std::vector<int>>::value);
    D_CM_CHECK(::djinterp::copy_preserves_all_axes<int>::value);
    D_CM_CHECK(( dt::holds_for_all<::djinterp::copy_preserves_all_axes,
                                   D_TEST_HOSTILE_TYPES>::value ));

    // merge_has_key_conflict: live and two-valued, with no concept face
    D_CM_CHECK(( ::djinterp::merge_has_key_conflict<
                     std::map<int, int>, std::map<int, int>>::value ));
    D_CM_CHECK(( ::djinterp::merge_has_key_conflict<
                     std::map<int, int>, std::multimap<int, int>>::value ));
    D_CM_CHECK(( !::djinterp::merge_has_key_conflict<
                      std::multimap<int, int>,
                      std::multimap<int, int>>::value ));
    D_CM_CHECK(( !::djinterp::merge_has_key_conflict<
                      std::vector<int>, std::vector<int>>::value ));
    D_CM_CHECK(( !::djinterp::merge_has_key_conflict<
                      std::set<int>, std::set<int>>::value ));

    D_CM_NOTE("F5b: copy_preserves_all_axes and merge_has_key_conflict have no "
              "requires-facing form; the key-conflict proviso is the "
              "substantive gap, being one of the three the traits preamble "
              "names and the only one a template must still use enable_if for");

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_CM_CONCEPTS_ENABLED
