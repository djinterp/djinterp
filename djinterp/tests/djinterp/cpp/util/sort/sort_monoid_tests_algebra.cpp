/*******************************************************************************
* djinterp [cpp]                                   sort_monoid_tests_algebra.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_monoid_tests_algebra.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "sort_monoid_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_sort_monoid_semigroup_registration
bool tests_sort_monoid_semigroup_registration()
{
    verdict result;

    //   The instance is registered in the explicit two-argument <T, void>
    // form against the SFINAE-hooked primary, so is_specialized is what the
    // protocol actually keys on.  If the specialization stopped matching,
    // mappend would silently fall back to the primary and fail to compile in
    // some distant caller instead of here.
    result.check((semigroup_traits<DIntRun, void>::is_specialized::value));
    result.check((semigroup_traits<DKeyedRun, void>::is_specialized::value));

    result.check(is_semigroup<DIntRun>::value);
    result.check(is_semigroup<DKeyedRun>::value);
    result.check((is_semigroup<sorted_run<int, greater<int> > >::value));

    return result.ok();
}


// tests_sort_monoid_monoid_registration
bool tests_sort_monoid_monoid_registration()
{
    verdict result;
    DIntRun identity;

    result.check((monoid_traits<DIntRun, void>::is_specialized::value));
    result.check((monoid_traits<DKeyedRun, void>::is_specialized::value));

    result.check(is_monoid<DIntRun>::value);
    result.check(is_monoid<DKeyedRun>::value);

    //   Every monoid is a semigroup, and the sorted-run instance registers
    // both -- combine without empty would not be enough for mconcat, which
    // needs a value to fold an empty sequence onto.
    result.check(is_semigroup<DIntRun>::value);

    //   monoid_traits::empty() and mempty<T>() must agree, and both must be
    // the empty run.
    identity = monoid_traits<DIntRun, void>::empty();

    result.check(identity.empty());
    result.check(identity.size() == 0);
    result.check(mempty<DIntRun>().empty());
    result.check(mempty<DIntRun>().size() == identity.size());

    return result.ok();
}


// tests_sort_monoid_mappend_merges
bool tests_sort_monoid_mappend_merges()
{
    verdict result;

    static const int left_values[]  = { 1, 3, 5 };
    static const int right_values[] = { 2, 4, 6 };
    static const int expected[]     = { 1, 2, 3, 4, 5, 6 };

    DIntRun left(ints_of(left_values, 3), less<int>(), true);
    DIntRun right(ints_of(right_values, 3), less<int>(), true);
    DIntRun combined;
    DIntRun merged;

    //   mappend on runs IS the merge: the algebra's whole claim.
    combined = semigroup_traits<DIntRun, void>::combine(left, right);
    merged   = mappend(left, right);

    result.check(ranges_equal(combined.data(), ints_of(expected, 6)));
    result.check(ranges_equal(merged.data(), ints_of(expected, 6)));
    result.check(ranges_equal(merged.data(), combined.data()));

    //   The operands are untouched -- combine takes them by const reference
    // and returns a new run.
    result.check(ranges_equal(left.data(), ints_of(left_values, 3)));
    result.check(ranges_equal(right.data(), ints_of(right_values, 3)));

    //   The result carries the LEFT operand's comparator, and is itself a
    // run that can be combined again.
    result.check(merged.compare()(1, 2));
    result.check(mappend(merged, left).size() == 9);

    //   The ordering used is the left run's, not the type's default.  Two
    // descending runs combine into a descending run.
    static const int high[]          = { 9, 7 };
    static const int low[]           = { 5, 1 };
    static const int high_then_low[] = { 9, 7, 5, 1 };

    sorted_run<int, greater<int> > descending_left(
        ints_of(high, 2), greater<int>(), true);
    sorted_run<int, greater<int> > descending_right(
        ints_of(low, 2), greater<int>(), true);

    result.check(ranges_equal(
        mappend(descending_left, descending_right).data(),
        ints_of(high_then_low, 4)));

    return result.ok();
}


// tests_sort_monoid_mempty_is_identity
bool tests_sort_monoid_mempty_is_identity()
{
    verdict result;

    static const int values[] = { 2, 4, 6 };

    DIntRun run(ints_of(values, 3), less<int>(), true);
    DIntRun identity;

    //   combine() short-circuits on an empty operand and returns the other
    // one AS IS, rather than merging against an empty vector.  The
    // observable consequence is that a stateful comparator on the empty side
    // is never consulted, and that the identity laws hold exactly rather
    // than merely up to element order.
    result.check(ranges_equal(mappend(identity, run).data(),
                              ints_of(values, 3)));
    result.check(ranges_equal(mappend(run, identity).data(),
                              ints_of(values, 3)));

    //   Identity combined with identity is identity.
    result.check(mappend(identity, identity).empty());
    result.check(mappend(mempty<DIntRun>(), mempty<DIntRun>()).size() == 0);

    return result.ok();
}


// tests_sort_monoid_identity_laws
bool tests_sort_monoid_identity_laws()
{
    verdict          result;
    std::vector<int> source;
    DIntRun          run;
    std::size_t      shape_index;
    std::size_t      size_index;
    std::size_t      length;

    //   The two monoid laws -- mempty <> x == x and x <> mempty == x --
    // checked over the whole corpus rather than on one hand-picked run, so a
    // size- or shape-dependent regression cannot hide.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length > 64)
        {
            continue;
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            source = make_shape(shape_index, length);
            run    = make_sorted_run(source);

            result.check(ranges_equal(
                mappend(mempty<DIntRun>(), run).data(), run.data()));
            result.check(ranges_equal(
                mappend(run, mempty<DIntRun>()).data(), run.data()));
            result.check(mappend(run, mempty<DIntRun>()).size() ==
                         run.size());
        }
    }

    return result.ok();
}


// tests_sort_monoid_associativity
bool tests_sort_monoid_associativity()
{
    verdict result;

    static const int a_values[] = { 1, 4, 7 };
    static const int b_values[] = { 2, 5, 8 };
    static const int c_values[] = { 3, 6, 9 };

    DIntRun a(ints_of(a_values, 3), less<int>(), true);
    DIntRun b(ints_of(b_values, 3), less<int>(), true);
    DIntRun c(ints_of(c_values, 3), less<int>(), true);

    std::vector<DKeyedValue> ka;
    std::vector<DKeyedValue> kb;
    std::vector<DKeyedValue> kc;

    DKeyedRun ra;
    DKeyedRun rb;
    DKeyedRun rc;
    DKeyedRun left_grouped;
    DKeyedRun right_grouped;

    DIntRun identity;

    static const int shared_keys[] = { 1, 1 };
    static const int a_tags[]      = { 0, 1 };
    static const int b_tags[]      = { 2, 3 };
    static const int c_tags[]      = { 4, 5 };

    std::size_t i;

    //   Associativity is the semigroup law, and for a merge it is not
    // obvious: a merge that broke ties inconsistently would still produce
    // sorted output from both groupings while disagreeing on the placement
    // of equivalents.
    result.check(ranges_equal(mappend(mappend(a, b), c).data(),
                              mappend(a, mappend(b, c)).data()));

    //   So the sharp version of the law uses elements that are all
    // equivalent under the comparator but distinguishable to the test.  Both
    // groupings must yield tags 0..5 in order.
    ka = keyed_of(shared_keys, a_tags, 2);
    kb = keyed_of(shared_keys, b_tags, 2);
    kc = keyed_of(shared_keys, c_tags, 2);

    ra = DKeyedRun(ka, keyed_value_less(), true);
    rb = DKeyedRun(kb, keyed_value_less(), true);
    rc = DKeyedRun(kc, keyed_value_less(), true);

    left_grouped  = mappend(mappend(ra, rb), rc);
    right_grouped = mappend(ra, mappend(rb, rc));

    result.check(left_grouped.size() == 6);
    result.check(right_grouped.size() == 6);

    for (i = 0; i < 6; ++i)
    {
        result.check(left_grouped.data()[i] == right_grouped.data()[i]);
        result.check(left_grouped.data()[i].tag == static_cast<int>(i));
    }

    //   Empty operands in any position leave associativity intact.
    result.check(ranges_equal(mappend(mappend(identity, a), b).data(),
                              mappend(identity, mappend(a, b)).data()));
    result.check(ranges_equal(mappend(mappend(a, identity), b).data(),
                              mappend(a, mappend(identity, b)).data()));
    result.check(ranges_equal(mappend(mappend(a, b), identity).data(),
                              mappend(a, mappend(b, identity)).data()));

    return result.ok();
}


// tests_sort_monoid_mconcat_is_merge_sort
bool tests_sort_monoid_mconcat_is_merge_sort()
{
    verdict                         result;
    std::vector<int>                source;
    std::vector<int>                expected;
    std::vector<DKeyedBatteryCase> keyed_cases;
    std::vector<DKeyedValue>        folded;
    std::size_t                     shape_index;
    std::size_t                     size_index;
    std::size_t                     case_index;
    std::size_t                     length;

    //   The header's headline claim: folding singleton runs with mconcat is
    // a bottom-up merge sort, expressed entirely through the algebra.  It is
    // checked against std::stable_sort over the whole corpus.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        length = battery_size_at(size_index);

        if (length > 128)
        {
            continue;
        }

        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            source   = make_shape(shape_index, length);
            expected = source;

            std::stable_sort(expected.begin(),
                             expected.end(),
                             less<int>());

            result.check(ranges_equal(
                mconcat(to_singleton_runs(source, less<int>())).data(),
                expected));
        }
    }

    //   And it is a STABLE merge sort, which the int corpus cannot show.
    // Folding singletons of equivalent elements must leave them in input
    // order.
    keyed_cases = keyed_battery();

    for (case_index = 0; case_index < keyed_cases.size(); ++case_index)
    {
        folded = mconcat(to_singleton_runs(keyed_cases[case_index].data,
                                           keyed_value_less())).data();

        result.check(folded.size() == keyed_cases[case_index].data.size());
        result.check(is_sorted_range(folded.begin(),
                                     folded.end(),
                                     keyed_value_less()));
        result.check(is_stable_result(folded));
    }

    //   mconcat of nothing is the identity.
    result.check(mconcat(std::vector<DIntRun>()).empty());

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
