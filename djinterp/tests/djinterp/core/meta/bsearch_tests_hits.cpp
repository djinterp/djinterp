#include "bsearch_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_hits_single_element
  A one-element pack whose key equals the needle.
  Tests the following:
  - the reported index is 0
  - found is true
  - the reported ::type is the sole entry
  - bsearch_by_t agrees with ::type
*/
bool
tests_hits_single_element()
{
    using result = search<5, ikey<5> >;

    D_BS_CHECK(result::index == 0u);
    D_BS_CHECK(result::found);
    D_BS_CHECK((std::is_same<result::type, ikey<5> >::value));

    // the alias tracks the trait
    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<5>::template below,
                                 asc_preds<5>::template above, ikey<5> >,
                    ikey<5> >::value));

    return true;
}

/*
tests_hits_first_last_and_middle
  The three canonical positions in a five-element pack (keys 1,3,5,7,9).
  Tests the following:
  - the first key is found at index 0
  - the middle key is found at index 2
  - the last key is found at index 4
  - each reports the matching entry as ::type
*/
bool
tests_hits_first_last_and_middle()
{
    // keys 1,3,5,7,9 at indices 0..4
    using e1 = ikey<1>; using e3 = ikey<3>; using e5 = ikey<5>;
    using e7 = ikey<7>; using e9 = ikey<9>;

    // first
    D_BS_CHECK((search<1, e1, e3, e5, e7, e9>::index) == 0u);
    D_BS_CHECK((std::is_same<search<1, e1, e3, e5, e7, e9>::type, e1>::value));

    // middle
    D_BS_CHECK((search<5, e1, e3, e5, e7, e9>::index) == 2u);
    D_BS_CHECK((std::is_same<search<5, e1, e3, e5, e7, e9>::type, e5>::value));

    // last
    D_BS_CHECK((search<9, e1, e3, e5, e7, e9>::index) == 4u);
    D_BS_CHECK((std::is_same<search<9, e1, e3, e5, e7, e9>::type, e9>::value));

    return true;
}

/*
tests_hits_every_position_odd_count
  Sweep all five positions of an odd-length pack.
  Tests the following:
  - each of the five keys is found
  - each is found at exactly its own index
  - all four non-trivial indices are covered, not just the midpoint
  - found is true for every one
*/
bool
tests_hits_every_position_odd_count()
{
    using e1 = ikey<1>; using e3 = ikey<3>; using e5 = ikey<5>;
    using e7 = ikey<7>; using e9 = ikey<9>;

    D_BS_CHECK((search<1, e1, e3, e5, e7, e9>::index) == 0u);
    D_BS_CHECK((search<3, e1, e3, e5, e7, e9>::index) == 1u);
    D_BS_CHECK((search<5, e1, e3, e5, e7, e9>::index) == 2u);
    D_BS_CHECK((search<7, e1, e3, e5, e7, e9>::index) == 3u);
    D_BS_CHECK((search<9, e1, e3, e5, e7, e9>::index) == 4u);

    D_BS_CHECK((search<1, e1, e3, e5, e7, e9>::found));
    D_BS_CHECK((search<3, e1, e3, e5, e7, e9>::found));
    D_BS_CHECK((search<5, e1, e3, e5, e7, e9>::found));
    D_BS_CHECK((search<7, e1, e3, e5, e7, e9>::found));
    D_BS_CHECK((search<9, e1, e3, e5, e7, e9>::found));

    return true;
}

/*
tests_hits_every_position_even_count
  Sweep all six positions of an even-length pack (keys 2,4,6,8,10,12).
  Tests the following:
  - each of the six keys is found at its own index
  - the even-count bisection (mid rounds down) still reaches every position
  - the reported entry matches at two representative positions
  - found is true throughout
*/
bool
tests_hits_every_position_even_count()
{
    using e2  = ikey<2>;  using e4  = ikey<4>;  using e6  = ikey<6>;
    using e8  = ikey<8>;  using e10 = ikey<10>; using e12 = ikey<12>;

    D_BS_CHECK((search<2,  e2, e4, e6, e8, e10, e12>::index) == 0u);
    D_BS_CHECK((search<4,  e2, e4, e6, e8, e10, e12>::index) == 1u);
    D_BS_CHECK((search<6,  e2, e4, e6, e8, e10, e12>::index) == 2u);
    D_BS_CHECK((search<8,  e2, e4, e6, e8, e10, e12>::index) == 3u);
    D_BS_CHECK((search<10, e2, e4, e6, e8, e10, e12>::index) == 4u);
    D_BS_CHECK((search<12, e2, e4, e6, e8, e10, e12>::index) == 5u);

    D_BS_CHECK((std::is_same<search<2,  e2, e4, e6, e8, e10, e12>::type, e2>::value));
    D_BS_CHECK((std::is_same<search<12, e2, e4, e6, e8, e10, e12>::type, e12>::value));

    D_BS_CHECK((search<6, e2, e4, e6, e8, e10, e12>::found));
    D_BS_CHECK((search<8, e2, e4, e6, e8, e10, e12>::found));

    return true;
}

