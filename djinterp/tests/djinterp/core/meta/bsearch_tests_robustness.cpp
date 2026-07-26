#include "bsearch_tests.hpp"


NS_DJINTERP
NS_TESTING

// -- a shared sixteen-element ascending pack (odd keys 1,3,...,31) ------------
//   Sixteen entries exercise four levels of bisection.  Named once here so the
// two large-pack tests below read cleanly.  Using `scaled` (distinct from ikey)
// keeps these entries visibly separate from the small-pack fixtures.
namespace
{
    using w0  = scaled<1>;   using w1  = scaled<3>;   using w2  = scaled<5>;
    using w3  = scaled<7>;   using w4  = scaled<9>;   using w5  = scaled<11>;
    using w6  = scaled<13>;  using w7  = scaled<15>;  using w8  = scaled<17>;
    using w9  = scaled<19>;  using w10 = scaled<21>;  using w11 = scaled<23>;
    using w12 = scaled<25>;  using w13 = scaled<27>;  using w14 = scaled<29>;
    using w15 = scaled<31>;
}

// wide
//   alias: the sixteen-element search, needle baked in, so each assertion below
// is a single readable line.
template<int _Needle>
using wide =
    bsearch_by<asc_preds<_Needle>::template below,
               asc_preds<_Needle>::template above,
               w0, w1, w2, w3, w4, w5, w6, w7,
               w8, w9, w10, w11, w12, w13, w14, w15>;

/*
tests_robustness_large_pack_all_hits
  Every key in a sixteen-element pack is found at its own index.
  Tests the following:
  - the sixteen odd keys 1..31 each resolve to indices 0..15 in order
  - found is true for the extremes and several interior probes
  - the deep (four-level) bisection lands exactly, not off by one
  - a representative matched ::type is the expected entry
*/
bool
tests_robustness_large_pack_all_hits()
{
    D_BS_CHECK(wide<1>::index  == 0u);
    D_BS_CHECK(wide<3>::index  == 1u);
    D_BS_CHECK(wide<5>::index  == 2u);
    D_BS_CHECK(wide<7>::index  == 3u);
    D_BS_CHECK(wide<9>::index  == 4u);
    D_BS_CHECK(wide<11>::index == 5u);
    D_BS_CHECK(wide<13>::index == 6u);
    D_BS_CHECK(wide<15>::index == 7u);
    D_BS_CHECK(wide<17>::index == 8u);
    D_BS_CHECK(wide<19>::index == 9u);
    D_BS_CHECK(wide<21>::index == 10u);
    D_BS_CHECK(wide<23>::index == 11u);
    D_BS_CHECK(wide<25>::index == 12u);
    D_BS_CHECK(wide<27>::index == 13u);
    D_BS_CHECK(wide<29>::index == 14u);
    D_BS_CHECK(wide<31>::index == 15u);

    D_BS_CHECK(wide<1>::found);
    D_BS_CHECK(wide<31>::found);
    D_BS_CHECK(wide<15>::found);
    D_BS_CHECK(wide<17>::found);

    // a representative matched type
    D_BS_CHECK((std::is_same<wide<17>::type, w8>::value));

    return true;
}

/*
tests_robustness_large_pack_boundaries_and_gaps
  In the same wide pack, extremes hit and every kind of absent needle misses.
  Tests the following:
  - the minimum and maximum keys are found at the ends
  - a needle below all keys misses
  - a needle above all keys misses
  - several even-valued needles (all in interior gaps) miss
*/
bool
tests_robustness_large_pack_boundaries_and_gaps()
{
    // extremes
    D_BS_CHECK(wide<1>::found);
    D_BS_CHECK(wide<1>::index == 0u);
    D_BS_CHECK(wide<31>::found);
    D_BS_CHECK(wide<31>::index == 15u);

    // below all / above all
    D_BS_CHECK(!wide<0>::found);
    D_BS_CHECK(wide<0>::index == lookup_npos);
    D_BS_CHECK(!wide<100>::found);
    D_BS_CHECK(wide<100>::index == lookup_npos);

    // interior gaps: every even number 2..30 is absent
    D_BS_CHECK(!wide<2>::found);
    D_BS_CHECK(!wide<8>::found);
    D_BS_CHECK(!wide<16>::found);
    D_BS_CHECK(!wide<24>::found);
    D_BS_CHECK(!wide<30>::found);

    D_BS_CHECK(wide<16>::index == lookup_npos);
    D_BS_CHECK((std::is_same<wide<16>::type, lookup_not_found>::value));

    return true;
}

