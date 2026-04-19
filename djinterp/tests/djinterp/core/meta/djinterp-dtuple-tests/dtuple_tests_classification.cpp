/******************************************************************************
* djinterp [testing]                            dtuple_tests_classification.cpp
*
*   Classification-group test definitions for the dtuple test suite:
* type_selector (first-match-wins case dispatch), homogeneity and
* 2D-tuple detection, and the interplay between is_2d_tuple and
* is_tuple_homogeneous (nested predicate composition).
*
*   NOTE on default_case coverage:
*   A thorough test of default_case as non-last-element was on the
* original expansion list.  That test is deferred — the current
* dtuple header does not have a defined behavior for "default_case
* appearing as non-last", and pinning arbitrary current behavior
* without a header-level decision would create a maintenance
* footgun.  See the TODO stub in tests_dtuple_type_selector below.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_classification.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   TYPE_SELECTOR
// =========================================================================

/*
tests_dtuple_type_selector
  Verifies type_selector resolves to the type of the first
  type_case whose condition is true, or to void when none match.
  Tests the following:
  - first matching case wins
  - later cases are ignored even when also true (pin this
    first-match-wins invariant — metaprograms downstream rely on it)
  - empty selector yields void
  - selector with all-false cases yields void
  - multi-case: false, false, true, false selects the third case
*/
bool
tests_dtuple_type_selector(
    test_handler& _handler
)
{
    // first match wins
    D_TEST_TYPE_EQ(
        type_select_t<type_case<true,  int>,
                      type_case<false, char>>,
        int);

    // skip false case to pick later true case
    D_TEST_TYPE_EQ(
        type_select_t<type_case<false, int>,
                      type_case<true,  char>>,
        char);

    // multiple true cases — first wins (pinned explicitly)
    D_TEST_TYPE_EQ(
        type_select_t<type_case<true,  long>,
                      type_case<true,  char>,
                      type_case<true,  int>>,
        long);

    // first wins across a longer case list with some true / false
    // interleavings
    D_TEST_TYPE_EQ(
        type_select_t<type_case<false, short>,
                      type_case<false, int>,
                      type_case<true,  char>,
                      type_case<false, long>,
                      type_case<true,  double>>,
        char);

    // no cases -> void
    D_TEST_TYPE_EQ(type_select_t<>, void);

    // all-false -> void
    D_TEST_TYPE_EQ(
        type_select_t<type_case<false, int>,
                      type_case<false, char>>,
        void);

    // TODO: default_case as non-last-element.  The dtuple header
    // currently has no defined behavior for a default_case
    // appearing before the final position of the type_select
    // pack; the three candidate behaviors are:
    //   (a) hard static_assert (compile error)
    //   (b) silently ignored — later cases apply
    //   (c) treated as type_case<true, T> (first-match-wins)
    // Pinning arbitrary current behavior here without a header-
    // level decision would create a maintenance footgun, so this
    // test is deferred.  When a decision is made, add the test
    // here alongside the positive default_case cases and remove
    // this comment.

    D_TEST_FIRE(_handler,
                on_compile_check,
                "type_selector/all-checks");

    return true;
}


// =========================================================================
// II.  HOMOGENEITY AND 2D
// =========================================================================

