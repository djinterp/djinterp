/*******************************************************************************
* djinterp [core]                                    quick_sort_tests_median.cpp
*
*
* path:      /tests/djinterp/core/util/sort/quick_sort_tests_median.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "quick_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_quick_sort_median_all_permutations
bool tests_quick_sort_median_all_permutations()
{
    verdict          result;
    std::vector<int> data;
    int              perm[6][3];
    std::size_t      p;

    //   All six permutations of three distinct values.  Whatever the input
    // order, the routine must leave the MEDIAN at _first -- and, because it
    // ends by swapping _first with _mid, the remaining two must land as
    // (minimum, maximum).  The answer is therefore [2, 1, 3] in every case.
    //
    //   Enumerating all six is what drives the three conditional swaps
    // through both outcomes each: the first fires for 213, 312 and 321; the
    // second for 231 and 321; the third for 132, 231, 312 and 321.
    perm[0][0] = 1; perm[0][1] = 2; perm[0][2] = 3;
    perm[1][0] = 1; perm[1][1] = 3; perm[1][2] = 2;
    perm[2][0] = 2; perm[2][1] = 1; perm[2][2] = 3;
    perm[3][0] = 2; perm[3][1] = 3; perm[3][2] = 1;
    perm[4][0] = 3; perm[4][1] = 1; perm[4][2] = 2;
    perm[5][0] = 3; perm[5][1] = 2; perm[5][2] = 1;

    for (p = 0; p < 6; ++p)
    {
        data.clear();
        data.push_back(perm[p][0]);
        data.push_back(perm[p][1]);
        data.push_back(perm[p][2]);

        quick_median_ints(data.begin(),
                          data.begin() + 1,
                          data.begin() + 2);

        result.check(data[0] == 2);
        result.check(data[1] == 1);
        result.check(data[2] == 3);
    }

    return result.ok();
}


// tests_quick_sort_median_duplicates
bool tests_quick_sort_median_duplicates()
{
    verdict          result;
    std::vector<int> data;

    //   With a strict weak ordering every comparison against an equivalent
    // is false, so the guarded swaps are skipped.  The routine must still
    // leave a value equivalent to the median at _first and must never lose
    // or duplicate an element.
    data.clear();
    data.push_back(5);
    data.push_back(5);
    data.push_back(5);

    quick_median_ints(data.begin(), data.begin() + 1, data.begin() + 2);

    result.check(data[0] == 5);
    result.check(data[1] == 5);
    result.check(data[2] == 5);

    //   Two equal, one smaller, in each of the three arrangements.
    data.clear();
    data.push_back(9);
    data.push_back(9);
    data.push_back(1);

    quick_median_ints(data.begin(), data.begin() + 1, data.begin() + 2);

    result.check(data[0] == 9);
    result.check(is_permutation_of(data, make_triple(9, 9, 1)));

    data.clear();
    data.push_back(1);
    data.push_back(9);
    data.push_back(9);

    quick_median_ints(data.begin(), data.begin() + 1, data.begin() + 2);

    result.check(data[0] == 9);
    result.check(is_permutation_of(data, make_triple(1, 9, 9)));

    data.clear();
    data.push_back(9);
    data.push_back(1);
    data.push_back(9);

    quick_median_ints(data.begin(), data.begin() + 1, data.begin() + 2);

    result.check(data[0] == 9);
    result.check(is_permutation_of(data, make_triple(9, 1, 9)));

    return result.ok();
}


// tests_quick_sort_median_degenerate_positions
bool tests_quick_sort_median_degenerate_positions()
{
    verdict          result;
    std::vector<int> data;

    //   quick_sort_apply computes _mid as _first + (size / 2) and passes
    // _last - 1 as _end, so for the smallest range it ever partitions the
    // three positions collide.  Aliased iterators must not corrupt the
    // range: a self-swap has to stay a no-op.
    //
    //   All three the same position.
    data.clear();
    data.push_back(4);

    quick_median_ints(data.begin(), data.begin(), data.begin());

    result.check(data.size() == 1);
    result.check(data[0] == 4);

    //   _mid aliased to _first, _end distinct: the closing swap of _first
    // with _mid is a self-swap, so the pair must merely be ordered by the
    // three guards.
    data.clear();
    data.push_back(7);
    data.push_back(2);

    quick_median_ints(data.begin(), data.begin(), data.begin() + 1);

    result.check(is_permutation_of(data, make_pair_vector(7, 2)));

    //   _mid aliased to _end.
    data.clear();
    data.push_back(7);
    data.push_back(2);

    quick_median_ints(data.begin(), data.begin() + 1, data.begin() + 1);

    result.check(is_permutation_of(data, make_pair_vector(7, 2)));

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
