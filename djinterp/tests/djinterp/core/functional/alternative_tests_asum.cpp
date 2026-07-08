/******************************************************************************
* djinterp [test]                                   alternative_tests_asum.cpp
*
*   Section II.3 of the alternative.hpp suite: asum, which folds a Foldable of
* alternatives with alt from aempty ("take the first that succeeds").  Covers
* the first-success selection, the empty foldable (yields aempty), all-empty
* input, single-element inputs, the leading-empties-skipped case, a non-int
* element type, and -- because asum sits at the Foldable x Alternative
* intersection -- the same over the bag<T> Foldable fixture (a Foldable other
* than the std::vector instance) plus the element-type deduction it relies on.
*
* path:      /tests/djinterp/core/functional/alternative_tests_asum.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "alternative_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_asum_first_success
  Tests the following:
  - asum returns the first engaged element: [none, some(3), some(9)] -> some(3).
*/
static bool
tests_asum_first_success()
{
    std::vector< opt<int> > xs;

    xs.push_back(opt<int>());
    xs.push_back(opt<int>(3));
    xs.push_back(opt<int>(9));

    return (::djinterp::asum(xs) == opt<int>(3));
}

/*
tests_asum_empty
  Tests the following:
  - asum of an empty foldable is aempty (the disengaged value): the fold seed
    survives with nothing to choose.
*/
static bool
tests_asum_empty()
{
    std::vector< opt<int> > xs;

    return (::djinterp::asum(xs) == opt<int>());
}

/*
tests_asum_all_none
  Tests the following:
  - asum of all-empty elements is empty.
*/
static bool
tests_asum_all_none()
{
    std::vector< opt<int> > xs;

    xs.push_back(opt<int>());
    xs.push_back(opt<int>());

    return (::djinterp::asum(xs) == opt<int>());
}

/*
tests_asum_single_some
  Tests the following:
  - asum of a single engaged element is that element.
*/
static bool
tests_asum_single_some()
{
    std::vector< opt<int> > xs;

    xs.push_back(opt<int>(5));

    return (::djinterp::asum(xs) == opt<int>(5));
}

/*
tests_asum_single_none
  Tests the following:
  - asum of a single empty element is empty.
*/
static bool
tests_asum_single_none()
{
    std::vector< opt<int> > xs;

    xs.push_back(opt<int>());

    return (::djinterp::asum(xs) == opt<int>());
}

/*
tests_asum_first_of_many
  Tests the following:
  - with every element engaged, asum returns the FIRST: [1,2,3] -> some(1).
*/
static bool
tests_asum_first_of_many()
{
    std::vector< opt<int> > xs;

    xs.push_back(opt<int>(1));
    xs.push_back(opt<int>(2));
    xs.push_back(opt<int>(3));

    return (::djinterp::asum(xs) == opt<int>(1));
}

/*
tests_asum_skip_leading_none
  Tests the following:
  - leading empties are skipped: [none, none, some(7)] -> some(7).
*/
static bool
tests_asum_skip_leading_none()
{
    std::vector< opt<int> > xs;

    xs.push_back(opt<int>());
    xs.push_back(opt<int>());
    xs.push_back(opt<int>(7));

    return (::djinterp::asum(xs) == opt<int>(7));
}

/*
tests_asum_string_element
  Tests the following:
  - asum selects the first engaged element at a non-int element type.
*/
static bool
tests_asum_string_element()
{
    std::vector< opt<std::string> > xs;

    xs.push_back(opt<std::string>());
    xs.push_back(opt<std::string>("hi"));
    xs.push_back(opt<std::string>("bye"));

    return (::djinterp::asum(xs) == opt<std::string>("hi"));
}

/*
tests_asum_over_bag
  Tests the following:
  - asum is generic over the Foldable, not vector-specific: folding the bag<T>
    fixture of alternatives yields the first engaged element.
*/
static bool
tests_asum_over_bag()
{
    bag< opt<int> > b;

    b.data.push_back(opt<int>());
    b.data.push_back(opt<int>(4));
    b.data.push_back(opt<int>(5));

    return (::djinterp::asum(b) == opt<int>(4));
}

/*
tests_asum_over_bag_empty
  Tests the following:
  - asum of an empty non-vector foldable is aempty.
*/
static bool
tests_asum_over_bag_empty()
{
    bag< opt<int> > b;

    return (::djinterp::asum(b) == opt<int>());
}

/*
tests_asum_foldable_value_type
  Tests the following:
  - the element-type asum deduces from the foldable is the alternative type
    itself: foldable_value_type_t<vector<opt<int>>> is opt<int> (and likewise
    for the bag fixture).
*/
static bool
tests_asum_foldable_value_type()
{
    const bool vec_elem =
        std::is_same<
            ::djinterp::foldable_value_type_t< std::vector< opt<int> > >,
            opt<int> >::value;

    const bool bag_elem =
        std::is_same<
            ::djinterp::foldable_value_type_t< bag< opt<int> > >,
            opt<int> >::value;

    return (vec_elem && bag_elem);
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
alternative_asum_block()
{
    dt::block_spec block;

    block.name       = "II.3 asum";
    block.descriptor =
        "choose across a foldable of alternatives: first success, empties, generic";

    block.tests.push_back(dt::test_spec{
        "first success",
        "[none, some(3), some(9)] -> some(3)",
        &tests_asum_first_success });

    block.tests.push_back(dt::test_spec{
        "empty foldable",
        "no elements -> aempty",
        &tests_asum_empty });

    block.tests.push_back(dt::test_spec{
        "all empty",
        "every element empty -> empty",
        &tests_asum_all_none });

    block.tests.push_back(dt::test_spec{
        "single engaged",
        "one engaged element -> that element",
        &tests_asum_single_some });

    block.tests.push_back(dt::test_spec{
        "single empty",
        "one empty element -> empty",
        &tests_asum_single_none });

    block.tests.push_back(dt::test_spec{
        "first of many",
        "all engaged -> the first",
        &tests_asum_first_of_many });

    block.tests.push_back(dt::test_spec{
        "skip leading empties",
        "[none, none, some(7)] -> some(7)",
        &tests_asum_skip_leading_none });

    block.tests.push_back(dt::test_spec{
        "string element",
        "first engaged at a non-int element type",
        &tests_asum_string_element });

    block.tests.push_back(dt::test_spec{
        "over bag foldable",
        "generic over any Foldable (not just vector)",
        &tests_asum_over_bag });

    block.tests.push_back(dt::test_spec{
        "empty bag foldable",
        "empty non-vector foldable -> aempty",
        &tests_asum_over_bag_empty });

    block.tests.push_back(dt::test_spec{
        "element-type deduction",
        "foldable_value_type_t is the alternative type",
        &tests_asum_foldable_value_type });

    return block;
}


NS_END  // testing
NS_END  // djinterp
