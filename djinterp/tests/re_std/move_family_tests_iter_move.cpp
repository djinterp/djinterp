/*******************************************************************************
* djinterp [re_std]                              move_family_tests_iter_move.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_iter_move.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_iter_move.cpp
//   re_std::iter_move — the C++20-style customisation-point object, plus the
// iter_rvalue_reference_t alias. Covers the default path (cast the dereference
// to its rvalue reference), the proxy pass-through (a by-value dereference is
// left as-is), the ADL customisation path (a user-provided iter_move wins over
// the default), and a behavioural check that the default path really moves.

// std
#include <type_traits>
#include <utility>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // custom_iter : an "iterator" carrying its own iter_move found by ADL. Its
    // customisation returns a distinctive sentinel so the test can prove the
    // ADL path (not the default cast) was taken.
    struct custom_iter
    {
        int* p;
        explicit custom_iter(int* _p = nullptr) : p(_p) {}
        int operator*() const { return *p; }
    };

    // ADL customisation: yields a sentinel unrelated to *p.
    long iter_move(const custom_iter&)
    {
        return 4242L;
    }
}


/*
tests_iter_move_default_type
  Verifies the default path casts the dereference to its rvalue reference.
  Tests the following:
  - iter_move(int*) has type int&&
*/
bool
tests_iter_move_default_type()
{
    int a = 5;

    return std::is_same<decltype(re_std::iter_move(&a)), int&&>::value;
}

/*
tests_iter_move_rvalue_reference_alias
  Verifies iter_rvalue_reference_t reports iter_move's result type.
  Tests the following:
  - iter_rvalue_reference_t<int*> is int&&
*/
bool
tests_iter_move_rvalue_reference_alias()
{
    return std::is_same<re_std::iter_rvalue_reference_t<int*>, int&&>::value;
}

/*
tests_iter_move_proxy_passthrough
  Verifies a by-value dereference is passed through, not cast to &&.
  Tests the following:
  - iter_rvalue_reference_t<proxy_iter> is proxy_ref unchanged
*/
bool
tests_iter_move_proxy_passthrough()
{
    return std::is_same<
        re_std::iter_rvalue_reference_t<proxy_iter>,
        proxy_ref>::value;
}

/*
tests_iter_move_adl_customization
  Verifies a user-provided iter_move is preferred over the default.
  Tests the following:
  - iter_move on a type with an ADL iter_move calls that customisation
*/
bool
tests_iter_move_adl_customization()
{
    int         a = 1;
    custom_iter it(&a);

    return (re_std::iter_move(it) == 4242L);
}

/*
tests_iter_move_moves
  Verifies the default path actually enables moving.
  Tests the following:
  - constructing an element from iter_move(ptr) takes the move path
*/
bool
tests_iter_move_moves()
{
    counters c;
    probe    src[1];
    src[0] = probe(&c, 3);

    c.moves  = 0;
    c.copies = 0;

    probe dst(re_std::iter_move(&src[0]));

    bool ok = true;

    ok = ok && (c.moves == 1);
    ok = ok && (c.copies == 0);
    ok = ok && (dst.id == 3);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
