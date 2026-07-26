#include "bsearch_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_misses_empty_pack
  Searching a pack with no entries.
  Tests the following:
  - the reported index is lookup_npos
  - found is false
  - the reported ::type is lookup_not_found
  - bsearch_by_t is lookup_not_found
*/
bool
tests_misses_empty_pack()
{
    using result = search<5>;

    D_BS_CHECK(result::index == lookup_npos);
    D_BS_CHECK(!result::found);
    D_BS_CHECK((std::is_same<result::type, lookup_not_found>::value));
    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<5>::template below,
                                 asc_preds<5>::template above>,
                    lookup_not_found>::value));

    return true;
}

/*
tests_misses_below_all
  A needle smaller than every key.
  Tests the following:
  - the search misses (found false)
  - the reported index is lookup_npos
  - ::type is lookup_not_found
  - the same holds for a needle far below the minimum
*/
bool
tests_misses_below_all()
{
    using e10 = ikey<10>; using e20 = ikey<20>; using e30 = ikey<30>;

    using just_below = search<9, e10, e20, e30>;

    D_BS_CHECK(!just_below::found);
    D_BS_CHECK(just_below::index == lookup_npos);
    D_BS_CHECK((std::is_same<just_below::type, lookup_not_found>::value));

    using far_below = search<-1000, e10, e20, e30>;

    D_BS_CHECK(!far_below::found);
    D_BS_CHECK(far_below::index == lookup_npos);

    return true;
}

/*
tests_misses_above_all
  A needle larger than every key.
  Tests the following:
  - the search misses
  - the reported index is lookup_npos
  - ::type is lookup_not_found
  - the same holds for a needle far above the maximum
*/
bool
tests_misses_above_all()
{
    using e10 = ikey<10>; using e20 = ikey<20>; using e30 = ikey<30>;

    using just_above = search<31, e10, e20, e30>;

    D_BS_CHECK(!just_above::found);
    D_BS_CHECK(just_above::index == lookup_npos);
    D_BS_CHECK((std::is_same<just_above::type, lookup_not_found>::value));

    using far_above = search<1000, e10, e20, e30>;

    D_BS_CHECK(!far_above::found);
    D_BS_CHECK(far_above::index == lookup_npos);

    return true;
}

/*
tests_misses_interior_gaps
  Needles that fall between adjacent keys.
  Tests the following:
  - a needle just above the first key but below the second misses
  - a needle in a middle gap misses
  - a needle just below the last key misses
  - every interior miss reports lookup_npos and lookup_not_found
*/
bool
tests_misses_interior_gaps()
{
    // keys 2,4,6,8 -> gaps at 3,5,7
    using e2 = ikey<2>; using e4 = ikey<4>; using e6 = ikey<6>; using e8 = ikey<8>;

    using gap3 = search<3, e2, e4, e6, e8>;
    using gap5 = search<5, e2, e4, e6, e8>;
    using gap7 = search<7, e2, e4, e6, e8>;

    D_BS_CHECK(!gap3::found);
    D_BS_CHECK(!gap5::found);
    D_BS_CHECK(!gap7::found);

    D_BS_CHECK(gap3::index == lookup_npos);
    D_BS_CHECK(gap5::index == lookup_npos);
    D_BS_CHECK(gap7::index == lookup_npos);

    D_BS_CHECK((std::is_same<gap3::type, lookup_not_found>::value));
    D_BS_CHECK((std::is_same<gap5::type, lookup_not_found>::value));
    D_BS_CHECK((std::is_same<gap7::type, lookup_not_found>::value));

    return true;
}

