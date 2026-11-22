/*******************************************************************************
* djinterp [cpp][container]                           array_tests_algorithms.cpp
*
*   Algorithm tests: fill and swap, the free array_equal, array_copy and
* array_swap, their lifetime gates, and their use in constant expressions.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_algorithms.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <string>  // std::string
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

// ay_constexpr_algorithms
//   function: runs fill, swap, array_copy, array_swap and array_equal in a
// constant expression and folds the outcome into one int.
//   a = 1 2 3 4 and b = 3 3 3 3 after the member swap; c copies a; the free
// swap then leaves b = 1 2 3 4 and c = 3 3 3 3; so array_equal(a, b) holds
// and c[0] is 3: the result is 103.
static constexpr
int ay_constexpr_algorithms()
{
    mi4 a;
    a.fill(3);

    mi4 b(1, 2, 3, 4);
    a.swap(b);

    mi4 c;
    array_copy(a, c);
    array_swap(b, c);

    return ( (array_equal(a, b) ? 100 : 0) + c[0] );
}

/*
tests_algorithms_fill
  fill() sets every element.
  Tests the following:
  - an int array filled with 7 holds 7 everywhere
  - a string array filled with "q" holds "q" everywhere
  - a non-iterable mutable cell fills too
*/
bool tests_algorithms_fill()
{
    mi4 a(1, 2, 3, 4);
    a.fill(7);
    D_AY_CHECK(a[0] == 7 && a[1] == 7 && a[2] == 7 && a[3] == 7);

    ms3 s;
    s.fill("q");
    D_AY_CHECK(s[0] == "q" && s[1] == "q" && s[2] == "q");

    mni4 n;
    n.fill(2);
    D_AY_CHECK(n[0] == 2 && n[3] == 2);

    return true;
}

/*
tests_algorithms_member_swap
  swap() exchanges every element.
  Tests the following:
  - two int arrays exchange their elements
  - two string arrays exchange theirs
  - swapping twice restores both
*/
bool tests_algorithms_member_swap()
{
    mi4 a(1, 2, 3, 4);
    mi4 b(5, 6, 7, 8);

    a.swap(b);
    D_AY_CHECK(a[0] == 5 && a[3] == 8);
    D_AY_CHECK(b[0] == 1 && b[3] == 4);

    a.swap(b);
    D_AY_CHECK(a[0] == 1 && b[0] == 5);

    ms3 s("a", "b", "c");
    ms3 t("x", "y", "z");
    s.swap(t);
    D_AY_CHECK(s[0] == "x" && s[2] == "z");
    D_AY_CHECK(t[0] == "a" && t[2] == "c");

    return true;
}

/*
tests_algorithms_array_equal
  array_equal compares element-wise.
  Tests the following:
  - equal values: true; one differing element: false
  - a mutable and an immutable cell with equal values: true
  - two extent-0 arrays: true
  - two string arrays
*/
bool tests_algorithms_array_equal()
{
    mi4 a(1, 2, 3, 4);
    mi4 b(1, 2, 3, 4);
    mi4 c(1, 2, 3, 5);

    D_AY_CHECK(array_equal(a, b));
    D_AY_CHECK(!array_equal(a, c));

    ii4 i(1, 2, 3, 4);
    D_AY_CHECK(array_equal(a, i));
    D_AY_CHECK(array_equal(i, a));

    mi0 z1;
    mi0 z2;
    D_AY_CHECK(array_equal(z1, z2));

    ms3 s("a", "b", "c");
    ms3 t("a", "b", "c");
    D_AY_CHECK(array_equal(s, t));

    t[1] = "B";
    D_AY_CHECK(!array_equal(s, t));

    return true;
}

/*
tests_algorithms_array_copy
  array_copy copies into a writable array.
  Tests the following:
  - from a mutable source
  - from an immutable source
  - into a constexpr-lifetime cell
  - the source is unchanged
*/
bool tests_algorithms_array_copy()
{
    mi4 src(1, 2, 3, 4);
    mi4 dst;

    array_copy(src, dst);
    D_AY_CHECK(array_equal(src, dst));

    const ii4 isrc(9, 8, 7, 6);
    array_copy(isrc, dst);
    D_AY_CHECK(dst[0] == 9 && dst[3] == 6);

    ci4 cdst;
    array_copy(src, cdst);
    D_AY_CHECK(cdst[0] == 1 && cdst[3] == 4);

    D_AY_CHECK(src[0] == 1 && src[3] == 4);

    return true;
}

/*
tests_algorithms_array_swap
  array_swap exchanges two writable arrays.
  Tests the following:
  - two mutable int arrays
  - a mutable and a constexpr-lifetime cell
  - two string arrays
*/
bool tests_algorithms_array_swap()
{
    mi4 a(1, 2, 3, 4);
    mi4 b(5, 6, 7, 8);

    array_swap(a, b);
    D_AY_CHECK(a[0] == 5 && b[0] == 1);

    ci4 c(0, 0, 0, 9);
    array_swap(a, c);
    D_AY_CHECK(a[3] == 9 && c[0] == 5);

    ms3 s("a", "b", "c");
    ms3 t("x", "y", "z");
    array_swap(s, t);
    D_AY_CHECK(s[1] == "y" && t[1] == "b");

    return true;
}

/*
tests_algorithms_mutation_is_gated
  The mutating algorithms refuse an immutable array.
  Tests the following:
  - ii4 and ini4 have no fill() or swap(); the other cells do
  - array_copy into an immutable array isn't callable; from one it is
  - array_swap with an immutable array isn't callable
*/
bool tests_algorithms_mutation_is_gated()
{
    D_AY_CHECK(!(is_detected<ay_fill_t, ii4>::value));
    D_AY_CHECK(!(is_detected<ay_fill_t, ini4>::value));
    D_AY_CHECK(!(is_detected<ay_swap_t, ii4>::value));
    D_AY_CHECK((is_detected<ay_fill_t, mni4>::value));
    D_AY_CHECK((is_detected<ay_fill_t, ci4>::value));
    D_AY_CHECK((is_detected<ay_swap_t, cni4>::value));

    D_AY_CHECK(!(is_detected<ay_array_copy_t, mi4, ii4>::value));
    D_AY_CHECK((is_detected<ay_array_copy_t, ii4, mi4>::value));
    D_AY_CHECK(!(is_detected<ay_array_swap_t, mi4, ii4>::value));
    D_AY_CHECK(!(is_detected<ay_array_swap_t, ii4, mi4>::value));
    D_AY_CHECK((is_detected<ay_array_swap_t, mi4, ci4>::value));

    return true;
}

/*
tests_algorithms_constexpr
  The algorithms in a constant expression.
  Tests the following:
  - fill, swap, array_copy, array_swap and array_equal evaluate at compile
    time (static_assert), with the result 103
  - the same function gives 103 at run time
*/
bool tests_algorithms_constexpr()
{
    static_assert(ay_constexpr_algorithms() == 103,
                  "the array algorithms are usable in constant expressions");

    D_AY_CHECK(ay_constexpr_algorithms() == 103);

    return true;
}

NS_END  // testing
NS_END  // djinterp
