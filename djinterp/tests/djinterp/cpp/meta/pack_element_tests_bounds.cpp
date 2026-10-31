/*******************************************************************************
* djinterp [cpp]                                   pack_element_tests_bounds.cpp
*
*
* path:      /tests/djinterp/cpp/meta/pack_element_tests_bounds.cpp
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
bool tests_pack_element_arity_contract()
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
    D_PE_CHECK(ok, (pe_is_resolvable<3, DPeFirst, DPeSecond, DPeThird,
                                     DPeFourth>::value));

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
bool tests_pack_element_guarded_access()
{
    bool ok = true;

    // in range: identical to direct access
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<0, void, DPeFirst, DPeSecond,
                                     DPeThird>,
                       DPeFirst>);
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<1, void, DPeFirst, DPeSecond,
                                     DPeThird>,
                       DPeSecond>);
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<2, void, DPeFirst, DPeSecond,
                                     DPeThird>,
                       DPeThird>);
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<2, void, DPeFirst, DPeSecond,
                                     DPeThird>,
                       pack_element_t<2, DPeFirst, DPeSecond, DPeThird>>);

    // the boundary: size - 1 is the last valid index, size is not
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<2, DPeFourth, DPeFirst, DPeSecond,
                                     DPeThird>,
                       DPeThird>);
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<3, DPeFourth, DPeFirst, DPeSecond,
                                     DPeThird>,
                       DPeFourth>);

    // far out of range - no hard error, just the fallback
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<9, DPeFourth, DPeFirst, DPeSecond>,
                       DPeFourth>);
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<1000, DPeFourth, DPeFirst>,
                       DPeFourth>);

    // the empty pack, which pack_element cannot even be named for
    D_PE_CHECK(ok, std::is_same_v<DPeElementOr<0, DPeFourth>, DPeFourth>);
    D_PE_CHECK(ok, std::is_same_v<DPeElementOr<7, DPeFourth>, DPeFourth>);

    // the fallback may be any type, including ones that cannot be
    // instantiated
    D_PE_CHECK(ok, std::is_same_v<DPeElementOr<5, void, DPeFirst>, void>);
    D_PE_CHECK(ok, std::is_same_v<DPeElementOr<5, pe_incomplete,
                                                DPeFirst>,
                                  pe_incomplete>);
    D_PE_CHECK(ok, std::is_same_v<DPeElementOr<5, int(int), DPeFirst>,
                                  int(int)>);

    // and an out-of-range guard over a pack of awkward elements is still
    // only a fallback - the elements are never touched
    D_PE_CHECK(ok, std::is_same_v<
                       DPeElementOr<99, DPeFourth,
                                     D_TEST_HOSTILE_TYPES>,
                       DPeFourth>);

    return ok;
}

/*
tests_pack_element_depth
  Recursion depth as a parameter rather than a hand-written battery.
  Tests the following:
  - for packs of 1, 2, 8, 32, 64, 128 and 256 distinct elements, EVERY
    index answers with its own element.  The fold behind DPeIndicesMatch
    has no short circuit, so a 256-element pack really does instantiate
    all 256 accesses, each recursing up to 256 deep
  - the extreme indices of a long pack are correct, which is where an
    off-by-one in the recursion would surface first
  - the module needs no explicit depth guard: it terminates purely on the
    base-case specialization, and nothing here approaches a compiler's
    default instantiation-depth limit
*/
bool tests_pack_element_depth()
{
    bool ok = true;

    // every index of packs of increasing length
    D_PE_CHECK(ok, DPeIndicesMatch<1>::value);
    D_PE_CHECK(ok, DPeIndicesMatch<2>::value);
    D_PE_CHECK(ok, DPeIndicesMatch<8>::value);
    D_PE_CHECK(ok, DPeIndicesMatch<32>::value);
    D_PE_CHECK(ok, DPeIndicesMatch<64>::value);
    D_PE_CHECK(ok, DPeIndicesMatch<128>::value);
    D_PE_CHECK(ok, DPeIndicesMatch<256>::value);

    // the extremes of a long pack, stated on their own
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<0, DPeTagList<256>>::type,
                       pe_tag<0>>);
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<255, DPeTagList<256>>::type,
                       pe_tag<255>>);
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<128, DPeTagList<256>>::type,
                       pe_tag<128>>);

    // an off-by-one at either end would show here
    D_PE_CHECK(ok, !std::is_same_v<
                       typename pe_at_list<255, DPeTagList<256>>::type,
                       pe_tag<254>>);
    D_PE_CHECK(ok, !std::is_same_v<
                       typename pe_at_list<0, DPeTagList<256>>::type,
                       pe_tag<1>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
