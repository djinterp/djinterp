/*******************************************************************************
* djinterp [re_std]                     move_family_tests_uninitialized_move.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_uninitialized_move.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_uninitialized_move.cpp
//   re_std::uninitialized_move(first, last, d_first) — move-constructs a range
// into uninitialised storage. Covers the plain case with the returned dest-end
// iterator, move-only semantics, and the strong exception guarantee w.r.t. the
// destination: when a move-construction throws partway, every destination
// element already built is destroyed before the exception propagates.

// std
#include <cstddef>
#include <new>
#include <stdexcept>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // raw_block : suitably-aligned uninitialised storage for _N objects of _T.
    template<typename _T, std::size_t _N>
    union raw_block
    {
        _T            _typed[_N];
        unsigned char _bytes[sizeof(_T) * _N];

        raw_block()  {}   // leave storage uninitialised
        ~raw_block() {}   // caller is responsible for destroying live objects
    };

    // throw_probe : move-construction throws on the throw_at-th call; live()
    // tracks how many destination objects are currently constructed.
    struct throw_probe
    {
        static int live;
        static int made;
        static int throw_at;

        int id;

        explicit throw_probe(int _id = 0) : id(_id) {}

        throw_probe(const throw_probe& _o) : id(_o.id) { ++live; }

        throw_probe(throw_probe&& _o)
            : id(_o.id)
        {
            if (made == throw_at)
            {
                throw std::runtime_error("throw_probe move");
            }
            ++made;
            ++live;
        }

        ~throw_probe() { --live; }
    };

    int throw_probe::live     = 0;
    int throw_probe::made     = 0;
    int throw_probe::throw_at = -1;
}


/*
tests_uninitialized_move_basic
  Verifies a plain move-construct into raw storage.
  Tests the following:
  - every element is move-constructed at its destination slot
  - the return value is d_first advanced by the element count
*/
bool
tests_uninitialized_move_basic()
{
    int src[4] = { 10, 20, 30, 40 };
    raw_block<int, 4> block;
    int* d = block._typed;

    int* end = re_std::uninitialized_move(&src[0], &src[4], d);

    bool ok = true;

    ok = ok && (end == d + 4);
    ok = ok && (d[0] == 10 && d[1] == 20 && d[2] == 30 && d[3] == 40);

    return ok;
}

/*
tests_uninitialized_move_semantics
  Verifies elements are move-constructed, not copy-constructed.
  Tests the following:
  - a range of move-only elements is transferred by move construction only
*/
bool
tests_uninitialized_move_semantics()
{
    counters  c;
    move_only src[3];
    src[0] = move_only(&c, 1);
    src[1] = move_only(&c, 2);
    src[2] = move_only(&c, 3);

    raw_block<move_only, 3> block;
    move_only* d = block._typed;

    c.moves  = 0;
    c.copies = 0;

    re_std::uninitialized_move(&src[0], &src[3], d);

    bool ok = true;

    ok = ok && (c.moves == 3);
    ok = ok && (c.copies == 0);
    ok = ok && (d[0].id == 1 && d[2].id == 3);

    for (int i = 0; i < 3; ++i)
    {
        d[i].~move_only();
    }

    return ok;
}

/*
tests_uninitialized_move_exception_safety
  Verifies the strong guarantee w.r.t. the destination range.
  Tests the following:
  - a throw during the third move-construction propagates
  - the destination elements built before the throw are all destroyed
    (the live count returns to its pre-call value)
*/
bool
tests_uninitialized_move_exception_safety()
{
    throw_probe src[5];
    src[0].id = 1; src[1].id = 2; src[2].id = 3; src[3].id = 4; src[4].id = 5;

    raw_block<throw_probe, 5> block;
    throw_probe* d = block._typed;

    throw_probe::made     = 0;
    throw_probe::throw_at = 2;                 // third move-construction throws
    const int live_before = throw_probe::live;

    bool threw = false;

    try
    {
        re_std::uninitialized_move(&src[0], &src[5], d);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    bool ok = true;

    ok = ok && (threw);
    ok = ok && (throw_probe::live == live_before);   // all built destinations destroyed

    throw_probe::throw_at = -1;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
