/*******************************************************************************
* djinterp [core]                                  pack_element_tests_bounds.cpp
*
*
* path:      /tests/djinterp/core/meta/pack_element_tests_bounds.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "pack_element_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_pack_element_arity_contract
  Where the out-of-range error lives, and which half of it is testable.
  Tests the following:
  - NAMING pack_element is well formed for any index, in range or not,
    because naming instantiates nothing
  - naming it with an EMPTY pack is a substitution failure, because
    too-few-template-arguments is diagnosed during naming; this is the one
    part of the boundary that can be detected rather than merely suffered
  - resolving ::type succeeds at every in-range index
  NOTE: the complementary case - resolving ::type on an OUT-OF-RANGE index
  - is deliberately absent, and cannot be added.  It instantiates the
  primary template, whose member names pack_element with a shorter pack
  until the pack is empty; that failure occurs inside an instantiation, so
  it is a hard error rather than a substitution failure and can be neither
  caught nor probed.  Stating it would fail the BUILD rather than fail a
  test.  The header documents this, and the guard exercised in
  tests_pack_element_guarded_access is the prescribed remedy.
*/
bool
tests_pack_element_arity_contract()
{
    bool ok = true;

    // the signatures, checked by binding rather than instantiating
    D_PE_CHECK(ok, pe_binds_indexed<pack_element>::value);
    D_PE_CHECK(ok, pe_binds_alias<pack_element_t>::value);

    // naming is always well formed once there is at least one element,
    // whatever the index - nothing is instantiated by a name
    D_PE_CHECK(ok, pe_is_nameable<0, int>::value);
    D_PE_CHECK(ok, pe_is_nameable<1, int>::value);
    D_PE_CHECK(ok, pe_is_nameable<9, int>::value);
    D_PE_CHECK(ok, (pe_is_nameable<1000, int, char>::value));

    // an empty pack cannot be named at all - the arity requirement is a
    // genuine substitution failure
    D_PE_CHECK(ok, !pe_is_nameable<0>::value);
    D_PE_CHECK(ok, !pe_is_nameable<1>::value);
    D_PE_CHECK(ok, !pe_is_nameable<9>::value);

    // ... and it is caught before any instantiation is attempted, which
    // is why asking for ::type on it is also safe to state
    D_PE_CHECK(ok, !pe_is_resolvable<0>::value);
    D_PE_CHECK(ok, !pe_is_resolvable<1>::value);

    // every IN-RANGE index resolves
    D_PE_CHECK(ok, pe_is_resolvable<0, int>::value);
    D_PE_CHECK(ok, (pe_is_resolvable<0, int, char>::value));
    D_PE_CHECK(ok, (pe_is_resolvable<1, int, char>::value));
    D_PE_CHECK(ok, (pe_is_resolvable<3, pe_first, pe_second, pe_third,
                                     pe_fourth>::value));

    return ok;
}

/*
tests_pack_element_guarded_access
  The remedy the header prescribes for a caller that may pass a bad index.
  Tests the following:
  - an in-range index returns exactly what direct access returns, at every
    index of a pack
  - an out-of-range index returns the fallback INSTEAD OF failing the
    build, which is the whole point: the guard's false branch never names
    pack_element, so the hard error never has the chance to fire
  - the boundary is where it should be - the last valid index is size - 1,
    and size itself already falls back
  - an empty pack falls back too, which matters because that is the one
    case pack_element cannot even be named for
  - the fallback may be any type at all, including void
  This mirrors pack_element_or in bsearch.hpp; the pattern is tested here
  so that the header's advice is verified rather than assumed.
*/
bool
tests_pack_element_guarded_access()
{
    bool ok = true;

    // in range: identical to direct access
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<0, void, pe_first, pe_second,
                                     pe_third>,
                       pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<1, void, pe_first, pe_second,
                                     pe_third>,
                       pe_second>);
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<2, void, pe_first, pe_second,
                                     pe_third>,
                       pe_third>);
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<2, void, pe_first, pe_second,
                                     pe_third>,
                       pack_element_t<2, pe_first, pe_second, pe_third>>);

    // the boundary: size - 1 is the last valid index, size is not
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<2, pe_fourth, pe_first, pe_second,
                                     pe_third>,
                       pe_third>);
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<3, pe_fourth, pe_first, pe_second,
                                     pe_third>,
                       pe_fourth>);

    // far out of range - no hard error, just the fallback
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<9, pe_fourth, pe_first, pe_second>,
                       pe_fourth>);
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<1000, pe_fourth, pe_first>,
                       pe_fourth>);

    // the empty pack, which pack_element cannot even be named for
    D_PE_CHECK(ok, std::is_same_v<pe_element_or<0, pe_fourth>, pe_fourth>);
    D_PE_CHECK(ok, std::is_same_v<pe_element_or<7, pe_fourth>, pe_fourth>);

    // the fallback may be any type, including ones that cannot be
    // instantiated
    D_PE_CHECK(ok, std::is_same_v<pe_element_or<5, void, pe_first>, void>);
    D_PE_CHECK(ok, std::is_same_v<pe_element_or<5, pe_incomplete,
                                                pe_first>,
                                  pe_incomplete>);
    D_PE_CHECK(ok, std::is_same_v<pe_element_or<5, int(int), pe_first>,
                                  int(int)>);

    // and an out-of-range guard over a pack of awkward elements is still
    // only a fallback - the elements are never touched
    D_PE_CHECK(ok, std::is_same_v<
                       pe_element_or<99, pe_fourth,
                                     D_TEST_HOSTILE_TYPES>,
                       pe_fourth>);

    return ok;
}

/*
tests_pack_element_depth
  Recursion depth as a parameter rather than a hand-written battery.
  Tests the following:
  - for packs of 1, 2, 8, 32, 64, 128 and 256 distinct elements, EVERY
    index answers with its own element.  The fold behind pe_indices_match
    has no short circuit, so a 256-element pack really does instantiate
    all 256 accesses, each recursing up to 256 deep
  - the extreme indices of a long pack are correct, which is where an
    off-by-one in the recursion would surface first
  - the module needs no explicit depth guard: it terminates purely on the
    base-case specialization, and nothing here approaches a compiler's
    default instantiation-depth limit
*/
bool
tests_pack_element_depth()
{
    bool ok = true;

    // every index of packs of increasing length
    D_PE_CHECK(ok, pe_indices_match<1>::value);
    D_PE_CHECK(ok, pe_indices_match<2>::value);
    D_PE_CHECK(ok, pe_indices_match<8>::value);
    D_PE_CHECK(ok, pe_indices_match<32>::value);
    D_PE_CHECK(ok, pe_indices_match<64>::value);
    D_PE_CHECK(ok, pe_indices_match<128>::value);
    D_PE_CHECK(ok, pe_indices_match<256>::value);

    // the extremes of a long pack, stated on their own
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<0, pe_tag_list<256>>::type,
                       pe_tag<0>>);
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<255, pe_tag_list<256>>::type,
                       pe_tag<255>>);
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<128, pe_tag_list<256>>::type,
                       pe_tag<128>>);

    // an off-by-one at either end would show here
    D_PE_CHECK(ok, !std::is_same_v<
                       typename pe_at_list<255, pe_tag_list<256>>::type,
                       pe_tag<254>>);
    D_PE_CHECK(ok, !std::is_same_v<
                       typename pe_at_list<0, pe_tag_list<256>>::type,
                       pe_tag<1>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
