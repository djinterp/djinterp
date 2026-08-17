#include "container_copy_merge_concepts_tests.hpp"


#if D_CM_CONCEPTS_ENABLED

NS_DJINTERP
NS_TESTING

/*
tests_ccmc_overflow_extent
  a static `extent` constant marks the result bounded.
  Tests the following:
  - cm_extent_seq against itself overflows
  - cm_plain_seq, which differs from it in NOTHING but the extent constant,
    does not - so the signal is isolated to that member alone
  - both pairs are mergeable, so the difference is the capacity clause and not
    the precondition
*/
bool
tests_ccmc_overflow_extent()
{
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_extent_seq,
                                                     cm_extent_seq> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<cm_plain_seq,
                                                      cm_plain_seq> ));

    // both are mergeable - the capacity clause is the only difference
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_extent_seq, cm_extent_seq> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_plain_seq, cm_plain_seq> ));

    return true;
}

/*
tests_ccmc_overflow_tuple_size
  a std::tuple_size specialization marks the result bounded.
  Tests the following:
  - std::array against itself overflows
  - std::vector does not, differing from it in no other signal this trait reads
    (neither has an extent or static bounds; array has no capacity() at all,
    and vector's is cancelled by reserve())
  - the two are mergeable with each other, both being sequences of int
*/
bool
tests_ccmc_overflow_tuple_size()
{
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                     std::array<int, 4>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::vector<int>> ));

    D_CM_CHECK(( ::djinterp::mergeable_with<std::array<int, 4>,
                                            std::vector<int>> ));

    // the diagonal, as batteries, so every cell instantiates
    D_CM_CHECK(( dt::holds_for_all<cm_overflow_self_c,
                                   std::array<int, 4>,
                                   std::array<char, 1>>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_overflow_self_c,
                                    std::vector<int>,
                                    std::set<int>,
                                    std::map<int, int>,
                                    std::string>::value ));

    return true;
}

/*
tests_ccmc_overflow_static_bounds
  static lower_bound / upper_bound mark a finite closed-interval domain bounded.
  Tests the following:
  - cm_interval_seq against itself overflows
  - it is mergeable with the other sequences, so the domain marking does not
    disturb the discipline reading
  - cm_plain_seq, which carries neither bound, does not overflow
*/
bool
tests_ccmc_overflow_static_bounds()
{
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_interval_seq,
                                                     cm_interval_seq> ));
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_interval_seq,
                                                     cm_plain_seq> ));

    D_CM_CHECK(( ::djinterp::mergeable_with<cm_interval_seq,
                                            std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::merge_discipline_of<cm_interval_seq>::value
                 == ::djinterp::merge_discipline::sequence ));

    return true;
}

/*
tests_ccmc_overflow_fixed_capacity
  a const-callable capacity() with no reserve() marks the result bounded.
  Tests the following:
  - cm_fixed_capacity_seq, a sequence, overflows
  - cm_bounded_bag, a SET carrying the same capacity signal, overflows too - so
    the proviso is read off the bound and not off the discipline
  - cm_bounded_bag against std::set<int> overflows, the bound belonging to the
    result type rather than to the pair
*/
bool
tests_ccmc_overflow_fixed_capacity()
{
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_fixed_capacity_seq,
                                                     cm_fixed_capacity_seq> ));

    // the same signal on a bag discipline
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_bounded_bag,
                                                     cm_bounded_bag> ));
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_bounded_bag,
                                                     std::set<int>> ));
    D_CM_CHECK(( ::djinterp::merge_discipline_of<cm_bounded_bag>::value
                 == ::djinterp::merge_discipline::set ));

    // the unbounded bag does not
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::set<int>,
                                                      cm_bounded_bag> ));

    return true;
}

