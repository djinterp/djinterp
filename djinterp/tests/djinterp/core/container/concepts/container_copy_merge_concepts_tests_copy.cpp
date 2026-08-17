#include "container_copy_merge_concepts_tests.hpp"


#if D_CM_CONCEPTS_ENABLED

NS_DJINTERP
NS_TESTING

/*
tests_ccmc_copy_positive
  copyable_container holds for containers that can be copy-constructed.
  Tests the following:
  - the std containers across all six merge disciplines satisfy it
  - std::array and std::string satisfy it (a bounded sequence and a text buffer)
  - the hand-rolled sequences satisfy it regardless of which capacity signal
    they carry, since boundedness is nothing to do with copyability
  - cm_elementless_range satisfies it, so the concept does not secretly require
    a value_type the way the merge concepts' element clause does
  - run through holds_for_all, so every cell instantiates rather than stopping
    at the first false
*/
bool
tests_ccmc_copy_positive()
{
    // the six disciplines, as the std library spells them
    D_CM_CHECK(( dt::holds_for_all<cm_copyable_c,
                                   std::vector<int>,
                                   std::set<int>,
                                   std::multiset<int>,
                                   std::map<int, int>,
                                   std::multimap<int, int>,
                                   std::array<int, 4>,
                                   std::string>::value ));

    // the hand-rolled shapes: every capacity signal, plus the elementless one
    D_CM_CHECK(( dt::holds_for_all<cm_copyable_c,
                                   cm_plain_seq,
                                   cm_extent_seq,
                                   cm_interval_seq,
                                   cm_fixed_capacity_seq,
                                   cm_growable_seq,
                                   cm_elementless_range,
                                   cm_unique_bag,
                                   cm_repeat_bag,
                                   cm_unique_keyed,
                                   cm_repeat_keyed,
                                   cm_bounded_bag>::value ));

    // spelled at a point as well, so the report shows the concept and not only
    // the quantifier
    D_CM_CHECK(::djinterp::copyable_container<std::vector<int>>);
    D_CM_CHECK(::djinterp::copyable_container<cm_elementless_range>);

    return true;
}

/*
tests_ccmc_copy_negative
  copyable_container fails, and fails for the right reason of the two.
  Tests the following:
  - cm_noncopyable_seq is iterable but has a deleted copy constructor: the
    container clause holds and the copy clause does not
  - cm_positional has a value_type and no traversal: the copy clause holds and
    the container clause does not
  - int and cm_opaque are copyable non-containers: only the container clause is
    at issue
  - each failure is confirmed against the underlying clause traits, so a test
    passing for the wrong reason is visible
*/
bool
tests_ccmc_copy_negative()
{
    // iterable, but the copy clause fails
    D_CM_CHECK(!::djinterp::copyable_container<cm_noncopyable_seq>);
    D_CM_CHECK(::djinterp::is_iterable_container<cm_noncopyable_seq>::value);
    D_CM_CHECK(!std::is_copy_constructible<cm_noncopyable_seq>::value);

    // copyable, but the container clause fails - a value_type is not enough
    D_CM_CHECK(!::djinterp::copyable_container<cm_positional>);
    D_CM_CHECK(!::djinterp::is_iterable_container<cm_positional>::value);
    D_CM_CHECK(std::is_copy_constructible<cm_positional>::value);

    // neither a container nor pretending to be one
    D_CM_CHECK(( dt::holds_for_none<cm_copyable_c,
                                    int,
                                    cm_opaque,
                                    cm_from_int,
                                    double*>::value ));

    return true;
}

/*
tests_ccmc_copy_cvref
  the concept answers identically under every cv-ref qualification.
  Tests the following:
  - all eight forms of std::vector<int> satisfy it (agrees AND all)
  - all eight forms of int refute it (agrees AND none), so the invariance is
    not the vacuous invariance of a uniformly false predicate
  - the deleted-copy negative is invariant too, which is the case a concept
    that forgot to strip would most plausibly get wrong
  - first_disagreement() is null in each case, so a regression reports the
    offending cell by name rather than a bare failure
*/
bool
tests_ccmc_copy_cvref()
{
    D_CONSTEXPR dt::cvref_report vec =
        dt::trait_across_cvref<cm_copyable_c, std::vector<int>>();
    D_CONSTEXPR dt::cvref_report plain =
        dt::trait_across_cvref<cm_copyable_c, int>();
    D_CONSTEXPR dt::cvref_report nocopy =
        dt::trait_across_cvref<cm_copyable_c, cm_noncopyable_seq>();

    // the positive: every form agrees with the bare type, and all are true
    D_CM_CHECK(vec.agrees());
    D_CM_CHECK(vec.all());
    D_CM_CHECK(vec.first_disagreement() == nullptr);

    // the negative: every form agrees, and all are false
    D_CM_CHECK(plain.agrees());
    D_CM_CHECK(plain.none());

    // the interesting negative - a container whose copy clause fails
    D_CM_CHECK(nocopy.agrees());
    D_CM_CHECK(nocopy.none());

    // and as a trait, so the fact can be folded into a larger expression
    D_CM_CHECK(( dt::trait_ignores_cvref<cm_copyable_c,
                                         std::vector<int>>::value ));
    D_CM_CHECK(( dt::trait_ignores_cvref<cm_copyable_c, int>::value ));

    return true;
}

