/*******************************************************************************
* djinterp [core]                                pack_element_tests_indexing.cpp
*
*
* path:      /tests/djinterp/core/meta/pack_element_tests_indexing.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "pack_element_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_pack_element_base_case
  The index-0 specialization.
  Tests the following:
  - index 0 yields the head, for a one-element pack and for packs of every
    other length
  - the answer does not depend on the tail's LENGTH or on its CONTENT: an
    incomplete type, a void, a function type sitting behind the head all
    leave index 0 alone
  - the specialization really is what answers index 0.  Were the primary
    template selected instead, it would compute _Index - 1 on an unsigned
    zero, wrap to the maximum std::size_t, and run off the end of the
    pack; every check in this function would be a hard error rather than
    a false answer.  That this function compiles at all is the claim
*/
bool
tests_pack_element_base_case()
{
    bool ok = true;

    // the minimum case: one element
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, pe_first>, pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       typename pack_element<0, pe_first>::type, pe_first>);

    // independent of the tail's length
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_second>, pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_second, pe_third>,
                       pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_second, pe_third,
                                      pe_fourth>,
                       pe_first>);

    // independent of the tail's content - the tail is carried, not read
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_incomplete>,
                       pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, void>, pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, int(int)>, pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, int[]>, pe_first>);
    D_PE_CHECK(ok, ::djinterp::test::holds_for_all<
                       pe_head_survives_tail,
                       D_TEST_HOSTILE_TYPES>::value);

    // the head itself may be any type, including one that is never
    // completed
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, void>, void>);
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, pe_incomplete>,
                                  pe_incomplete>);
    D_PE_CHECK(ok, ::djinterp::test::holds_for_all<
                       pe_singleton_holds, D_TEST_HOSTILE_TYPES>::value);

    // and index 0 of a very long pack is still just the head
    D_PE_CHECK(ok, std::is_same_v<
                       typename pe_at_list<0, pe_tag_list<128>>::type,
                       pe_tag<0>>);

    return ok;
}

/*
tests_pack_element_recursion
  The primary template - one head peeled per step.
  Tests the following:
  - every interior index of a four-element pack answers with its own
    element, and the last index answers with the last element
  - the RECURSION EQUATION itself holds: indexing a pack at K is the same
    as indexing its tail at K - 1.  That is the implementation's defining
    property, so it is stated directly rather than inferred from a table
    of answers
  - walking consecutive indices walks consecutive elements, so the
    mapping is order-preserving rather than merely correct pointwise
*/
bool
tests_pack_element_recursion()
{
    bool ok = true;

    // every index of a four-element pack
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, pe_first, pe_second, pe_third,
                                      pe_fourth>,
                       pe_second>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_first, pe_second, pe_third,
                                      pe_fourth>,
                       pe_third>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<3, pe_first, pe_second, pe_third,
                                      pe_fourth>,
                       pe_fourth>);

    // the last index of a two-element pack - the shortest recursion
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, pe_first, pe_second>, pe_second>);

    // the recursion equation: index K of a pack is index K - 1 of its tail
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<3, pe_first, pe_second, pe_third,
                                      pe_fourth>,
                       pack_element_t<2, pe_second, pe_third, pe_fourth>>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_second, pe_third, pe_fourth>,
                       pack_element_t<1, pe_third, pe_fourth>>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, pe_third, pe_fourth>,
                       pack_element_t<0, pe_fourth>>);

    // a wrong index is a visible failure, not an accidental match
    D_PE_CHECK(ok, !std::is_same_v<
                       pack_element_t<1, pe_first, pe_second>, pe_first>);
    D_PE_CHECK(ok, !std::is_same_v<
                       pack_element_t<2, pe_first, pe_second, pe_third>,
                       pe_second>);

    // consecutive indices walk consecutive elements
    D_PE_CHECK(ok, std::is_same_v<
                       pe_rebuild<pe_first, pe_second, pe_third,
                                  pe_fourth>::via_alias,
                       pe_origin<pe_first, pe_second, pe_third,
                                 pe_fourth>>);

    // the recursive case carries an arbitrary element intact
    D_PE_CHECK(ok, ::djinterp::test::holds_for_all<
                       pe_after_head_holds,
                       D_TEST_HOSTILE_TYPES>::value);

    return ok;
}

/*
tests_pack_element_trait_shape
  pack_element AS A TYPE, independent of the answer it carries.
  Tests the following:
  - it is an empty, stateless, trivially constructible aggregate that
    exposes exactly one member, `type`
  - it carries no ::value and is therefore NOT a boolean trait, which
    matters because the surrounding codebase's quantifiers accept only
    boolean traits and would otherwise silently accept this one
  - two instantiations naming the same index and pack are the same type,
    and instantiations differing in either are distinct - so the template
    is a genuine function of both arguments
  - two DIFFERENT instantiations may still yield the same ::type, which
    is the distinction between the trait and its answer
*/
bool
tests_pack_element_trait_shape()
{
    bool ok = true;

    using at0 = pack_element<0, pe_first, pe_second>;
    using at1 = pack_element<1, pe_first, pe_second>;

    // an empty, stateless tag carrying one member type
    D_PE_CHECK(ok, std::is_class_v<at0>);
    D_PE_CHECK(ok, std::is_empty_v<at0>);
    D_PE_CHECK(ok, sizeof(at0) == 1);
    D_PE_CHECK(ok, std::is_aggregate_v<at0>);
    D_PE_CHECK(ok, std::is_trivially_default_constructible_v<at0>);
    D_PE_CHECK(ok, std::is_trivially_destructible_v<at0>);
    D_PE_CHECK(ok, std::is_standard_layout_v<at0>);

    // it is not a boolean trait, and must not be mistaken for one
    D_PE_CHECK(ok, !::djinterp::test::is_bool_trait<at0>::value);
    D_PE_CHECK(ok, !::djinterp::test::is_bool_trait<at1>::value);

    // a genuine function of BOTH arguments
    D_PE_CHECK(ok, std::is_same_v<at0,
                                  pack_element<0, pe_first, pe_second>>);
    D_PE_CHECK(ok, !std::is_same_v<at0, at1>);
    D_PE_CHECK(ok, !std::is_same_v<
                       pack_element<0, pe_first>,
                       pack_element<0, pe_first, pe_second>>);

    // ... yet distinct instantiations may agree on the answer
    D_PE_CHECK(ok, std::is_same_v<
                       typename pack_element<0, pe_first>::type,
                       typename pack_element<0, pe_first,
                                             pe_second>::type>);
    D_PE_CHECK(ok, std::is_same_v<
                       typename pack_element<1, pe_first, pe_second>::type,
                       typename pack_element<0, pe_second>::type>);

    // the base-case specialization and the primary are the same template
    D_PE_CHECK(ok, pe_binds_indexed<pack_element>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