/*
tests_ccmc_overflow_reserve_anti_signal
  reserve() is what keeps a capacity() from meaning a FIXED bound.
  Tests the following:
  - cm_growable_seq, which differs from cm_fixed_capacity_seq only in having a
    reserve(), does not overflow
  - std::vector and std::string, which have both accessors, agree
  - the pair isolates the anti-signal exactly: same capacity(), same
    discipline, same element, opposite verdict
*/
bool
tests_ccmc_overflow_reserve_anti_signal()
{
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<cm_growable_seq,
                                                      cm_growable_seq> ));
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_fixed_capacity_seq,
                                                     cm_fixed_capacity_seq> ));

    // the two are otherwise indistinguishable to this axis
    D_CM_CHECK((    ::djinterp::merge_discipline_of<cm_growable_seq>::value
                 == ::djinterp::merge_discipline_of<
                        cm_fixed_capacity_seq>::value ));
    D_CM_CHECK(( ::djinterp::mergeable_with<cm_growable_seq,
                                            cm_fixed_capacity_seq> ));

    // the std types with both accessors
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::string,
                                                      std::string> ));

    return true;
}

/*
tests_ccmc_overflow_requires_mergeable
  boundedness alone is not enough - the merge must be defined first.
  Tests the following:
  - a bounded sequence against a bag refutes the concept, the disciplines
    disagreeing, despite the result carrying every capacity signal
  - a bounded sequence against an element-incompatible sequence refutes it too
  - a bounded non-container refutes it
  - in each case the capacity signal really IS present on the result type, so
    the refusal is attributable to the mergeability precondition alone
*/
bool
tests_ccmc_overflow_requires_mergeable()
{
    // cross-family: the result is bounded, the merge is not defined
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<cm_extent_seq,
                                                      cm_unique_bag> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                      std::set<int>> ));

    // element-incompatible
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<
                     cm_extent_seq,
                     std::vector<cm_opaque>> ));

    // the precondition is what failed, in each case
    D_CM_CHECK(( !::djinterp::mergeable_with<cm_extent_seq, cm_unique_bag> ));
    D_CM_CHECK(( !::djinterp::mergeable_with<std::array<int, 4>,
                                             std::set<int>> ));

    // ...and the bound really is there - the same result type overflows when
    // the merge IS defined
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_extent_seq,
                                                     cm_plain_seq> ));
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                     std::vector<int>> ));

    return true;
}

/*
tests_ccmc_overflow_reads_left_operand
  FINDING F1, pinned as observed: the concept answers about its FIRST operand,
  not the target its name and comment describe.
  Tests the following:
  - merge_may_overflow_into<bounded, unbounded> is TRUE and
    merge_may_overflow_into<unbounded, bounded> is FALSE, for the hand-rolled
    pair and for std::array against std::vector
  - both orderings are mergeable, so the asymmetry is the capacity clause's and
    not the precondition's
  - the root cause is exhibited directly: merge_result_type_t<_Left, _Right> is
    clean_t<_Left>, so merge_may_overflow reads its capacity signals off the
    LEFT operand, and the concept passes _From there
  Read as the concept's comment intends - "the merge CAN exceed _To's capacity.
  Bounded targets need this checked" - both lines are backwards: merging into an
  unbounded target cannot overflow, and merging into a bounded one can. The
  TRAIT is self-consistent and correctly documented ("The result type (the left
  operand) decides it"); it is the concept's _From / _To naming that imposes a
  directional reading the forwarding does not honour.
  This test will FAIL if the operand order is repaired. That is the intent: it
  is the alarm on the defect, not an endorsement of it. See the suite header
  preamble for the two available repairs.
*/
bool
tests_ccmc_overflow_reads_left_operand()
{
    // bounded FIRST -> true; bounded SECOND -> false.  Both backwards under
    // the documented "merge _From into _To" reading.
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<cm_fixed_capacity_seq,
                                                     cm_plain_seq> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<cm_plain_seq,
                                                      cm_fixed_capacity_seq> ));

    // the same inversion with std types
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                     std::vector<int>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::vector<int>,
                                                      std::array<int, 4>> ));

    // both orderings merge, so the precondition is not what differs
    D_CM_CHECK(( ::djinterp::mergeable_with<std::array<int, 4>,
                                            std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::mergeable_with<std::vector<int>,
                                            std::array<int, 4>> ));

    // the root cause: the result type is the LEFT operand, cleaned
    D_CM_CHECK(( std::is_same<
                     ::djinterp::merge_result_type_t<std::array<int, 4>,
                                                     std::vector<int>>,
                     std::array<int, 4>>::value ));
    D_CM_CHECK(( std::is_same<
                     ::djinterp::merge_result_type_t<const std::vector<int>&,
                                                     std::array<int, 4>>,
                     std::vector<int>>::value ));

    // the concept is therefore NOT symmetric, alone among the four
    D_CM_CHECK((    ::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                        std::vector<int>>
                 != ::djinterp::merge_may_overflow_into<std::vector<int>,
                                                        std::array<int, 4>> ));

    D_CM_NOTE("F1: merge_may_overflow_into answers about _From, not _To - "
              "<bounded, unbounded> is true and <unbounded, bounded> is false, "
              "the inverse of the documented 'may exceed _To's capacity' "
              "reading; pinned as observed, so a repair trips this test");

    return true;
}

