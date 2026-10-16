/*******************************************************************************
* djinterp [testing]                           sorted_container_tests_gating.cpp
*
*   Section TU: the enable_if gate on is_sorted_range.  Positional sortedness
*   is
* a property only an ORDERED container has, so the overload is present for
* sequences and absent for every associative (the gate follows
* is_ordered_container) and for non-containers - all read off the probe trait
* at
* compile time.
*
*
* path:      /tests/djinterp/core/container/sorted_container_tests_gating.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "sorted_container_tests.hpp"   // normal mode: header + macro


NS_DJINTERP
NS_TESTING

// tests_sorted_gating_ordered_enabled
//   is_sorted_range is available for ordered containers.
bool
tests_sorted_gating_ordered_enabled()
{
    D_SR_CHECK(can_sorted_range<std::vector<int>>::value);
    D_SR_CHECK(can_sorted_range<std::list<int>>::value);
    D_SR_CHECK(can_sorted_range<std::deque<int>>::value);
    D_SR_CHECK(can_sorted_range<std::string>::value);
    D_SR_CHECK((can_sorted_range<std::array<int, 3>>::value));

    return true;
}

// tests_sorted_gating_associative_disabled
//   no associative is ordered, so the overload is removed for all of them -
// the comparator-keyed ones and the hash-ordered ones alike.
bool
tests_sorted_gating_associative_disabled()
{
    D_SR_CHECK(!can_sorted_range<std::set<int>>::value);
    D_SR_CHECK(!can_sorted_range<std::multiset<int>>::value);
    D_SR_CHECK((!can_sorted_range<std::map<int, int>>::value));
    D_SR_CHECK(!can_sorted_range<std::unordered_set<int>>::value);
    D_SR_CHECK((!can_sorted_range<std::unordered_map<int, int>>::value));

    return true;
}

// tests_sorted_gating_non_container_disabled
//   a scalar or a pointer is not an ordered container, so it is rejected.
bool
tests_sorted_gating_non_container_disabled()
{
    D_SR_CHECK(!can_sorted_range<int>::value);
    D_SR_CHECK(!can_sorted_range<double>::value);
    D_SR_CHECK(!can_sorted_range<int*>::value);

    return true;
}

NS_END  // testing
NS_END  // djinterp
