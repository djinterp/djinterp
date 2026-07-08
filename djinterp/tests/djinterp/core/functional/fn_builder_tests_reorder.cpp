/******************************************************************************
* djinterp [test]                                  fn_builder_tests_reorder.cpp
*
* Slicing / reordering fluent-operation tests (section ii, part 2):
*   test_take_skip    -- take and skip, including edge counts.
*   test_distinct     -- de-duplication via operator==.
*   test_reverse_sort -- reversed and sorted(cmp).
******************************************************************************/

#include <vector>
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_take_skip(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2, 3, 4, 5 };

    // take fewer than size
    test::record_assertion(_h,
        fn_builder<int>::create().take(3).execute(in)
            == std::vector<int>({ 1, 2, 3 }),
        "take: keeps the first n elements");

    // take(0) -> empty; take beyond size -> all
    test::record_assertion(_h,
        fn_builder<int>::create().take(0).execute(in).empty(),
        "take: take(0) yields nothing");
    test::record_assertion(_h,
        fn_builder<int>::create().take(99).execute(in) == in,
        "take: n beyond size keeps everything");

    // skip some
    test::record_assertion(_h,
        fn_builder<int>::create().skip(2).execute(in)
            == std::vector<int>({ 3, 4, 5 }),
        "skip: drops the first n elements");

    // skip(0) -> all; skip beyond size -> empty
    test::record_assertion(_h,
        fn_builder<int>::create().skip(0).execute(in) == in,
        "skip: skip(0) keeps everything");
    test::record_assertion(_h,
        fn_builder<int>::create().skip(99).execute(in).empty(),
        "skip: n beyond size yields empty");

    // take and skip compose into a window
    test::record_assertion(_h,
        fn_builder<int>::create().skip(1).take(2).execute(in)
            == std::vector<int>({ 2, 3 }),
        "take/skip: compose into a window");

    return;
}


void test_distinct(test::test_handler& _h)
{
    // de-duplicates, preserving first-seen order
    std::vector<int> in = { 1, 2, 2, 3, 1, 3, 4 };
    test::record_assertion(_h,
        fn_builder<int>::create().distinct().execute(in)
            == std::vector<int>({ 1, 2, 3, 4 }),
        "distinct: removes duplicates, preserving first-seen order");

    // already-distinct input is unchanged
    std::vector<int> uniq = { 5, 6, 7 };
    test::record_assertion(_h,
        fn_builder<int>::create().distinct().execute(uniq) == uniq,
        "distinct: an already-distinct input is unchanged");

    return;
}


void test_reverse_sort(test::test_handler& _h)
{
    std::vector<int> in = { 3, 1, 4, 1, 5 };

    // reversed
    test::record_assertion(_h,
        fn_builder<int>::create().reversed().execute(in)
            == std::vector<int>({ 5, 1, 4, 1, 3 }),
        "reversed: reverses element order");

    // sorted ascending
    test::record_assertion(_h,
        fn_builder<int>::create().sorted(fn_less{}).execute(in)
            == std::vector<int>({ 1, 1, 3, 4, 5 }),
        "sorted: orders ascending with a less comparator");

    // sorted descending
    test::record_assertion(_h,
        fn_builder<int>::create().sorted(fn_greater{}).execute(in)
            == std::vector<int>({ 5, 4, 3, 1, 1 }),
        "sorted: orders descending with a greater comparator");

    // sorted then distinct composes
    test::record_assertion(_h,
        fn_builder<int>::create().sorted(fn_less{}).distinct().execute(in)
            == std::vector<int>({ 1, 3, 4, 5 }),
        "sorted/distinct: compose to a sorted unique sequence");

    return;
}


NS_END  // testing
NS_END  // djinterp