/*
tests_misses_single_element_low_and_high
  Misses against a one-element pack, on both sides of the sole key.
  Tests the following:
  - a needle below the key misses
  - a needle above the key misses
  - both report lookup_npos
  - the matching needle still hits (control), so the pack itself is searchable
*/
bool
tests_misses_single_element_low_and_high()
{
    using only = ikey<5>;

    D_BS_CHECK(!(search<1, only>::found));
    D_BS_CHECK(!(search<9, only>::found));
    D_BS_CHECK((search<1, only>::index) == lookup_npos);
    D_BS_CHECK((search<9, only>::index) == lookup_npos);

    // control: the sole key is findable
    D_BS_CHECK((search<5, only>::found));
    D_BS_CHECK((search<5, only>::index) == 0u);

    return true;
}

/*
tests_misses_sentinels_are_shared_values
  The miss sentinels are the shared, well-known values.
  Tests the following:
  - lookup_npos equals (std::size_t)-1
  - a missed search reports exactly that value
  - lookup_not_found is an empty, complete type usable as ::type
  - the missed ::type is exactly lookup_not_found (not merely convertible)
*/
bool
tests_misses_sentinels_are_shared_values()
{
    // the index sentinel is the canonical npos
    D_BS_CHECK(lookup_npos == static_cast<std::size_t>(-1));

    using miss = search<100, ikey<1>, ikey<2>, ikey<3> >;

    D_BS_CHECK(miss::index == lookup_npos);
    D_BS_CHECK(miss::index == static_cast<std::size_t>(-1));

    // the type sentinel is a complete type
    D_BS_CHECK(std::is_class<lookup_not_found>::value);
    D_BS_CHECK((std::is_same<miss::type, lookup_not_found>::value));

    static_assert(miss::index == lookup_npos, "miss index is the shared sentinel");

    return true;
}

/*
tests_misses_result_members_are_consistent
  ::type, ::found and ::index tell one coherent story on a miss.
  Tests the following:
  - found is false
  - index is lookup_npos
  - ::type is lookup_not_found
  - the three agree across two different kinds of miss (gap and out-of-range)
*/
bool
tests_misses_result_members_are_consistent()
{
    using gap = search<5, ikey<2>, ikey<4>, ikey<6>, ikey<8> >;

    D_BS_CHECK(!gap::found);
    D_BS_CHECK(gap::index == lookup_npos);
    D_BS_CHECK((std::is_same<gap::type, lookup_not_found>::value));

    using oob = search<1000, ikey<2>, ikey<4>, ikey<6>, ikey<8> >;

    D_BS_CHECK(!oob::found);
    D_BS_CHECK(oob::index == lookup_npos);
    D_BS_CHECK((std::is_same<oob::type, lookup_not_found>::value));

    // found is exactly the derived predicate (idx != npos)
    D_BS_CHECK(gap::found == (gap::index != lookup_npos));
    D_BS_CHECK(oob::found == (oob::index != lookup_npos));

    return true;
}

/*
tests_misses_alias_yields_not_found
  bsearch_by_t is lookup_not_found on a miss.
  Tests the following:
  - the alias equals lookup_not_found for an out-of-range needle
  - it equals bsearch_by<...>::type
  - it equals lookup_not_found for an interior-gap needle too
  - a hit on the same pack gives a non-sentinel type (contrast)
*/
bool
tests_misses_alias_yields_not_found()
{
    using e2 = ikey<2>; using e4 = ikey<4>; using e6 = ikey<6>;

    // out-of-range
    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<100>::template below,
                                 asc_preds<100>::template above, e2, e4, e6>,
                    lookup_not_found>::value));

    // agrees with ::type
    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<100>::template below,
                                 asc_preds<100>::template above, e2, e4, e6>,
                    search<100, e2, e4, e6>::type>::value));

    // interior gap
    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<3>::template below,
                                 asc_preds<3>::template above, e2, e4, e6>,
                    lookup_not_found>::value));

    // contrast: a hit is not the sentinel
    D_BS_CHECK(!(std::is_same<
                     bsearch_by_t<asc_preds<4>::template below,
                                  asc_preds<4>::template above, e2, e4, e6>,
                     lookup_not_found>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
