/*******************************************************************************
* djinterp [test]                                         array_tests_access.cpp
*
*   Access tests: subscript, at, front, back and data; the size queries; and
* the lifetime gate, which leaves an immutable cell read-only and a constexpr
* cell writable.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_access.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <type_traits>  // std::is_same
#include <utility>      // std::declval
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

/*
tests_access_subscript_and_at
  operator[] and at() reach the same element.
  Tests the following:
  - a write through [] reads back through at(), and the reverse
  - const access through a const reference
  - at() on a const array returns a const reference
*/
bool tests_access_subscript_and_at()
{
    mi4 a(1, 2, 3, 4);

    a[1]    = 20;
    a.at(2) = 30;
    D_AY_CHECK(a.at(1) == 20);
    D_AY_CHECK(a[2] == 30);

    const mi4& c = a;
    D_AY_CHECK(c[0] == 1 && c.at(3) == 4);
    D_AY_CHECK((std::is_same<decltype(c.at(0)), const int&>::value));
    D_AY_CHECK((std::is_same<decltype(a[0]), int&>::value));

    return true;
}

/*
tests_access_front_back_data
  front(), back() and data().
  Tests the following:
  - front() and back() are the first and last elements, and writable
  - data() is the address of element 0, and data() + i of element i
  - const data() is const int*
*/
bool tests_access_front_back_data()
{
    mi4 a(1, 2, 3, 4);

    D_AY_CHECK(a.front() == 1 && a.back() == 4);

    a.front() = 10;
    a.back()  = 40;
    D_AY_CHECK(a[0] == 10 && a[3] == 40);

    D_AY_CHECK(a.data() == &a[0]);
    D_AY_CHECK(a.data() + 3 == &a[3]);
    D_AY_CHECK(a.data()[1] == 2);

    const mi4& c = a;
    D_AY_CHECK((std::is_same<decltype(c.data()), const int*>::value));
    D_AY_CHECK(c.data() == a.data());

    return true;
}

/*
tests_access_size_queries
  size(), max_size(), capacity() and empty().
  Tests the following:
  - all three sizes are the extent, 4
  - an extent-0 array reports 0 and empty
  - the queries are constant expressions
*/
bool tests_access_size_queries()
{
    mi4 a;
    D_AY_CHECK(a.size() == 4 && a.max_size() == 4 && a.capacity() == 4);
    D_AY_CHECK(!a.empty());

    mi0 z;
    D_AY_CHECK(z.size() == 0 && z.max_size() == 0 && z.capacity() == 0);
    D_AY_CHECK(z.empty());

    ini4 n;
    D_AY_CHECK(n.size() == 4 && !n.empty());

    static_assert(mi4().size() == 4, "size() is a constant expression");
    static_assert(mi0().empty(), "empty() is a constant expression");

    return true;
}

/*
tests_access_immutable_is_read_only
  The immutable cells have const access only.
  Tests the following:
  - no assignable [] or at() on ii4 or ini4
  - mi4 and mni4 do have them (non-iterability gates iteration only)
  - data() on a non-const immutable array is const int*
  - reads through [], at(), front() and back() still work
*/
bool tests_access_immutable_is_read_only()
{
    D_AY_CHECK(!(is_detected<ay_mut_index_t, ii4>::value));
    D_AY_CHECK(!(is_detected<ay_mut_at_t, ii4>::value));
    D_AY_CHECK(!(is_detected<ay_mut_index_t, ini4>::value));
    D_AY_CHECK((is_detected<ay_mut_index_t, mi4>::value));
    D_AY_CHECK((is_detected<ay_mut_index_t, mni4>::value));
    D_AY_CHECK((is_detected<ay_mut_at_t, mni4>::value));

    D_AY_CHECK((std::is_same<decltype(std::declval<ii4&>().data()),
                             const int*>::value));

    ii4 i(1, 2, 3, 4);
    D_AY_CHECK(i[1] == 2 && i.at(2) == 3);
    D_AY_CHECK(i.front() == 1 && i.back() == 4);

    return true;
}

/*
tests_access_constexpr_cell_is_writable
  A constexpr-lifetime cell keeps its mutable accessors.
  Tests the following:
  - ci4 and cni4 have assignable [] and at()
  - writes through [], at(), front() and back() land
  - data() is int*
*/
bool tests_access_constexpr_cell_is_writable()
{
    D_AY_CHECK((is_detected<ay_mut_index_t, ci4>::value));
    D_AY_CHECK((is_detected<ay_mut_at_t, cni4>::value));

    ci4 c;
    c[0]      = 5;
    c.at(1)   = 6;
    c.back()  = 8;
    D_AY_CHECK(c.front() == 5 && c[1] == 6 && c[3] == 8);

    D_AY_CHECK((std::is_same<decltype(c.data()), int*>::value));

    return true;
}

NS_END  // testing
NS_END  // djinterp
