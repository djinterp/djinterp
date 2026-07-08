/******************************************************************************
* djinterp [test]                            view_tests_adapters_combining.cpp
*
* Combining adapter view tests (section IV, part 2): adapters that pair,
* join, reorder, or regroup elements.
*
*   test_enumerate -- pairs each element with its zero-based index.
*   test_zip       -- pairs two views in lockstep, stopping at the shorter.
*   test_concat    -- yields all of one view then all of another.
*   test_reverse   -- reverses a bidirectional view.
*   test_chunk     -- groups into fixed-size vectors (last may be short).
*   test_stride    -- yields every n-th element starting from the first.
******************************************************************************/

#include <vector>
#include <utility>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_enumerate(test::test_handler& _h)
{
    std::vector<int> data = { 10, 20, 30 };

    using pair_t = std::pair<std::size_t, int>;
    std::vector<pair_t> expected = { pair_t(0, 10),
                                     pair_t(1, 20),
                                     pair_t(2, 30) };

    test::record_assertion(_h,
        (data | views::enumerate() | to_vector()) == expected,
        "enumerate: pairs each element with its index");

    // empty input -> empty
    std::vector<int> none;
    test::record_assertion(_h,
        (none | views::enumerate() | count()) == 0u,
        "enumerate: empty input yields nothing");

    return;
}


void test_zip(test::test_handler& _h)
{
    std::vector<int>  a = { 1, 2, 3 };
    std::vector<char> b = { 'x', 'y' };   // deliberately shorter

    ref_view<std::vector<int>>  va(a);
    ref_view<std::vector<char>> vb(b);

    using pair_t = std::pair<int, char>;
    std::vector<pair_t> expected = { pair_t(1, 'x'), pair_t(2, 'y') };

    // stops at the shorter of the two
    test::record_assertion(_h,
        (va | views::zip(vb) | to_vector()) == expected,
        "zip: pairs in lockstep and stops at the shorter view");

    // zipping against an empty view -> empty
    std::vector<char>           none;
    ref_view<std::vector<char>> vnone(none);
    test::record_assertion(_h,
        (va | views::zip(vnone) | count()) == 0u,
        "zip: an empty operand yields nothing");

    return;
}


void test_concat(test::test_handler& _h)
{
    std::vector<int> a = { 1, 2 };
    std::vector<int> b = { 3, 4, 5 };

    ref_view<std::vector<int>> va(a);
    ref_view<std::vector<int>> vb(b);

    test::record_assertion(_h,
        (va | views::concat(vb) | to_vector())
            == std::vector<int>({ 1, 2, 3, 4, 5 }),
        "concat: yields all of the first view then all of the second");

    // concat with an empty second view
    std::vector<int>           none;
    ref_view<std::vector<int>> vnone(none);
    test::record_assertion(_h,
        (va | views::concat(vnone) | to_vector())
            == std::vector<int>({ 1, 2 }),
        "concat: appending an empty view changes nothing");

    return;
}


void test_reverse(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5 };

    test::record_assertion(_h,
        (data | views::reverse() | to_vector())
            == std::vector<int>({ 5, 4, 3, 2, 1 }),
        "reverse: yields elements in reverse order");

    // single element reverses to itself
    std::vector<int> one = { 42 };
    test::record_assertion(_h,
        (one | views::reverse() | to_vector()) == std::vector<int>({ 42 }),
        "reverse: a single element is its own reverse");

    return;
}


void test_chunk(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5 };

    using chunk_t = std::vector<int>;
    std::vector<chunk_t> expected = { chunk_t({ 1, 2 }),
                                      chunk_t({ 3, 4 }),
                                      chunk_t({ 5 }) };

    // groups of 2; the final group is short
    test::record_assertion(_h,
        (data | views::chunk(2) | to_vector()) == expected,
        "chunk: groups into fixed-size vectors, last group short");

    // chunk size larger than input -> a single full-input group
    std::vector<chunk_t> one_group = { chunk_t({ 1, 2, 3, 4, 5 }) };
    test::record_assertion(_h,
        (data | views::chunk(99) | to_vector()) == one_group,
        "chunk: a size beyond the input yields one group");

    return;
}


void test_stride(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5, 6 };

    // every 2nd element, starting at the first
    test::record_assertion(_h,
        (data | views::stride(2) | to_vector())
            == std::vector<int>({ 1, 3, 5 }),
        "stride: yields every n-th element from the first");

    // stride of 1 -> unchanged
    test::record_assertion(_h,
        (data | views::stride(1) | to_vector())
            == std::vector<int>({ 1, 2, 3, 4, 5, 6 }),
        "stride: a stride of 1 yields every element");

    return;
}


NS_END  // testing
NS_END  // djinterp
