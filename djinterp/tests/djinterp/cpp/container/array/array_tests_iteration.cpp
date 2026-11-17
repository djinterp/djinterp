/*******************************************************************************
* djinterp [test]                                      array_tests_iteration.cpp
*
*   Iteration tests: begin/end and their const and reverse forms, standard
* algorithms over an array, the iterability gate, and the empty array.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_iteration.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <algorithm>    // std::sort, std::find, std::reverse, std::equal
#include <numeric>      // std::accumulate
#include <type_traits>  // std::is_same
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

/*
tests_iteration_begin_end
  [begin, end) is the array's storage.
  Tests the following:
  - end - begin is the extent, and begin is data()
  - the iterator type is a plain pointer
  - range-for visits the elements in order
  - a write through an iterator lands in the array
*/
bool tests_iteration_begin_end()
{
    mi4 a(1, 2, 3, 4);

    D_AY_CHECK(a.end() - a.begin() == 4);
    D_AY_CHECK(a.begin() == a.data());
    D_AY_CHECK((std::is_same<mi4::iterator, int*>::value));

    int expected = 1;
    int sum      = 0;

    for (int v : a)
    {
        D_AY_CHECK(v == expected);
        ++expected;
        sum += v;
    }

    D_AY_CHECK(sum == 10);

    *(a.begin() + 2) = 30;
    D_AY_CHECK(a[2] == 30);

    return true;
}

/*
tests_iteration_const_and_reverse
  The const and reverse iterators.
  Tests the following:
  - cbegin/cend equal begin/end, and are const int*
  - begin() on a const array is const int*
  - rbegin..rend visits 4, 3, 2, 1, and writes through rbegin land
  - const rbegin..rend on a const array visits the same
*/
bool tests_iteration_const_and_reverse()
{
    mi4 a(1, 2, 3, 4);

    D_AY_CHECK(a.cbegin() == a.begin() && a.cend() == a.end());
    D_AY_CHECK((std::is_same<decltype(a.cbegin()), const int*>::value));

    const mi4& c = a;
    D_AY_CHECK((std::is_same<decltype(c.begin()), const int*>::value));

    int expected = 4;

    for (auto it = a.rbegin(); it != a.rend(); ++it)
    {
        D_AY_CHECK(*it == expected);
        --expected;
    }

    D_AY_CHECK(expected == 0);

    *a.rbegin() = 40;
    D_AY_CHECK(a[3] == 40);

    D_AY_CHECK(*c.rbegin() == 40);
    D_AY_CHECK(c.rend() - c.rbegin() == 4);

    return true;
}

/*
tests_iteration_std_algorithms
  Standard algorithms over an array.
  Tests the following:
  - std::sort orders the elements
  - std::accumulate sums them
  - std::find finds an element at its position
  - std::reverse reverses them
  - std::equal compares two arrays
*/
bool tests_iteration_std_algorithms()
{
    mi4 a(3, 1, 4, 2);

    std::sort(a.begin(), a.end());
    D_AY_CHECK(a[0] == 1 && a[1] == 2 && a[2] == 3 && a[3] == 4);

    D_AY_CHECK(std::accumulate(a.begin(), a.end(), 0) == 10);
    D_AY_CHECK(std::find(a.begin(), a.end(), 3) - a.begin() == 2);
    D_AY_CHECK(std::find(a.begin(), a.end(), 9) == a.end());

    std::reverse(a.begin(), a.end());
    D_AY_CHECK(a[0] == 4 && a[3] == 1);

    mi4 b(4, 3, 2, 1);
    D_AY_CHECK(std::equal(a.begin(), a.end(), b.begin()));

    b[0] = 0;
    D_AY_CHECK(!std::equal(a.begin(), a.end(), b.begin()));

    return true;
}

/*
tests_iteration_is_gated
  The iterability and lifetime gates on iteration.
  Tests the following:
  - the three non-iterable cells have no begin(), cbegin() or rbegin()
  - the three iterable cells have them
  - an immutable iterable cell's begin(), on a non-const object, is the
    const iterator, and its rbegin() the const reverse iterator
*/
bool tests_iteration_is_gated()
{
    D_AY_CHECK(!(is_detected<ay_begin_t, mni4>::value));
    D_AY_CHECK(!(is_detected<ay_begin_t, ini4>::value));
    D_AY_CHECK(!(is_detected<ay_begin_t, cni4>::value));
    D_AY_CHECK(!(is_detected<ay_cbegin_t, mni4>::value));
    D_AY_CHECK(!(is_detected<ay_rbegin_t, cni4>::value));

    D_AY_CHECK((is_detected<ay_begin_t, mi4>::value));
    D_AY_CHECK((is_detected<ay_begin_t, ii4>::value));
    D_AY_CHECK((is_detected<ay_begin_t, ci4>::value));

    D_AY_CHECK((std::is_same<ay_begin_t<ii4>, const int*>::value));
    D_AY_CHECK((std::is_same<ay_rbegin_t<ii4>,
                             ii4::const_reverse_iterator>::value));

    ii4 i(1, 2, 3, 4);
    int sum = 0;

    for (int v : i)
    {
        sum += v;
    }

    D_AY_CHECK(sum == 10);

    return true;
}

/*
tests_iteration_empty_array
  An extent-0 array iterates nothing.
  Tests the following:
  - begin() == end() and rbegin() == rend()
  - range-for runs no iteration
*/
bool tests_iteration_empty_array()
{
    mi0 z;

    D_AY_CHECK(z.begin() == z.end());
    D_AY_CHECK(z.rbegin() == z.rend());

    int visits = 0;

    for (int v : z)
    {
        (void)v;
        ++visits;
    }

    D_AY_CHECK(visits == 0);

    return true;
}

NS_END  // testing
NS_END  // djinterp
