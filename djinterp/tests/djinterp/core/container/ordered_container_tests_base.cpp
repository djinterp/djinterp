/*******************************************************************************
* djinterp [testing]                            ordered_container_tests_base.cpp
*
*   Section TU: the ordered_base CRTP order operations.  Every method is driven
* through a vector-backed derived container, and each branch is reached
* deliberately - the empty guard in rotate_right, the clear-to-fill path in the
* shifts, and all four crossing cases of is_palindrome.  A closing array-backed
* run shows the base does not depend on the backing store.
*
*
* path:      /tests/djinterp/core/container/ordered_container_tests_base.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "ordered_container_tests.hpp"   // normal mode


NS_DJINTERP
NS_TESTING

// tests_ordered_front_back
//   front()/back() name the ends; for one element they are the same element.
bool
tests_ordered_front_back()
{
    vec_seq<int> _v{10, 20, 30, 40};

    D_OR_CHECK(_v.front() == 10);
    D_OR_CHECK(_v.back() == 40);

    vec_seq<int> _one{7};

    D_OR_CHECK(_one.front() == 7);
    D_OR_CHECK(_one.back() == 7);
    D_OR_CHECK(_one.front() == _one.back());

    return true;
}

// tests_ordered_nth
//   nth(n) is an iterator to position n - first, interior, last.
bool
tests_ordered_nth()
{
    vec_seq<int> _v{5, 6, 7, 8, 9};

    D_OR_CHECK(*_v.nth(0) == 5);
    D_OR_CHECK(*_v.nth(2) == 7);
    D_OR_CHECK(*_v.nth(4) == 9);

    return true;
}

// tests_ordered_is_sorted_default
//   is_sorted() over the default order: true for non-descending, empty,
//   single.
bool
tests_ordered_is_sorted_default()
{
    vec_seq<int> _sorted{1, 2, 2, 3, 5};

    D_OR_CHECK(_sorted.is_sorted());

    vec_seq<int> _unsorted{1, 3, 2};

    D_OR_CHECK(!_unsorted.is_sorted());

    vec_seq<int> _empty;

    D_OR_CHECK(_empty.is_sorted());

    vec_seq<int> _single{42};

    D_OR_CHECK(_single.is_sorted());

    return true;
}

// tests_ordered_is_sorted_custom
//   is_sorted(cmp) uses the supplied comparator.
bool
tests_ordered_is_sorted_custom()
{
    vec_seq<int> _descending{9, 7, 4, 4, 1};

    D_OR_CHECK(_descending.is_sorted(std::greater<int>()));

    vec_seq<int> _ascending{1, 2, 3};

    D_OR_CHECK(!_ascending.is_sorted(std::greater<int>()));

    return true;
}

// tests_ordered_is_palindrome
//   every crossing case: empty, single, even/odd palindromes and non-.
bool
tests_ordered_is_palindrome()
{
    vec_seq<int> _empty;

    D_OR_CHECK(_empty.is_palindrome());

    vec_seq<int> _single{1};

    D_OR_CHECK(_single.is_palindrome());

    vec_seq<int> _two_same{4, 4};

    D_OR_CHECK(_two_same.is_palindrome());

    vec_seq<int> _two_diff{4, 5};

    D_OR_CHECK(!_two_diff.is_palindrome());

    vec_seq<int> _odd_pal{1, 2, 3, 2, 1};

    D_OR_CHECK(_odd_pal.is_palindrome());

    vec_seq<int> _odd_non{1, 2, 3, 4, 1};

    D_OR_CHECK(!_odd_non.is_palindrome());

    vec_seq<int> _even_pal{1, 2, 2, 1};

    D_OR_CHECK(_even_pal.is_palindrome());

    vec_seq<int> _even_non{1, 2, 3, 1};

    D_OR_CHECK(!_even_non.is_palindrome());

    return true;
}

// tests_ordered_reverse
//   reverse() flips order; empty/single are no-ops; twice is the identity.
bool
tests_ordered_reverse()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.reverse();
    D_OR_CHECK(contents_equal(_v, {5, 4, 3, 2, 1}));

    _v.reverse();
    D_OR_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    vec_seq<int> _empty;

    _empty.reverse();
    D_OR_CHECK(contents_equal(_empty, std::initializer_list<int>{}));

    vec_seq<int> _single{9};

    _single.reverse();
    D_OR_CHECK(contents_equal(_single, {9}));

    return true;
}

// tests_ordered_rotate_left
//   rotate_left(n): n==0 no-op, an ordinary rotate, and n>size modulo size.
bool
tests_ordered_rotate_left()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.rotate_left(0);
    D_OR_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.rotate_left(2);
    D_OR_CHECK(contents_equal(_v, {3, 4, 5, 1, 2}));

    // 7 % 5 == 2: the same as a left rotation by two.
    vec_seq<int> _w{1, 2, 3, 4, 5};

    _w.rotate_left(7);
    D_OR_CHECK(contents_equal(_w, {3, 4, 5, 1, 2}));

    return true;
}

// tests_ordered_rotate_right
//   rotate_right(n): n==0 no-op, an ordinary rotate, the empty guard, n>size.
bool
tests_ordered_rotate_right()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.rotate_right(0);
    D_OR_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.rotate_right(2);
    D_OR_CHECK(contents_equal(_v, {4, 5, 1, 2, 3}));

    // the size==0 guard: nothing to rotate, no modulo by zero.
    vec_seq<int> _empty;

    _empty.rotate_right(3);
    D_OR_CHECK(contents_equal(_empty, std::initializer_list<int>{}));

    // 7 % 5 == 2: the same as a right rotation by two.
    vec_seq<int> _w{1, 2, 3, 4, 5};

    _w.rotate_right(7);
    D_OR_CHECK(contents_equal(_w, {4, 5, 1, 2, 3}));

    return true;
}

// tests_ordered_shift_left
//   shift_left(n,fill): n==0 no-op, an ordinary shift filling the tail, and
//   the
// n>=size path that clears the whole sequence to fill.
bool
tests_ordered_shift_left()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.shift_left(0, 0);
    D_OR_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.shift_left(2, 0);
    D_OR_CHECK(contents_equal(_v, {3, 4, 5, 0, 0}));

    // n == size: clears to fill.
    vec_seq<int> _exact{1, 2, 3, 4, 5};

    _exact.shift_left(5, -1);
    D_OR_CHECK(contents_equal(_exact, {-1, -1, -1, -1, -1}));

    // n > size: still clears to fill.
    vec_seq<int> _over{1, 2, 3};

    _over.shift_left(6, 0);
    D_OR_CHECK(contents_equal(_over, {0, 0, 0}));

    return true;
}

// tests_ordered_shift_right
//   shift_right(n,fill): n==0 no-op, an ordinary shift filling the head, and
//   the
// n>=size path that clears the whole sequence to fill.
bool
tests_ordered_shift_right()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.shift_right(0, 0);
    D_OR_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.shift_right(2, 0);
    D_OR_CHECK(contents_equal(_v, {0, 0, 1, 2, 3}));

    // n == size: clears to fill.
    vec_seq<int> _exact{1, 2, 3, 4, 5};

    _exact.shift_right(5, -1);
    D_OR_CHECK(contents_equal(_exact, {-1, -1, -1, -1, -1}));

    // n > size: still clears to fill.
    vec_seq<int> _over{1, 2, 3};

    _over.shift_right(6, 0);
    D_OR_CHECK(contents_equal(_over, {0, 0, 0}));

    return true;
}

// tests_ordered_storage_agnostic
//   the operations read and write only through begin()/end()/size(), so a
// fixed array-backed container behaves identically.
bool
tests_ordered_storage_agnostic()
{
    arr_seq<int, 5> _a{1, 2, 3, 4, 5};

    D_OR_CHECK(_a.front() == 1);
    D_OR_CHECK(_a.back() == 5);
    D_OR_CHECK(!_a.is_palindrome());

    _a.reverse();
    D_OR_CHECK(contents_equal(_a, {5, 4, 3, 2, 1}));

    _a.rotate_left(1);
    D_OR_CHECK(contents_equal(_a, {4, 3, 2, 1, 5}));

    arr_seq<int, 3> _pal{2, 7, 2};

    D_OR_CHECK(_pal.is_palindrome());

    return true;
}

NS_END  // testing
NS_END  // djinterp
