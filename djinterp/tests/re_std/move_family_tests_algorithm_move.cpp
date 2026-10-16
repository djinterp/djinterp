/*******************************************************************************
* djinterp [re_std]                         move_family_tests_algorithm_move.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_algorithm_move.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_algorithm_move.cpp
//   re_std::move(first, last, d_first) — the range-move algorithm. Covers a plain
// move with the returned destination-end iterator, move-only semantics, the
// empty-range edge, and the documented coexistence of the 3-argument algorithm
// with the 1-argument cast of the same name (overload resolution keeps them
// distinct).

// std
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_algorithm_move_basic
  Verifies the algorithm moves a range and returns the destination end.
  Tests the following:
  - every element is transferred in order
  - the return value is d_first advanced by the element count
*/
bool
tests_algorithm_move_basic()
{
    int src[4] = { 1, 2, 3, 4 };
    int dst[4] = { 0, 0, 0, 0 };

    int* end = re_std::move(&src[0], &src[4], &dst[0]);

    bool ok = true;

    ok = ok && (end == &dst[4]);
    ok = ok && (dst[0] == 1 && dst[1] == 2 && dst[2] == 3 && dst[3] == 4);

    return ok;
}

/*
tests_algorithm_move_semantics
  Verifies the algorithm moves elements rather than copying them.
  Tests the following:
  - a range of move-only elements is transferred by move assignment only
*/
bool
tests_algorithm_move_semantics()
{
    counters  c;
    move_only src[3];
    src[0] = move_only(&c, 1);
    src[1] = move_only(&c, 2);
    src[2] = move_only(&c, 3);

    move_only dst[3];
    c.moves  = 0;
    c.copies = 0;

    re_std::move(&src[0], &src[3], &dst[0]);

    bool ok = true;

    ok = ok && (c.moves == 3);
    ok = ok && (c.copies == 0);
    ok = ok && (dst[0].id == 1 && dst[2].id == 3);

    return ok;
}

/*
tests_algorithm_move_empty
  Verifies an empty input range is a no-op.
  Tests the following:
  - the return value equals the unmoved destination begin
  - the destination is untouched
*/
bool
tests_algorithm_move_empty()
{
    int src[1] = { 9 };
    int dst[1] = { 5 };

    int* end = re_std::move(&src[0], &src[0], &dst[0]);

    bool ok = true;

    ok = ok && (end == &dst[0]);
    ok = ok && (dst[0] == 5);

    return ok;
}

/*
tests_algorithm_move_overload_coexist
  Verifies the 3-argument algorithm and the 1-argument cast coexist.
  Tests the following:
  - the 1-argument form is the rvalue cast (yields T&&)
  - the 3-argument form is the algorithm (yields the destination iterator)
  - both are callable from the same scope without ambiguity
*/
bool
tests_algorithm_move_overload_coexist()
{
    int a = 3;

    bool ok = true;

    ok = ok && (std::is_same<decltype(re_std::move(a)), int&&>::value);

    int src[2] = { 7, 8 };
    int dst[2] = { 0, 0 };
    ok = ok && (std::is_same<
                    decltype(re_std::move(&src[0], &src[2], &dst[0])),
                    int*>::value);

    int* end = re_std::move(&src[0], &src[2], &dst[0]);
    ok = ok && (end == &dst[2] && dst[0] == 7 && dst[1] == 8);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
