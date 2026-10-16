/*******************************************************************************
* djinterp [testing]                            sorted_container_tests_range.cpp
*
*   Section TU: the runtime behaviour of is_sorted_range - both the
* default-comparator and the supplied-comparator overloads - over sorted,
* unsorted, empty, single, duplicate, and boundary inputs, and across several
* ordered container kinds.
*
*
* path:      /tests/djinterp/core/container/sorted_container_tests_range.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "sorted_container_tests.hpp"   // normal mode: header + macro


NS_DJINTERP
NS_TESTING

// tests_sorted_default_basic
//   the default-comparator overload: non-descending is sorted, an out-of-order
// sequence is not, and the empty/single degenerate cases are sorted.
bool
tests_sorted_default_basic()
{
    D_SR_CHECK(is_sorted_range(std::vector<int>{1, 2, 3, 4, 5}));
    D_SR_CHECK(!is_sorted_range(std::vector<int>{1, 3, 2}));
    D_SR_CHECK(is_sorted_range(std::vector<int>{}));
    D_SR_CHECK(is_sorted_range(std::vector<int>{42}));

    return true;
}

// tests_sorted_default_duplicates_and_boundary
//   equal adjacent elements are non-descending (so sorted); an inversion at
//   the
// very start or the very end is caught.
bool
tests_sorted_default_duplicates_and_boundary()
{
    D_SR_CHECK(is_sorted_range(std::vector<int>{1, 1, 2, 2, 3}));
    D_SR_CHECK(!is_sorted_range(std::vector<int>{2, 2, 1}));

    // a single inversion at the head, then at the tail.
    D_SR_CHECK(!is_sorted_range(std::vector<int>{3, 1, 2}));
    D_SR_CHECK(!is_sorted_range(std::vector<int>{1, 2, 1}));

    return true;
}

// tests_sorted_custom_comparator
//   the comparator overload reads sortedness against a supplied relation.
bool
tests_sorted_custom_comparator()
{
    D_SR_CHECK(is_sorted_range(std::vector<int>{5, 4, 3, 2, 1},
                               std::greater<int>()));
    D_SR_CHECK(!is_sorted_range(std::vector<int>{1, 2, 3},
                                std::greater<int>()));

    // degenerate inputs are sorted under any comparator.
    D_SR_CHECK(is_sorted_range(std::vector<int>{}, std::greater<int>()));
    D_SR_CHECK(is_sorted_range(std::vector<int>{7}, std::greater<int>()));

    return true;
}

// tests_sorted_across_containers
//   the check reads only through begin()/end(), so it applies to any ordered
// container: a list, a deque, a fixed array, and a string.
bool
tests_sorted_across_containers()
{
    D_SR_CHECK(is_sorted_range(std::list<int>{1, 2, 3}));
    D_SR_CHECK(!is_sorted_range(std::list<int>{3, 2, 1}));

    D_SR_CHECK(is_sorted_range(std::deque<int>{1, 2, 2, 3}));
    D_SR_CHECK(!is_sorted_range(std::deque<int>{2, 1}));

    D_SR_CHECK((is_sorted_range(std::array<int, 3>{{1, 2, 3}})));
    D_SR_CHECK((!is_sorted_range(std::array<int, 3>{{3, 1, 2}})));

    D_SR_CHECK(is_sorted_range(std::string{"abc"}));
    D_SR_CHECK(!is_sorted_range(std::string{"cba"}));

    return true;
}

NS_END  // testing
NS_END  // djinterp
