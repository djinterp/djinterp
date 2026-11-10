/*******************************************************************************
* djinterp [test]                                   fn_builder_tests_reorder.cpp
*
* Slicing / reordering fluent-operation tests (section ii, part 2):
*   test_take_skip    -- take and skip, including edge counts.
*   test_distinct     -- de-duplication via operator==.
*   test_reverse_sort -- reversed and sorted(cmp).
*
*
* path:      /tests/djinterp/cpp/functional/fn_builder_tests_reorder.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_take_skip()
{
    bool verdict = true;

    std::vector<int> in = { 1, 2, 3, 4, 5 };

    // take fewer than size
    record_assertion(verdict,
        DFnBuilder<int>::create().take(3).execute(in)
            == std::vector<int>({ 1, 2, 3 }),
        "take: keeps the first n elements");

    // take(0) -> empty; take beyond size -> all
    record_assertion(verdict,
        DFnBuilder<int>::create().take(0).execute(in).empty(),
        "take: take(0) yields nothing");
    record_assertion(verdict,
        DFnBuilder<int>::create().take(99).execute(in) == in,
        "take: n beyond size keeps everything");

    // skip some
    record_assertion(verdict,
        DFnBuilder<int>::create().skip(2).execute(in)
            == std::vector<int>({ 3, 4, 5 }),
        "skip: drops the first n elements");

    // skip(0) -> all; skip beyond size -> empty
    record_assertion(verdict,
        DFnBuilder<int>::create().skip(0).execute(in) == in,
        "skip: skip(0) keeps everything");
    record_assertion(verdict,
        DFnBuilder<int>::create().skip(99).execute(in).empty(),
        "skip: n beyond size yields empty");

    // take and skip compose into a window
    record_assertion(verdict,
        DFnBuilder<int>::create().skip(1).take(2).execute(in)
            == std::vector<int>({ 2, 3 }),
        "take/skip: compose into a window");

    return verdict;
}


bool test_distinct()
{
    bool verdict = true;

    // de-duplicates, preserving first-seen order
    std::vector<int> in = { 1, 2, 2, 3, 1, 3, 4 };
    record_assertion(verdict,
        DFnBuilder<int>::create().distinct().execute(in)
            == std::vector<int>({ 1, 2, 3, 4 }),
        "distinct: removes duplicates, preserving first-seen order");

    // already-distinct input is unchanged
    std::vector<int> uniq = { 5, 6, 7 };
    record_assertion(verdict,
        DFnBuilder<int>::create().distinct().execute(uniq) == uniq,
        "distinct: an already-distinct input is unchanged");

    return verdict;
}


bool test_reverse_sort()
{
    bool verdict = true;

    std::vector<int> in = { 3, 1, 4, 1, 5 };

    // reversed
    record_assertion(verdict,
        DFnBuilder<int>::create().reversed().execute(in)
            == std::vector<int>({ 5, 1, 4, 1, 3 }),
        "reversed: reverses element order");

    // sorted ascending
    record_assertion(verdict,
        DFnBuilder<int>::create().sorted(DFnLess{}).execute(in)
            == std::vector<int>({ 1, 1, 3, 4, 5 }),
        "sorted: orders ascending with a less comparator");

    // sorted descending
    record_assertion(verdict,
        DFnBuilder<int>::create().sorted(DFnGreater{}).execute(in)
            == std::vector<int>({ 5, 4, 3, 1, 1 }),
        "sorted: orders descending with a greater comparator");

    // sorted then distinct composes
    record_assertion(verdict,
        DFnBuilder<int>::create().sorted(DFnLess{}).distinct().execute(in)
            == std::vector<int>({ 1, 3, 4, 5 }),
        "sorted/distinct: compose to a sorted unique sequence");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
