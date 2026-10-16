/*******************************************************************************
* djinterp [re_std]                          move_family_tests_move_iterator.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_move_iterator.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_move_iterator.cpp
//   re_std::move_iterator — the adaptor whose dereference yields an xvalue so
// that consuming algorithms move-construct. Covers the two reference shapes
// (T&& for a real-reference base, pass-through for a proxy/by-value base), the
// move-through-dereference behaviour, base(), the full arithmetic and
// relational surface, the converting constructor, operator[], operator->,
// make_move_iterator, and the preserved iterator category (plus the C++20
// iterator_concept).

// std
#include <iterator>
#include <type_traits>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_move_iterator_reference_ref
  Verifies the reference type for a real-reference base iterator.
  Tests the following:
  - move_iterator<int*>::reference is int&&
*/
bool
tests_move_iterator_reference_ref()
{
    return std::is_same<re_std::move_iterator<int*>::reference, int&&>::value;
}

/*
tests_move_iterator_reference_proxy
  Verifies the reference type for a proxy (by-value reference) base iterator.
  Tests the following:
  - move_iterator<proxy_iter>::reference is proxy_ref unchanged (no dangling &&)
*/
bool
tests_move_iterator_reference_proxy()
{
    return std::is_same<
        re_std::move_iterator<proxy_iter>::reference,
        proxy_ref>::value;
}

/*
tests_move_iterator_deref_moves
  Verifies dereferencing a move_iterator yields a movable xvalue.
  Tests the following:
  - constructing an element from *move_iterator takes the move path
*/
bool
tests_move_iterator_deref_moves()
{
    counters c;
    probe    src[1];
    src[0] = probe(&c, 7);

    re_std::move_iterator<probe*> it = re_std::make_move_iterator(&src[0]);

    c.moves  = 0;
    c.copies = 0;

    probe dst(*it);

    bool ok = true;

    ok = ok && (c.moves == 1);
    ok = ok && (c.copies == 0);
    ok = ok && (dst.id == 7);

    return ok;
}

/*
tests_move_iterator_base
  Verifies base() exposes the wrapped iterator.
  Tests the following:
  - base() returns the underlying iterator value
*/
bool
tests_move_iterator_base()
{
    int arr[3] = { 1, 2, 3 };

    re_std::move_iterator<int*> it = re_std::make_move_iterator(&arr[1]);

    return (it.base() == &arr[1]);
}

/*
tests_move_iterator_arithmetic
  Verifies the arithmetic surface.
  Tests the following:
  - prefix / postfix ++ and -- advance and retreat by one
  - += and -= shift by n
  - it + n, n + it, and it - n produce shifted iterators
  - the iterator difference equals the base difference
*/
bool
tests_move_iterator_arithmetic()
{
    int arr[5] = { 0, 1, 2, 3, 4 };

    re_std::move_iterator<int*> b = re_std::make_move_iterator(&arr[0]);
    re_std::move_iterator<int*> e = re_std::make_move_iterator(&arr[5]);

    bool ok = true;

    ok = ok && ((e - b) == 5);

    re_std::move_iterator<int*> it = b;
    ++it;
    ok = ok && (it.base() == &arr[1]);
    --it;
    ok = ok && (it.base() == &arr[0]);

    re_std::move_iterator<int*> post = it++;
    ok = ok && (post.base() == &arr[0] && it.base() == &arr[1]);
    it--;
    ok = ok && (it.base() == &arr[0]);

    it += 3;
    ok = ok && (it.base() == &arr[3]);
    it -= 2;
    ok = ok && (it.base() == &arr[1]);

    ok = ok && ((b + 2).base() == &arr[2]);
    ok = ok && ((2 + b).base() == &arr[2]);
    ok = ok && ((e - 1).base() == &arr[4]);

    return ok;
}

/*
tests_move_iterator_relational
  Verifies the six relational operators forward to the base (non-flipped).
  Tests the following:
  - ==, != compare base equality
  - <, >, <=, >= order by base position (not reversed)
*/
bool
tests_move_iterator_relational()
{
    int arr[3] = { 0, 1, 2 };

    re_std::move_iterator<int*> lo = re_std::make_move_iterator(&arr[0]);
    re_std::move_iterator<int*> hi = re_std::make_move_iterator(&arr[2]);
    re_std::move_iterator<int*> lo2 = re_std::make_move_iterator(&arr[0]);

    bool ok = true;

    ok = ok && (lo == lo2);
    ok = ok && (lo != hi);
    ok = ok && (lo < hi);
    ok = ok && (hi > lo);
    ok = ok && (lo <= lo2);
    ok = ok && (hi >= lo);
    ok = ok && (!(hi < lo));

    return ok;
}

/*
tests_move_iterator_converting_ctor
  Verifies the converting constructor from move_iterator<U>.
  Tests the following:
  - a move_iterator<int*> converts to move_iterator<const int*>, preserving base
*/
bool
tests_move_iterator_converting_ctor()
{
    int arr[2] = { 4, 5 };

    re_std::move_iterator<int*>       it = re_std::make_move_iterator(&arr[0]);
    re_std::move_iterator<const int*> cv(it);

    return (cv.base() == &arr[0]);
}

/*
tests_move_iterator_index
  Verifies operator[] yields a movable xvalue at an offset.
  Tests the following:
  - constructing from it[n] takes the move path and reads the right element
*/
bool
tests_move_iterator_index()
{
    counters c;
    probe    src[3];
    src[0] = probe(&c, 1);
    src[1] = probe(&c, 2);
    src[2] = probe(&c, 3);

    re_std::move_iterator<probe*> it = re_std::make_move_iterator(&src[0]);

    c.moves  = 0;
    c.copies = 0;

    probe dst(it[2]);

    bool ok = true;

    ok = ok && (c.moves == 1);
    ok = ok && (c.copies == 0);
    ok = ok && (dst.id == 3);

    return ok;
}

/*
tests_move_iterator_arrow
  Verifies operator-> returns the underlying iterator.
  Tests the following:
  - operator-> yields the base iterator (retained for C++11..C++20)
*/
bool
tests_move_iterator_arrow()
{
    int arr[2] = { 8, 9 };

    re_std::move_iterator<int*> it = re_std::make_move_iterator(&arr[1]);

    return (it.operator->() == &arr[1]);
}

/*
tests_move_iterator_make
  Verifies make_move_iterator wraps an iterator into a move_iterator.
  Tests the following:
  - the result type is move_iterator<Iter>
  - the result's base is the supplied iterator
*/
bool
tests_move_iterator_make()
{
    int arr[1] = { 5 };

    re_std::move_iterator<int*> it = re_std::make_move_iterator(&arr[0]);

    bool ok = true;

    ok = ok && (std::is_same<decltype(it), re_std::move_iterator<int*> >::value);
    ok = ok && (it.base() == &arr[0]);

    return ok;
}

/*
tests_move_iterator_category
  Verifies the iterator category is preserved and the concept is input.
  Tests the following:
  - move_iterator<int*>::iterator_category is the base's random-access tag
  - move_iterator<int*>::iterator_concept is input_iterator_tag (C++20 only)
*/
bool
tests_move_iterator_category()
{
    bool ok = std::is_same<
        re_std::move_iterator<int*>::iterator_category,
        std::random_access_iterator_tag>::value;

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    ok = ok && (std::is_same<
                    re_std::move_iterator<int*>::iterator_concept,
                    re_std::input_iterator_tag>::value);
#endif

    return ok;
}


NS_END  // testing
NS_END  // djinterp
