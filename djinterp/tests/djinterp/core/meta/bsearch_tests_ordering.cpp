#include "bsearch_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_ordering_descending_pack
  With the arrows flipped, a high-to-low pack searches correctly.
  Tests the following:
  - a present key is found at its descending position
  - the first (largest) and last (smallest) keys are found at 0 and the end
  - a needle in a descending gap misses
  - a needle outside the range misses
*/
bool
tests_ordering_descending_pack()
{
    // keys 9,7,5,3,1 -- sorted DESCENDING, at indices 0..4
    using d9 = ikey<9>; using d7 = ikey<7>; using d5 = ikey<5>;
    using d3 = ikey<3>; using d1 = ikey<1>;

    // an interior present key
    D_BS_CHECK((search_desc<5, d9, d7, d5, d3, d1>::index) == 2u);
    D_BS_CHECK((search_desc<5, d9, d7, d5, d3, d1>::found));
    D_BS_CHECK((std::is_same<search_desc<5, d9, d7, d5, d3, d1>::type, d5>::value));

    // the extremes
    D_BS_CHECK((search_desc<9, d9, d7, d5, d3, d1>::index) == 0u);
    D_BS_CHECK((search_desc<1, d9, d7, d5, d3, d1>::index) == 4u);

    // a descending gap and an out-of-range needle miss
    D_BS_CHECK(!(search_desc<4, d9, d7, d5, d3, d1>::found));
    D_BS_CHECK(!(search_desc<10, d9, d7, d5, d3, d1>::found));
    D_BS_CHECK((search_desc<4, d9, d7, d5, d3, d1>::index) == lookup_npos);

    return true;
}

/*
tests_ordering_type_needle_by_sizeof
  A TYPE needle with no ::key -- entries ordered by sizeof.
  Tests the following:
  - a type whose size matches an entry is found at that entry's position
  - the smallest and largest sizes are found at the ends
  - a size falling between two entries misses
  - the engine reaches the matched entry as ::type without ever reading ::key
*/
bool
tests_ordering_type_needle_by_sizeof()
{
    // sizes 1,2,4,8 (ascending by sizeof) at indices 0..3
    using s1 = sized<1>; using s2 = sized<2>; using s4 = sized<4>; using s8 = sized<8>;

    // matching sizes
    D_BS_CHECK((search_size<1, s1, s2, s4, s8>::index) == 0u);
    D_BS_CHECK((search_size<4, s1, s2, s4, s8>::index) == 2u);
    D_BS_CHECK((search_size<8, s1, s2, s4, s8>::index) == 3u);

    D_BS_CHECK((search_size<4, s1, s2, s4, s8>::found));
    D_BS_CHECK((std::is_same<search_size<4, s1, s2, s4, s8>::type, s4>::value));

    // sizes 3, 5, 16 are absent
    D_BS_CHECK(!(search_size<3,  s1, s2, s4, s8>::found));
    D_BS_CHECK(!(search_size<5,  s1, s2, s4, s8>::found));
    D_BS_CHECK(!(search_size<16, s1, s2, s4, s8>::found));

    // confirm the entries genuinely have those sizes (the ordering key)
    D_BS_CHECK(sizeof(s1) == 1u);
    D_BS_CHECK(sizeof(s8) == 8u);

    return true;
}

/*
tests_ordering_duplicate_keys_land_deterministically
  When several entries equal the needle, the engine lands, not lower_bounds.
  Tests the following:
  - three entries sharing the needle's key report the BISECTION LANDING index,
    which for {0,1,2} is index 1 (mid = 0 + (3-0)/2)
  - the reported ::type is the entry at that landing index, distinguishable by
    its tag from its equal-keyed neighbours
  - the result is found (a hit, not a miss)
  - a two-way duplicate lands deterministically as well
*/
bool
tests_ordering_duplicate_keys_land_deterministically()
{
    // three entries, all key 5, distinct tags, at indices 0,1,2
    using a = tag_key<5, 100>;
    using b = tag_key<5, 101>;
    using c = tag_key<5, 102>;

    using result = search<5, a, b, c>;

    // the first probe is mid = 1, an immediate hit -> index 1
    D_BS_CHECK(result::found);
    D_BS_CHECK(result::index == 1u);
    D_BS_CHECK((std::is_same<result::type, b>::value));
    D_BS_CHECK(result::type::tag == 101);

    // a two-element duplicate: mid = 0 + (2-0)/2 = 1 -> lands on the second
    using pair_result = search<5, tag_key<5, 200>, tag_key<5, 201> >;

    D_BS_CHECK(pair_result::found);
    D_BS_CHECK(pair_result::index == 1u);
    D_BS_CHECK(pair_result::type::tag == 201);

    return true;
}