/*
tests_dtuple_homogeneity_and_2d
  Verifies is_tuple_homogeneous and is_2d_tuple classify tuples
  correctly across single-element, multi-element, mixed, nested,
  and degenerate inputs.
  Tests the following:
  - single-element tuples are homogeneous
  - multi-element tuples of one type are homogeneous
  - mixed-type tuples are NOT homogeneous
  - empty tuple's homogeneity classification (current behavior)
  - 2D tuples are recognized as 2D
  - flat tuples are NOT recognized as 2D
  - empty tuple is recognized as vacuously 2D (per current code)
  - outer size of a 2D tuple reports the correct row count
*/
bool
tests_dtuple_homogeneity_and_2d(
    test_handler& _handler
)
{
    // homogeneity — positive
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous, std::tuple<int>);
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous, std::tuple<int, int>);
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous,
                      std::tuple<char, char, char>);

    // homogeneity — negative
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous,
                       std::tuple<int, char>);
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous,
                       std::tuple<int, int, char>);

    // empty tuple is NOT homogeneous as currently written (falls
    // through to the primary template).  Pin the behavior.
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous, std::tuple<>);

    // 2D — positive
    D_TEST_TRAIT_TRUE(is_2d_tuple, std::tuple<>);
    D_TEST_TRAIT_TRUE(is_2d_tuple,
                      std::tuple<std::tuple<int>>);
    D_TEST_TRAIT_TRUE(is_2d_tuple,
                      std::tuple<std::tuple<int>,
                                 std::tuple<char, long>>);

    // 2D — negative
    D_TEST_TRAIT_FALSE(is_2d_tuple, int);
    D_TEST_TRAIT_FALSE(is_2d_tuple,
                       std::tuple<int, char>);
    D_TEST_TRAIT_FALSE(is_2d_tuple,
                       std::tuple<std::tuple<int>, char>);

    // outer size of a 2D tuple
    static_assert(
        (tuple_outer_size<std::tuple<std::tuple<int>,
                                     std::tuple<char>>>::value == 2),
        "outer size of 2x1 is 2");

    // outer size of an empty 2D tuple
    static_assert(
        (tuple_outer_size<std::tuple<>>::value == 0),
        "outer size of empty tuple is 0");

    D_TEST_FIRE(_handler,
                on_compile_check,
                "homogeneity_and_2d/all-checks");

    return true;
}


// =========================================================================
// III. IS_2D_TUPLE ∩ IS_TUPLE_HOMOGENEOUS
// =========================================================================

/*
tests_dtuple_is_2d_intersect_homog
  Verifies the interaction between is_2d_tuple and
  is_tuple_homogeneous on types that satisfy BOTH predicates (e.g.
  a tuple<tuple<int>, tuple<int>> — a 2D tuple whose rows are all
  the same row type).  This is the canonical "matrix-of-same-row"
  shape and should be uniformly recognized by both predicates.
  Tests the following:
  - tuple<tuple<int>, tuple<int>> satisfies both is_2d_tuple and
    is_tuple_homogeneous
  - trait_equivalent_for confirms agreement across a representative
    pack of types that are either "both" or "neither"
  - the two predicates DISAGREE on a flat homogeneous tuple
    (homogeneous but not 2D) — pinning the non-equivalence for the
    general case
  - the two predicates DISAGREE on a 2D tuple of mixed-width rows
    (2D but not homogeneous) — pinning the other direction
*/
bool
tests_dtuple_is_2d_intersect_homog(
    test_handler& _handler
)
{
    // the canonical matrix-of-same-rows type: both predicates fire
    D_TEST_TRAIT_TRUE(is_2d_tuple,
                      std::tuple<std::tuple<int>,
                                 std::tuple<int>>);
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous,
                      std::tuple<std::tuple<int>,
                                 std::tuple<int>>);

    // for types where both predicates agree (both true OR both
    // false) on this specific pack, trait_equivalent_for holds
    D_TEST_TRAIT_EQUIVALENT(
        internal::is_dtuple_2d_pred,
        internal::is_dtuple_homogeneous_pred,
        std::tuple<std::tuple<int>, std::tuple<int>>);

    // a flat homogeneous tuple: homogeneous but NOT 2D
    D_TEST_TRAIT_TRUE (is_tuple_homogeneous,
                       std::tuple<int, int, int>);
    D_TEST_TRAIT_FALSE(is_2d_tuple,
                       std::tuple<int, int, int>);

    // a 2D tuple of mixed-width rows: 2D but NOT homogeneous
    // (because the two row types tuple<int> and tuple<char, long>
    // are different types at the row level)
    D_TEST_TRAIT_TRUE(is_2d_tuple,
                      std::tuple<std::tuple<int>,
                                 std::tuple<char, long>>);
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous,
                       std::tuple<std::tuple<int>,
                                  std::tuple<char, long>>);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "is_2d_intersect_homog/all-checks");

    return true;
}


NS_END  // testing
NS_END  // djinterp