/*
tests_hits_result_members_are_consistent
  ::type, ::found and ::index tell one coherent story on a hit.
  Tests the following:
  - found is true
  - index is not lookup_npos
  - pack_element_t at the reported index is the reported ::type
  - the same holds at a different hit position
*/
bool
tests_hits_result_members_are_consistent()
{
    using e1 = ikey<1>; using e3 = ikey<3>; using e5 = ikey<5>;
    using e7 = ikey<7>; using e9 = ikey<9>;

    using at5 = search<5, e1, e3, e5, e7, e9>;

    D_BS_CHECK(at5::found);
    D_BS_CHECK(at5::index != lookup_npos);
    D_BS_CHECK((std::is_same<at5::type,
                             pack_element_t<at5::index, e1, e3, e5, e7, e9> >::value));

    using at9 = search<9, e1, e3, e5, e7, e9>;

    D_BS_CHECK(at9::found);
    D_BS_CHECK(at9::index != lookup_npos);
    D_BS_CHECK((std::is_same<at9::type,
                             pack_element_t<at9::index, e1, e3, e5, e7, e9> >::value));

    return true;
}

/*
tests_hits_type_alias_matches_entry
  bsearch_by_t is a faithful shorthand on a hit.
  Tests the following:
  - bsearch_by_t equals bsearch_by<...>::type
  - both name the matched entry
  - the agreement holds at another position
  - the alias resolves to a concrete entry, not to itself
*/
bool
tests_hits_type_alias_matches_entry()
{
    using e1 = ikey<1>; using e3 = ikey<3>; using e5 = ikey<5>;

    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<3>::template below,
                                 asc_preds<3>::template above, e1, e3, e5>,
                    search<3, e1, e3, e5>::type>::value));

    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<3>::template below,
                                 asc_preds<3>::template above, e1, e3, e5>,
                    e3>::value));

    D_BS_CHECK((std::is_same<
                    bsearch_by_t<asc_preds<1>::template below,
                                 asc_preds<1>::template above, e1, e3, e5>,
                    e1>::value));

    return true;
}

/*
tests_hits_two_element_pack
  The smallest pack with an actual left / right choice.
  Tests the following:
  - the first key is found at index 0
  - the second key is found at index 1
  - a needle below both misses
  - a needle between them and one above both miss
*/
bool
tests_hits_two_element_pack()
{
    using e3 = ikey<3>; using e7 = ikey<7>;

    // hits
    D_BS_CHECK((search<3, e3, e7>::index) == 0u);
    D_BS_CHECK((search<7, e3, e7>::index) == 1u);
    D_BS_CHECK((search<3, e3, e7>::found));
    D_BS_CHECK((search<7, e3, e7>::found));

    // misses around and between
    D_BS_CHECK(!(search<1, e3, e7>::found));   // below both
    D_BS_CHECK(!(search<5, e3, e7>::found));   // between
    D_BS_CHECK(!(search<9, e3, e7>::found));   // above both

    return true;
}

/*
tests_hits_index_is_a_constant_expression
  A hit's ::index is a genuine compile-time constant.
  Tests the following:
  - ::index instantiates a std::size_t-parameterised template (only a constant
    can be a template argument)
  - the instantiated constant equals the index
  - ::found likewise drives a bool-parameterised template
  - both hold for a concrete hit
*/
bool
tests_hits_index_is_a_constant_expression()
{
    using e1 = ikey<1>; using e5 = ikey<5>; using e9 = ikey<9>;
    using at5 = search<5, e1, e5, e9>;

    // forming these types at all proves the members are constant expressions
    using idx = index_is_constexpr<at5::index>;
    using fnd = consumes_bool<at5::found>;

    D_BS_CHECK(idx::value == at5::index);
    D_BS_CHECK(idx::value == 1u);
    D_BS_CHECK(fnd::value == at5::found);
    D_BS_CHECK(fnd::value);

    static_assert(at5::index == 1u, "hit index must be a constant expression");
    static_assert(at5::found, "hit found must be a constant expression");

    return true;
}

NS_END  // testing
NS_END  // djinterp
