/*******************************************************************************
* djinterp [core]                                   pack_element_tests_types.cpp
*
*
* path:      /tests/djinterp/core/meta/pack_element_tests_types.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "pack_element_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_pack_element_type_preservation
  Elements come back EXACTLY as they went in.
  Tests the following:
  - no decay of any kind: an array stays an array rather than becoming a
    pointer, a function stays a function rather than becoming a function
    pointer, and cv- and reference-qualifiers are neither added nor
    stripped
  - the awkward corners of the type system all survive - void and const
    void, bounded and unbounded arrays, lvalue and rvalue references,
    function references, member object and member function pointers, and
    a type that is never completed
  - they survive at index 0 and at an interior index alike, so neither
    the base case nor the recursive case launders its element
*/
bool
tests_pack_element_type_preservation()
{
    bool ok = true;

    // no decay - the two classic laundering bugs
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, int[3]>, int[3]>);
    D_PE_CHECK(ok, !std::is_same_v<pack_element_t<0, int[3]>, int*>);
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, int(int)>, int(int)>);
    D_PE_CHECK(ok, !std::is_same_v<pack_element_t<0, int(int)>,
                                   int(*)(int)>);

    // cv-qualifiers are neither added nor stripped
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, const int>, const int>);
    D_PE_CHECK(ok, !std::is_same_v<pack_element_t<0, const int>, int>);
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, int>, int>);
    D_PE_CHECK(ok, !std::is_same_v<pack_element_t<0, int>, const int>);

    // references are preserved, and the two categories stay distinct
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, int&>, int&>);
    D_PE_CHECK(ok, std::is_same_v<pack_element_t<0, int&&>, int&&>);
    D_PE_CHECK(ok, !std::is_same_v<pack_element_t<0, int&>, int&&>);

    // one pack carrying every awkward type at once, checked at every
    // index and through all three routes
    D_PE_CHECK(ok, (pe_routes_agree<const int,
                                    volatile int,
                                    const volatile int,
                                    int&,
                                    int&&,
                                    int[3],
                                    int[],
                                    int(int),
                                    int(*)(int),
                                    int(&)(int),
                                    void,
                                    const void,
                                    std::nullptr_t,
                                    int pe_first::*,
                                    void (pe_first::*)(),
                                    pe_incomplete>::value));

    // the same types at an INTERIOR index, so the recursive case is
    // exercised on each of them rather than only the base case
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<3, pe_first, pe_second, pe_third,
                                      int[3]>,
                       int[3]>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_first, pe_second, int(int)>,
                       int(int)>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_first, pe_second, void>, void>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_first, pe_second,
                                      pe_incomplete>,
                       pe_incomplete>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, pe_first, int&&>, int&&>);

    return ok;
}

/*
tests_pack_element_hostile_pack
  The whole adversarial type zoo as a single pack.
  Tests the following:
  - rebuilding the zoo index by index through pack_element_t reproduces it
    exactly, which states "every index of a 26-element pack of maximally
    awkward types is right" as one claim
  - the struct route agrees, and so does std::tuple_element_t - an
    independent implementation of the same mapping, so a shared bug in
    the two djinterp spellings would still be caught
  - the fold has no short circuit, so every one of those indices really is
    instantiated; a type that broke the module would break the BUILD here
  - the same holds for the class-only and non-class-only sub-zoos, which
    isolates whether any failure came from the class types or the rest
*/
bool
tests_pack_element_hostile_pack()
{
    bool ok = true;

    // the full zoo, all three routes
    D_PE_CHECK(ok, pe_routes_agree<D_TEST_HOSTILE_TYPES>::value);

    // the two halves on their own
    D_PE_CHECK(ok, pe_routes_agree<D_TEST_HOSTILE_CLASS_TYPES>::value);
    D_PE_CHECK(ok, pe_routes_agree<D_TEST_HOSTILE_NONCLASS_TYPES>::value);

    // and the complete-types-only variants, which a consumer restricted
    // to complete types would rely on
    D_PE_CHECK(ok, pe_routes_agree<D_TEST_HOSTILE_TYPES_COMPLETE>::value);
    D_PE_CHECK(ok, pe_routes_agree<
                       D_TEST_HOSTILE_CLASS_TYPES_COMPLETE>::value);

    // each single zoo type as a one-element pack, and behind a head
    D_PE_CHECK(ok, ::djinterp::test::holds_for_all<
                       pe_singleton_holds, D_TEST_HOSTILE_TYPES>::value);
    D_PE_CHECK(ok, ::djinterp::test::holds_for_all<
                       pe_after_head_holds, D_TEST_HOSTILE_TYPES>::value);
    D_PE_CHECK(ok, ::djinterp::test::holds_for_all<
                       pe_head_survives_tail,
                       D_TEST_HOSTILE_TYPES>::value);

    return ok;
}

/*
tests_pack_element_duplicates
  Packs in which the same type appears more than once.
  Tests the following:
  - the index selects by POSITION, not by identity: a repeated type is
    reported at every position it occupies
  - a pack of entirely identical elements answers with that element at
    every index, including the last
  - a pack alternating two types round-trips, so repetition does not
    disturb the ordering
  This is worth stating because the recursion peels by position only; a
  hypothetical implementation that searched by type would pass every test
  built from packs of distinct elements and fail all of these.
*/
bool
tests_pack_element_duplicates()
{
    bool ok = true;

    // the same type at several positions
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, pe_first, pe_second, pe_first>,
                       pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_first, pe_second, pe_first>,
                       pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, pe_first, pe_second, pe_first>,
                       pe_second>);

    // entirely identical elements, including the last index
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<0, int, int, int, int>, int>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, int, int, int, int>, int>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<3, int, int, int, int>, int>);

    // ... and with a type that cannot be instantiated at all
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<1, void, void, void>, void>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<2, pe_incomplete, pe_incomplete,
                                      pe_incomplete>,
                       pe_incomplete>);

    // repetition does not disturb the ordering
    D_PE_CHECK(ok, (pe_routes_agree<pe_first, pe_second, pe_first,
                                    pe_second, pe_first>::value));
    D_PE_CHECK(ok, (pe_routes_agree<int, int, int, int>::value));

    // a duplicated element is still reached by position after a longer
    // recursion, which is where a type-matching implementation would
    // return the first occurrence instead
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<4, pe_first, pe_second, pe_third,
                                      pe_fourth, pe_first>,
                       pe_first>);
    D_PE_CHECK(ok, std::is_same_v<
                       pack_element_t<4, pe_first, pe_second, pe_third,
                                      pe_fourth, pe_first>,
                       pack_element_t<0, pe_first>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
