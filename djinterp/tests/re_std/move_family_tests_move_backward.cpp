/*******************************************************************************
* djinterp [re_std]                          move_family_tests_move_backward.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_move_backward.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_move_backward.cpp
//   re_std::move_backward(first, last, d_last) — moves a range from the back so
// that overlapping right-shifts stay correct. Covers a plain backward move with
// the returned destination-begin iterator, the overlap case it exists for,
// move-only semantics, and the empty-range edge.

// std
#include <utility>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_move_backward_basic
  Verifies the algorithm moves a range in reverse and returns the destination
  begin.
  Tests the following:
  - every element lands at the correct position, last source at d_last - 1
  - the return value is d_last minus the element count
*/
bool
tests_move_backward_basic()
{
    int src[3] = { 1, 2, 3 };
    int dst[3] = { 0, 0, 0 };

    int* begin = re_std::move_backward(&src[0], &src[3], &dst[3]);

    bool ok = true;

    ok = ok && (begin == &dst[0]);
    ok = ok && (dst[0] == 1 && dst[1] == 2 && dst[2] == 3);

    return ok;
}

/*
tests_move_backward_overlap
  Verifies the right-shift-on-overlap case the algorithm exists for.
  Tests the following:
  - moving [0,3) of a buffer to end at index 5 shifts the values right without
    clobbering not-yet-moved source elements
*/
bool
tests_move_backward_overlap()
{
    int buf[5] = { 1, 2, 3, 4, 5 };

    // move buf[0..3) into the window ending at buf[5]  ->  {1,2,1,2,3}
    int* begin = re_std::move_backward(&buf[0], &buf[3], &buf[5]);

    bool ok = true;

    ok = ok && (begin == &buf[2]);
    ok = ok && (buf[2] == 1 && buf[3] == 2 && buf[4] == 3);
    ok = ok && (buf[0] == 1 && buf[1] == 2);

    return ok;
}

/*
tests_move_backward_semantics
  Verifies the algorithm moves elements rather than copying them.
  Tests the following:
  - a range of move-only elements is transferred by move assignment only
*/
bool
tests_move_backward_semantics()
{
    counters  c;
    move_only src[3];
    src[0] = move_only(&c, 1);
    src[1] = move_only(&c, 2);
    src[2] = move_only(&c, 3);

    move_only dst[3];
    c.moves  = 0;
    c.copies = 0;

    re_std::move_backward(&src[0], &src[3], &dst[3]);

    bool ok = true;

    ok = ok && (c.moves == 3);
    ok = ok && (c.copies == 0);
    ok = ok && (dst[0].id == 1 && dst[2].id == 3);

    return ok;
}

/*
tests_move_backward_empty
  Verifies an empty input range is a no-op.
  Tests the following:
  - the return value equals the unmoved destination end
  - the destination is untouched
*/
bool
tests_move_backward_empty()
{
    int src[1] = { 9 };
    int dst[1] = { 5 };

    int* begin = re_std::move_backward(&src[0], &src[0], &dst[1]);

    bool ok = true;

    ok = ok && (begin == &dst[1]);
    ok = ok && (dst[0] == 5);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
