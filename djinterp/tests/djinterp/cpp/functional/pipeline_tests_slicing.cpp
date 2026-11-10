/*******************************************************************************
* djinterp [test]                                     pipeline_tests_slicing.cpp
*
* Slicing / reordering tests (section ii, part 2):
*   test_take    -- take, take_last, take_while (incl. edge counts).
*   test_skip    -- skip, skip_while (incl. edge counts).
*   test_slice   -- slice(start, end, step), including the step == 0 error.
*   test_reorder -- reversed, sorted(cmp), sorted() default.
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests_slicing.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = DFunctionPipeline<int>;


bool test_take()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4, 5 });

    // take fewer than size
    record_assertion(verdict,
        p.take(3).to_vector() == std::vector<int>({ 1, 2, 3 }),
        "take: keeps the first n elements");

    // take(0) -> empty; take beyond size -> all
    record_assertion(verdict, p.take(0).empty(),
        "take: take(0) is empty");
    record_assertion(verdict,
        p.take(99).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "take: n beyond size keeps everything");

    // take_last
    record_assertion(verdict,
        p.take_last(2).to_vector() == std::vector<int>({ 4, 5 }),
        "take_last: keeps the final n elements");
    record_assertion(verdict,
        p.take_last(99).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "take_last: n beyond size keeps everything");

    // take_while stops at first failure
    pi q = pi::from({ 2, 4, 5, 6 });
    record_assertion(verdict,
        q.take_while(fn_is_even{}).to_vector() == std::vector<int>({ 2, 4 }),
        "take_while: keeps the leading satisfying run");

    return verdict;
}


bool test_skip()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4, 5 });

    // skip some
    record_assertion(verdict,
        p.skip(2).to_vector() == std::vector<int>({ 3, 4, 5 }),
        "skip: drops the first n elements");

    // skip(0) -> all; skip beyond size -> empty
    record_assertion(verdict,
        p.skip(0).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "skip: skip(0) keeps everything");
    record_assertion(verdict, p.skip(99).empty(),
        "skip: n beyond size yields empty");

    // skip_while drops the leading satisfying run
    pi q = pi::from({ 2, 4, 5, 6 });
    record_assertion(verdict,
        q.skip_while(fn_is_even{}).to_vector() == std::vector<int>({ 5, 6 }),
        "skip_while: drops the leading satisfying run, keeps the rest");

    return verdict;
}


bool test_slice()
{
    bool verdict = true;

    pi p = pi::from({ 0, 1, 2, 3, 4, 5, 6, 7 });

    // [1, 6) step 1
    record_assertion(verdict,
        p.slice(1, 6).to_vector() == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "slice: [start, end) with default step");

    // [0, 8) step 2
    record_assertion(verdict,
        p.slice(0, 8, 2).to_vector() == std::vector<int>({ 0, 2, 4, 6 }),
        "slice: honors a step greater than one");

    // end beyond size is clamped
    record_assertion(verdict,
        p.slice(6, 99).to_vector() == std::vector<int>({ 6, 7 }),
        "slice: end beyond size is clamped");

    // step == 0 -> error pipeline
    pi bad = p.slice(0, 4, 0);
    record_assertion(verdict, bad.has_error() && bad.error_code() == -1,
        "slice: a step of zero produces an error pipeline");

    return verdict;
}


bool test_reorder()
{
    bool verdict = true;

    pi p = pi::from({ 3, 1, 4, 1, 5 });

    // reversed
    record_assertion(verdict,
        p.reversed().to_vector() == std::vector<int>({ 5, 1, 4, 1, 3 }),
        "reversed: reverses element order");

    // sorted with comparator (descending)
    record_assertion(verdict,
        p.sorted(DFnGreater{}).to_vector()
            == std::vector<int>({ 5, 4, 3, 1, 1 }),
        "sorted(cmp): orders by the comparator");

    // sorted() default ascending
    record_assertion(verdict,
        p.sorted().to_vector() == std::vector<int>({ 1, 1, 3, 4, 5 }),
        "sorted: default ordering is ascending");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
