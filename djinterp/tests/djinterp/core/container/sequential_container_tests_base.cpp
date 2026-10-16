/*******************************************************************************
* djinterp [testing]                         sequential_container_tests_base.cpp
*
*   Section TU: the sequential_base CRTP order operations.  Structurally a
* mirror of the ordered_base coverage - every method and every branch (the
* empty guard in rotate_right, the clear-to-fill path in the shifts, the four
* crossing cases of is_palindrome) driven through a vector-backed container,
* closing with an array-backed run for storage independence.
*
*
* path:      /tests/djinterp/core/container/sequential_container_tests_base.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "sequential_container_tests.hpp"   // normal mode: header + macro


NS_DJINTERP
NS_TESTING

// tests_sequential_front_back
//   front()/back() name the ends; for one element they coincide.
bool
tests_sequential_front_back()
{
    vec_seq<int> _v{10, 20, 30, 40};

    D_SQ_CHECK(_v.front() == 10);
    D_SQ_CHECK(_v.back() == 40);

    vec_seq<int> _one{7};

    D_SQ_CHECK(_one.front() == 7);
    D_SQ_CHECK(_one.back() == 7);

    return true;
}

// tests_sequential_nth
//   nth(n) is an iterator to position n - first, interior, last.
bool
tests_sequential_nth()
{
    vec_seq<int> _v{5, 6, 7, 8, 9};

    D_SQ_CHECK(*_v.nth(0) == 5);
    D_SQ_CHECK(*_v.nth(2) == 7);
    D_SQ_CHECK(*_v.nth(4) == 9);

    return true;
}

// tests_sequential_is_sorted_default
//   is_sorted() over the default order.
bool
tests_sequential_is_sorted_default()
{
    vec_seq<int> _sorted{1, 2, 2, 3, 5};

    D_SQ_CHECK(_sorted.is_sorted());

    vec_seq<int> _unsorted{1, 3, 2};

    D_SQ_CHECK(!_unsorted.is_sorted());

    vec_seq<int> _empty;

    D_SQ_CHECK(_empty.is_sorted());

    vec_seq<int> _single{42};

    D_SQ_CHECK(_single.is_sorted());

    return true;
}

// tests_sequential_is_sorted_custom
//   is_sorted(cmp) uses the supplied comparator.
bool
tests_sequential_is_sorted_custom()
{
    vec_seq<int> _descending{9, 7, 4, 4, 1};

    D_SQ_CHECK(_descending.is_sorted(std::greater<int>()));

    vec_seq<int> _ascending{1, 2, 3};

    D_SQ_CHECK(!_ascending.is_sorted(std::greater<int>()));

    return true;
}

// tests_sequential_is_palindrome
//   every crossing case: empty, single, even/odd palindromes and non-.
bool
tests_sequential_is_palindrome()
{
    vec_seq<int> _empty;

    D_SQ_CHECK(_empty.is_palindrome());

    vec_seq<int> _single{1};

    D_SQ_CHECK(_single.is_palindrome());

    vec_seq<int> _two_same{4, 4};

    D_SQ_CHECK(_two_same.is_palindrome());

    vec_seq<int> _two_diff{4, 5};

    D_SQ_CHECK(!_two_diff.is_palindrome());

    vec_seq<int> _odd_pal{1, 2, 3, 2, 1};

    D_SQ_CHECK(_odd_pal.is_palindrome());

    vec_seq<int> _odd_non{1, 2, 3, 4, 1};

    D_SQ_CHECK(!_odd_non.is_palindrome());

    vec_seq<int> _even_pal{1, 2, 2, 1};

    D_SQ_CHECK(_even_pal.is_palindrome());

    vec_seq<int> _even_non{1, 2, 3, 1};

    D_SQ_CHECK(!_even_non.is_palindrome());

    return true;
}

// tests_sequential_reverse
//   reverse() flips order; empty/single are no-ops; twice is the identity.
bool
tests_sequential_reverse()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.reverse();
    D_SQ_CHECK(contents_equal(_v, {5, 4, 3, 2, 1}));

    _v.reverse();
    D_SQ_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    vec_seq<int> _empty;

    _empty.reverse();
    D_SQ_CHECK(contents_equal(_empty, std::initializer_list<int>{}));

    vec_seq<int> _single{9};

    _single.reverse();
    D_SQ_CHECK(contents_equal(_single, {9}));

    return true;
}

// tests_sequential_rotate_left
//   rotate_left(n): n==0 no-op, ordinary rotate, n>size modulo size.
bool
tests_sequential_rotate_left()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.rotate_left(0);
    D_SQ_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.rotate_left(2);
    D_SQ_CHECK(contents_equal(_v, {3, 4, 5, 1, 2}));

    vec_seq<int> _w{1, 2, 3, 4, 5};

    _w.rotate_left(7);
    D_SQ_CHECK(contents_equal(_w, {3, 4, 5, 1, 2}));

    return true;
}

// tests_sequential_rotate_right
//   rotate_right(n): n==0 no-op, ordinary rotate, the empty guard, n>size
//   wrap.
bool
tests_sequential_rotate_right()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.rotate_right(0);
    D_SQ_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.rotate_right(2);
    D_SQ_CHECK(contents_equal(_v, {4, 5, 1, 2, 3}));

    vec_seq<int> _empty;

    _empty.rotate_right(3);
    D_SQ_CHECK(contents_equal(_empty, std::initializer_list<int>{}));

    vec_seq<int> _w{1, 2, 3, 4, 5};

    _w.rotate_right(7);
    D_SQ_CHECK(contents_equal(_w, {4, 5, 1, 2, 3}));

    return true;
}

// tests_sequential_shift_left
//   shift_left(n,fill): n==0 no-op, fill the tail, and the n>=size clear.
bool
tests_sequential_shift_left()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.shift_left(0, 0);
    D_SQ_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.shift_left(2, 0);
    D_SQ_CHECK(contents_equal(_v, {3, 4, 5, 0, 0}));

    vec_seq<int> _exact{1, 2, 3, 4, 5};

    _exact.shift_left(5, -1);
    D_SQ_CHECK(contents_equal(_exact, {-1, -1, -1, -1, -1}));

    vec_seq<int> _over{1, 2, 3};

    _over.shift_left(6, 0);
    D_SQ_CHECK(contents_equal(_over, {0, 0, 0}));

    return true;
}

// tests_sequential_shift_right
//   shift_right(n,fill): n==0 no-op, fill the head, and the n>=size clear.
bool
tests_sequential_shift_right()
{
    vec_seq<int> _v{1, 2, 3, 4, 5};

    _v.shift_right(0, 0);
    D_SQ_CHECK(contents_equal(_v, {1, 2, 3, 4, 5}));

    _v.shift_right(2, 0);
    D_SQ_CHECK(contents_equal(_v, {0, 0, 1, 2, 3}));

    vec_seq<int> _exact{1, 2, 3, 4, 5};

    _exact.shift_right(5, -1);
    D_SQ_CHECK(contents_equal(_exact, {-1, -1, -1, -1, -1}));

    vec_seq<int> _over{1, 2, 3};

    _over.shift_right(6, 0);
    D_SQ_CHECK(contents_equal(_over, {0, 0, 0}));

    return true;
}

// tests_sequential_storage_agnostic
//   the operations touch only begin()/end()/size(), so a fixed array-backed
// container behaves identically.
bool
tests_sequential_storage_agnostic()
{
    arr_seq<int, 5> _a{1, 2, 3, 4, 5};

    D_SQ_CHECK(_a.front() == 1);
    D_SQ_CHECK(_a.back() == 5);

    _a.reverse();
    D_SQ_CHECK(contents_equal(_a, {5, 4, 3, 2, 1}));

    _a.rotate_right(2);
    D_SQ_CHECK(contents_equal(_a, {2, 1, 5, 4, 3}));

    arr_seq<int, 3> _pal{2, 7, 2};

    D_SQ_CHECK(_pal.is_palindrome());

    return true;
}

NS_END  // testing
NS_END  // djinterp
