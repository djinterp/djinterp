/******************************************************************************
* djinterp [test]                                   pipeline_tests_slicing.cpp
*
* Slicing / reordering tests (section ii, part 2):
*   test_take    -- take, take_last, take_while (incl. edge counts).
*   test_skip    -- skip, skip_while (incl. edge counts).
*   test_slice   -- slice(start, end, step), including the step == 0 error.
*   test_reorder -- reversed, sorted(cmp), sorted() default.
******************************************************************************/

#include <vector>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


void test_take(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4, 5 });

    // take fewer than size
    test::record_assertion(_h,
        p.take(3).to_vector() == std::vector<int>({ 1, 2, 3 }),
        "take: keeps the first n elements");

    // take(0) -> empty; take beyond size -> all
    test::record_assertion(_h, p.take(0).empty(),
        "take: take(0) is empty");
    test::record_assertion(_h,
        p.take(99).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "take: n beyond size keeps everything");

    // take_last
    test::record_assertion(_h,
        p.take_last(2).to_vector() == std::vector<int>({ 4, 5 }),
        "take_last: keeps the final n elements");
    test::record_assertion(_h,
        p.take_last(99).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "take_last: n beyond size keeps everything");

    // take_while stops at first failure
    pi q = pi::from({ 2, 4, 5, 6 });
    test::record_assertion(_h,
        q.take_while(fn_is_even{}).to_vector() == std::vector<int>({ 2, 4 }),
        "take_while: keeps the leading satisfying run");

    return;
}


void test_skip(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4, 5 });

    // skip some
    test::record_assertion(_h,
        p.skip(2).to_vector() == std::vector<int>({ 3, 4, 5 }),
        "skip: drops the first n elements");

    // skip(0) -> all; skip beyond size -> empty
    test::record_assertion(_h,
        p.skip(0).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "skip: skip(0) keeps everything");
    test::record_assertion(_h, p.skip(99).empty(),
        "skip: n beyond size yields empty");

    // skip_while drops the leading satisfying run
    pi q = pi::from({ 2, 4, 5, 6 });
    test::record_assertion(_h,
        q.skip_while(fn_is_even{}).to_vector() == std::vector<int>({ 5, 6 }),
        "skip_while: drops the leading satisfying run, keeps the rest");

    return;
}


void test_slice(test::test_handler& _h)
{
    pi p = pi::from({ 0, 1, 2, 3, 4, 5, 6, 7 });

    // [1, 6) step 1
    test::record_assertion(_h,
        p.slice(1, 6).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "slice: [start, end) with default step");

    // [0, 8) step 2
    test::record_assertion(_h,
        p.slice(0, 8, 2).to_vector() == std::vector<int>({ 0, 2, 4, 6 }),
        "slice: honors a step greater than one");

    // end beyond size is clamped
    test::record_assertion(_h,
        p.slice(6, 99).to_vector() == std::vector<int>({ 6, 7 }),
        "slice: end beyond size is clamped");

    // step == 0 -> error pipeline
    pi bad = p.slice(0, 4, 0);
    test::record_assertion(_h, bad.has_error() && bad.error_code() == -1,
        "slice: a step of zero produces an error pipeline");

    return;
}


void test_reorder(test::test_handler& _h)
{
    pi p = pi::from({ 3, 1, 4, 1, 5 });

    // reversed
    test::record_assertion(_h,
        p.reversed().to_vector() == std::vector<int>({ 5, 1, 4, 1, 3 }),
        "reversed: reverses element order");

    // sorted with comparator (descending)
    test::record_assertion(_h,
        p.sorted(fn_greater{}).to_vector()
            == std::vector<int>({ 5, 4, 3, 1, 1 }),
        "sorted(cmp): orders by the comparator");

    // sorted() default ascending
    test::record_assertion(_h,
        p.sorted().to_vector() == std::vector<int>({ 1, 1, 3, 4, 5 }),
        "sorted: default ordering is ascending");

    return;
}


NS_END  // testing
NS_END  // djinterp