/*
tests_ccmc_overflow_cvref
  the concept answers identically under every cv-ref qualification.
  Tests the following:
  - all eight forms of an overflowing pair agree and are true (all)
  - all eight forms of a non-overflowing but mergeable pair agree and are false
    (none), so the invariance is not the vacuous invariance of a predicate that
    is never true
  - one operand qualified at a time, including qualification of the operand
    whose bound is being read - the case a concept that cleaned only one
    argument would get wrong
*/
bool
tests_ccmc_overflow_cvref()
{
    D_CONSTEXPR dt::cvref_report arr =
        dt::trait_across_cvref<cm_overflow_self_c, std::array<int, 4>>();
    D_CONSTEXPR dt::cvref_report vec =
        dt::trait_across_cvref<cm_overflow_self_c, std::vector<int>>();
    D_CONSTEXPR dt::cvref_report fixed =
        dt::trait_across_cvref<cm_overflow_self_c, cm_fixed_capacity_seq>();

    D_CM_CHECK(arr.agrees());
    D_CM_CHECK(arr.all());
    D_CM_CHECK(arr.first_disagreement() == nullptr);

    D_CM_CHECK(vec.agrees());
    D_CM_CHECK(vec.none());

    D_CM_CHECK(fixed.agrees());
    D_CM_CHECK(fixed.all());

    // the bound-carrying operand, qualified
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<const std::array<int, 4>&,
                                                     std::vector<int>> ));
    D_CM_CHECK(( ::djinterp::merge_may_overflow_into<std::array<int, 4>&&,
                                                     const std::vector<int>&> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<
                     const volatile std::vector<int>,
                     std::array<int, 4>&&> ));

    return true;
}

/*
tests_ccmc_overflow_hostile
  the concept survives, and rejects, the full adversarial zoo.
  Tests the following:
  - holds_for_none over D_TEST_HOSTILE_TYPES paired with itself: nothing that
    fails the mergeability precondition can overflow
  - count_holds is exactly zero, in the form that instantiates every cell, so
    the boundedness probes - which name std::tuple_size, a static member, and a
    const-qualified capacity() call - are proved SFINAE-friendly across every
    hostile shape
  - each half of the zoo separately
  - hostile shapes paired with real containers, in both operand positions,
    since this is the one concept for which position matters
*/
bool
tests_ccmc_overflow_hostile()
{
    D_CM_CHECK(( dt::holds_for_none<cm_overflow_self_c,
                                    D_TEST_HOSTILE_TYPES>::value ));

    D_CM_CHECK(( dt::count_holds<cm_overflow_self_c,
                                 D_TEST_HOSTILE_TYPES>::value == 0u ));

    D_CM_CHECK(( dt::holds_for_none<cm_overflow_self_c,
                                    D_TEST_HOSTILE_CLASS_TYPES>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_overflow_self_c,
                                    D_TEST_HOSTILE_NONCLASS_TYPES>::value ));

    // both operand positions - the asymmetric concept of the four
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<dt::fixtures::array_type,
                                                      std::array<int, 4>> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<std::array<int, 4>,
                                                      dt::fixtures::array_type> ));
    D_CM_CHECK(( !::djinterp::merge_may_overflow_into<dt::fixtures::incomplete,
                                                      dt::fixtures::literal> ));

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_CM_CONCEPTS_ENABLED