/*
tests_ccmc_copy_hostile
  the concept survives, and rejects, every adversarial shape it can be asked
  about - and records the one it cannot be asked about at all.
  Tests the following:
  - holds_for_none over D_TEST_HOSTILE_TYPES_COMPLETE: the concept is false for
    every fixture in the zoo
  - count_holds is exactly zero, which is the same statement made in a form
    that instantiates every cell, so a definition ill-formed for any hostile
    shape breaks the BUILD here rather than hiding behind a short circuit
  - the class and non-class halves separately, so a regression names its half
  FINDING F2: the battery runs over the COMPLETE list, not the full zoo,
  because copyable_container routes through std::is_copy_constructible, which
  the standard requires be given a complete type. is_copyable_container names
  that trait in a conjunction, and naming it instantiates it whatever the other
  operand says - so copyable_container<incomplete> fails the build, not the
  report. The restriction is inherited rather than chosen, but it is real, and
  it is the one respect in which this concept is less total than the three
  merge concepts, none of which touches a completeness-requiring trait.
*/
bool
tests_ccmc_copy_hostile()
{
    // the full zoo minus the incomplete fixture - see F2
    D_CM_CHECK(( dt::holds_for_none<cm_copyable_c,
                                    D_TEST_HOSTILE_TYPES_COMPLETE>::value ));

    // the same claim, made so that every cell must instantiate
    D_CM_CHECK(( dt::count_holds<cm_copyable_c,
                                 D_TEST_HOSTILE_TYPES_COMPLETE>::value == 0u ));

    // each half separately, so a regression says which
    D_CM_CHECK(( dt::holds_for_none<
                     cm_copyable_c,
                     D_TEST_HOSTILE_CLASS_TYPES_COMPLETE>::value ));
    D_CM_CHECK(( dt::holds_for_none<cm_copyable_c,
                                    D_TEST_HOSTILE_NONCLASS_TYPES>::value ));

    D_CM_NOTE("F2: copyable_container inherits std::is_copy_constructible's "
              "completeness mandate - it cannot be asked about an incomplete "
              "type at all, so this battery runs over "
              "D_TEST_HOSTILE_TYPES_COMPLETE rather than the full zoo");

    return true;
}

/*
tests_ccmc_copy_constrains
  the concept works as a CONSTRAINT, not merely as a bool.
  Tests the following:
  - a copyable_container-constrained overload is chosen over an otherwise
    identical unconstrained one for a satisfying type
  - the unconstrained fallback is chosen for a type failing either clause
  - the selection is a constant expression, so the dispatch happens where a
    requires-clause is actually consulted rather than at run time
  This is the module's stated purpose - "spelled so it can constrain a template
  instead of gating one through enable_if" - and evaluating the concept as a
  bool does not exercise it.
*/
bool
tests_ccmc_copy_constrains()
{
    D_CM_CHECK(cm_copy_probe<std::vector<int>>() == 1);
    D_CM_CHECK(cm_copy_probe<std::map<int, int>>() == 1);
    D_CM_CHECK(cm_copy_probe<cm_plain_seq>() == 1);

    // fails the copy clause
    D_CM_CHECK(cm_copy_probe<cm_noncopyable_seq>() == 0);

    // fails the container clause
    D_CM_CHECK(cm_copy_probe<cm_positional>() == 0);
    D_CM_CHECK(cm_copy_probe<int>() == 0);

    // the dispatch resolves during constant evaluation
    D_CONSTEXPR int chosen = cm_copy_probe<std::vector<int>>();
    D_CM_CHECK(chosen == 1);

    return true;
}

NS_END  // testing
NS_END  // djinterp

#endif  // D_CM_CONCEPTS_ENABLED
