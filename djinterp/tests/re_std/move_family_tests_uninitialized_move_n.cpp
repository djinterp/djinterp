/*******************************************************************************
* djinterp [re_std]                   move_family_tests_uninitialized_move_n.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_uninitialized_move_n.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_uninitialized_move_n.cpp
//   re_std::uninitialized_move_n(first, n, d_first) — the sized variant. Covers
// the returned {input-advanced-by-n, destination-end} pair, move-only
// semantics, and the same strong exception guarantee w.r.t. the destination.

// std
#include <cstddef>
#include <new>
#include <stdexcept>
#include <utility>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    template<typename _T, std::size_t _N>
    union raw_block
    {
        _T            _typed[_N];
        unsigned char _bytes[sizeof(_T) * _N];

        raw_block()  {}
        ~raw_block() {}
    };

    struct throw_probe_n
    {
        static int live;
        static int made;
        static int throw_at;

        int id;

        explicit throw_probe_n(int _id = 0) : id(_id) {}

        throw_probe_n(const throw_probe_n& _o) : id(_o.id) { ++live; }

        throw_probe_n(throw_probe_n&& _o)
            : id(_o.id)
        {
            if (made == throw_at)
            {
                throw std::runtime_error("throw_probe_n move");
            }
            ++made;
            ++live;
        }

        ~throw_probe_n() { --live; }
    };

    int throw_probe_n::live     = 0;
    int throw_probe_n::made     = 0;
    int throw_probe_n::throw_at = -1;
}


/*
tests_uninitialized_move_n_basic
  Verifies the sized move-construct and its returned iterator pair.
  Tests the following:
  - every element is move-constructed at its destination slot
  - the returned pair is {src advanced by n, destination end}
*/
bool
tests_uninitialized_move_n_basic()
{
    int src[4] = { 10, 20, 30, 40 };
    raw_block<int, 4> block;
    int* d = block._typed;

    std::pair<int*, int*> pr = re_std::uninitialized_move_n(&src[0], 4, d);

    bool ok = true;

    ok = ok && (pr.first == &src[4]);
    ok = ok && (pr.second == d + 4);
    ok = ok && (d[0] == 10 && d[3] == 40);

    return ok;
}

/*
tests_uninitialized_move_n_semantics
  Verifies elements are move-constructed, not copy-constructed.
  Tests the following:
  - a range of move-only elements is transferred by move construction only
*/
bool
tests_uninitialized_move_n_semantics()
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

    std::pair<move_only*, move_only*> pr =
        re_std::uninitialized_move_n(&src[0], 3, d);

    bool ok = true;

    ok = ok && (c.moves == 3);
    ok = ok && (c.copies == 0);
    ok = ok && (pr.first == &src[3]);
    ok = ok && (pr.second == d + 3);

    for (int i = 0; i < 3; ++i)
    {
        d[i].~move_only();
    }

    return ok;
}

/*
tests_uninitialized_move_n_exception_safety
  Verifies the strong guarantee w.r.t. the destination range.
  Tests the following:
  - a throw during the third move-construction propagates
  - the destination elements built before the throw are all destroyed
*/
bool
tests_uninitialized_move_n_exception_safety()
{
    throw_probe_n src[5];

    raw_block<throw_probe_n, 5> block;
    throw_probe_n* d = block._typed;

    throw_probe_n::made     = 0;
    throw_probe_n::throw_at = 2;
    const int live_before = throw_probe_n::live;

    bool threw = false;

    try
    {
        re_std::uninitialized_move_n(&src[0], 5, d);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    bool ok = true;

    ok = ok && (threw);
    ok = ok && (throw_probe_n::live == live_before);

    throw_probe_n::throw_at = -1;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