/*
tests_robustness_members_usable_as_template_arguments
  ::index and ::found are constant expressions on both a hit and a miss.
  Tests the following:
  - a hit's index instantiates a std::size_t-parameterised template
  - a hit's found instantiates a bool-parameterised template
  - a miss's index (lookup_npos) does the same -- npos is still a constant
  - the instantiated constants equal the members
*/
bool
tests_robustness_members_usable_as_template_arguments()
{
    using hit  = wide<17>;
    using miss = wide<16>;

    using hit_idx  = index_is_constexpr<hit::index>;
    using hit_fnd  = consumes_bool<hit::found>;
    using miss_idx = index_is_constexpr<miss::index>;
    using miss_fnd = consumes_bool<miss::found>;

    D_BS_CHECK(hit_idx::value == hit::index);
    D_BS_CHECK(hit_idx::value == 8u);
    D_BS_CHECK(hit_fnd::value == hit::found);

    D_BS_CHECK(miss_idx::value == miss::index);
    D_BS_CHECK(miss_idx::value == lookup_npos);
    D_BS_CHECK(miss_fnd::value == miss::found);

    static_assert(hit::index == 8u, "hit index is constexpr");
    static_assert(miss::index == lookup_npos, "miss index is constexpr");

    return true;
}

/*
tests_robustness_repeated_instantiation_is_stable
  The trait is a pure function of its arguments.
  Tests the following:
  - the same search named twice yields one identical ::type
  - it yields one identical ::index
  - a hit and its repeat agree on found
  - a miss and its repeat agree on the sentinel
*/
bool
tests_robustness_repeated_instantiation_is_stable()
{
    // naming wide<9> twice must be the same trait
    D_BS_CHECK((std::is_same<wide<9>::type, wide<9>::type>::value));
    D_BS_CHECK(wide<9>::index == wide<9>::index);
    D_BS_CHECK(wide<9>::found == wide<9>::found);
    D_BS_CHECK(wide<9>::index == 4u);

    // a miss is equally stable
    D_BS_CHECK((std::is_same<wide<16>::type, wide<16>::type>::value));
    D_BS_CHECK(wide<16>::index == wide<16>::index);
    D_BS_CHECK(wide<16>::index == lookup_npos);

    return true;
}

/*
tests_robustness_distinct_needles_distinct_results
  Distinct needles over one pack give distinct results (no cross-instantiation
  bleed).
  Tests the following:
  - two hits at different keys report different indices
  - and different matched types
  - a hit and a miss over the same pack differ in found and in ::type
  - three distinct hits give three distinct indices
*/
bool
tests_robustness_distinct_needles_distinct_results()
{
    // different indices
    D_BS_CHECK(wide<5>::index != wide<25>::index);
    D_BS_CHECK(wide<5>::index == 2u);
    D_BS_CHECK(wide<25>::index == 12u);

    // different matched types
    D_BS_CHECK(!(std::is_same<wide<5>::type, wide<25>::type>::value));

    // a hit differs from a miss
    D_BS_CHECK(wide<5>::found != wide<6>::found);
    D_BS_CHECK(!(std::is_same<wide<5>::type, wide<6>::type>::value));

    // three distinct hits, three distinct indices
    D_BS_CHECK(wide<1>::index != wide<15>::index);
    D_BS_CHECK(wide<15>::index != wide<31>::index);
    D_BS_CHECK(wide<1>::index != wide<31>::index);

    return true;
}

NS_END  // testing
NS_END  // djinterp