/*
tests_ordering_branch_directions_go_right
  An always-below predicate pair drives the search right at every step.
  Tests the following:
  - with below always true (and above false), the engine steps right until the
    range empties, reporting a miss
  - this holds regardless of pack size (a walk off the top)
  - the reported index is lookup_npos
  - the reported ::type is lookup_not_found
*/
bool
tests_ordering_branch_directions_go_right()
{
    // below = always true means every mid "sorts before needle" -> go right;
    // above is never_either (always false), so no step ever goes left.
    using r3 = bsearch_by<always_below, never_either, ikey<1>, ikey<2>, ikey<3> >;

    D_BS_CHECK(!r3::found);
    D_BS_CHECK(r3::index == lookup_npos);
    D_BS_CHECK((std::is_same<r3::type, lookup_not_found>::value));

    // a larger pack still walks off the top
    using r7 = bsearch_by<always_below, never_either,
                          ikey<1>, ikey<2>, ikey<3>, ikey<4>, ikey<5>, ikey<6>, ikey<7> >;

    D_BS_CHECK(!r7::found);
    D_BS_CHECK(r7::index == lookup_npos);

    return true;
}

/*
tests_ordering_branch_directions_go_left
  An always-above predicate pair drives the search left at every step.
  Tests the following:
  - with above always true (and below false), the engine steps left until the
    range empties, reporting a miss
  - this holds regardless of pack size (a collapse to the bottom)
  - the reported index is lookup_npos
  - the reported ::type is lookup_not_found
*/
bool
tests_ordering_branch_directions_go_left()
{
    // above = always true means every mid "sorts after needle" -> go left;
    // below is never_either (always false), so no step ever goes right.
    using l3 = bsearch_by<never_either, always_above,
                          ikey<1>, ikey<2>, ikey<3> >;

    D_BS_CHECK(!l3::found);
    D_BS_CHECK(l3::index == lookup_npos);
    D_BS_CHECK((std::is_same<l3::type, lookup_not_found>::value));

    using l7 = bsearch_by<never_either, always_above,
                          ikey<1>, ikey<2>, ikey<3>, ikey<4>, ikey<5>, ikey<6>, ikey<7> >;

    D_BS_CHECK(!l7::found);
    D_BS_CHECK(l7::index == lookup_npos);

    return true;
}

/*
tests_ordering_vacuous_hit_at_first_probe
  A never-either predicate pair makes the very first probe a hit.
  Tests the following:
  - with both predicates false, the first midpoint is reported as a hit
  - for a five-element pack that midpoint is index 2
  - found is true
  - for a single-element pack the vacuous hit is index 0
*/
bool
tests_ordering_vacuous_hit_at_first_probe()
{
    // both false -> hit at the first mid; for 5 entries mid = 0 + (5-0)/2 = 2
    using five = bsearch_by<never_either, never_either,
                            ikey<1>, ikey<2>, ikey<3>, ikey<4>, ikey<5> >;

    D_BS_CHECK(five::found);
    D_BS_CHECK(five::index == 2u);
    D_BS_CHECK((std::is_same<five::type, ikey<3> >::value));

    // single element: mid = 0
    using one = bsearch_by<never_either, never_either, ikey<42> >;

    D_BS_CHECK(one::found);
    D_BS_CHECK(one::index == 0u);
    D_BS_CHECK((std::is_same<one::type, ikey<42> >::value));

    return true;
}

/*
tests_ordering_needle_absent_from_engine_signature
  The needle lives only in the predicates, never in the engine's parameters.
  Tests the following:
  - two searches of one pack for two different needles instantiate the SAME
    bsearch_by template family, differing only in the predicate arguments
  - each still returns its own correct result
  - swapping the needle does not require a different engine
  - a third needle over the same pack behaves independently
*/
bool
tests_ordering_needle_absent_from_engine_signature()
{
    using e1 = ikey<1>; using e5 = ikey<5>; using e9 = ikey<9>;

    // Both of these are bsearch_by<SomeBelow, SomeAbove, e1, e5, e9> -- the
    // pack is identical; only the baked-in needle differs.  Distinct results
    // from one engine shape is the whole point.
    D_BS_CHECK((search<1, e1, e5, e9>::index) == 0u);
    D_BS_CHECK((search<9, e1, e5, e9>::index) == 2u);
    D_BS_CHECK((search<5, e1, e5, e9>::index) == 1u);

    // the matched types differ accordingly
    D_BS_CHECK((std::is_same<search<1, e1, e5, e9>::type, e1>::value));
    D_BS_CHECK((std::is_same<search<9, e1, e5, e9>::type, e9>::value));

    // and a needle that misses over the same pack is independent
    D_BS_CHECK(!(search<4, e1, e5, e9>::found));

    return true;
}

NS_END  // testing
NS_END  // djinterp
