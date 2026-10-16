/*******************************************************************************
* djinterp [re_std]                       move_family_tests_move_if_noexcept.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_move_if_noexcept.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_move_if_noexcept.cpp
//   re_std::move_if_noexcept — returns an rvalue reference (allowing a move) when
// the type's move is nothrow OR the type is not copyable, and a const lvalue
// reference (forcing a copy) otherwise. Covers all three branches of that rule
// by result type, plus a behavioural check that the chosen cast really moves or
// really copies.

// std
#include <type_traits>
#include <utility>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // nothrow_move : nothrow move ctor + a copy ctor -> move_if_noexcept moves.
    struct nothrow_move
    {
        nothrow_move()                    {}
        nothrow_move(const nothrow_move&) {}
        nothrow_move(nothrow_move&&) noexcept {}
    };

    // throwing_move : potentially-throwing move ctor + a copy ctor
    // -> move_if_noexcept returns const& to preserve the strong guarantee.
    struct throwing_move
    {
        throwing_move()                     {}
        throwing_move(const throwing_move&) {}
        throwing_move(throwing_move&&)      {}   // not noexcept
    };

    // moveonly_throwing : potentially-throwing move ctor, NO copy ctor
    // -> move_if_noexcept must still return T&& (no copy to fall back on).
    struct moveonly_throwing
    {
        moveonly_throwing()                              {}
        moveonly_throwing(const moveonly_throwing&)            = delete;
        moveonly_throwing(moveonly_throwing&&)           {}   // not noexcept
    };

    // behaviour probes: a nothrow-move copyable type and a throwing-move
    // copyable type, both tallying which path a construction took.
    struct nothrow_probe
    {
        counters* c;
        explicit nothrow_probe(counters* _c = nullptr) noexcept : c(_c) {}
        nothrow_probe(const nothrow_probe& _o)          : c(_o.c) { if (c) { ++c->copies; } }
        nothrow_probe(nothrow_probe&& _o) noexcept      : c(_o.c) { if (c) { ++c->moves;  } }
    };

    struct throwing_probe
    {
        counters* c;
        explicit throwing_probe(counters* _c = nullptr) noexcept : c(_c) {}
        throwing_probe(const throwing_probe& _o)        : c(_o.c) { if (c) { ++c->copies; } }
        throwing_probe(throwing_probe&& _o)             : c(_o.c) { if (c) { ++c->moves;  } }  // not noexcept
    };
}


/*
tests_move_if_noexcept_nothrow_type
  Verifies the nothrow-move branch.
  Tests the following:
  - move_if_noexcept(T&) yields T&& when T's move ctor is noexcept
*/
bool
tests_move_if_noexcept_nothrow_type()
{
    return std::is_same<
        decltype(re_std::move_if_noexcept(std::declval<nothrow_move&>())),
        nothrow_move&&>::value;
}

/*
tests_move_if_noexcept_throwing_copyable
  Verifies the throwing-but-copyable branch.
  Tests the following:
  - move_if_noexcept(T&) yields const T& when T's move can throw and T is copyable
*/
bool
tests_move_if_noexcept_throwing_copyable()
{
    return std::is_same<
        decltype(re_std::move_if_noexcept(std::declval<throwing_move&>())),
        const throwing_move&>::value;
}

/*
tests_move_if_noexcept_move_only
  Verifies the move-only branch overrides the throwing check.
  Tests the following:
  - move_if_noexcept(T&) yields T&& when T has no copy ctor, even if its move
    can throw
*/
bool
tests_move_if_noexcept_move_only()
{
    return std::is_same<
        decltype(re_std::move_if_noexcept(std::declval<moveonly_throwing&>())),
        moveonly_throwing&&>::value;
}

/*
tests_move_if_noexcept_semantics
  Verifies the chosen cast really moves or really copies.
  Tests the following:
  - a nothrow-move type is moved by move_if_noexcept
  - a throwing-move copyable type is copied by move_if_noexcept
*/
bool
tests_move_if_noexcept_semantics()
{
    bool ok = true;

    {
        counters      c;
        nothrow_probe src(&c);
        nothrow_probe dst(re_std::move_if_noexcept(src));
        (void)dst;
        ok = ok && (c.moves == 1 && c.copies == 0);
    }
    {
        counters       c;
        throwing_probe src(&c);
        throwing_probe dst(re_std::move_if_noexcept(src));
        (void)dst;
        ok = ok && (c.moves == 0 && c.copies == 1);
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
