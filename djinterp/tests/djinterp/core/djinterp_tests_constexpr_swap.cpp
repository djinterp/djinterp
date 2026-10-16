/*******************************************************************************
* djinterp [core]                              djinterp_tests_constexpr_swap.cpp
*
*
* path:      /tests/djinterp/core/djinterp_tests_constexpr_swap.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "djinterp_tests.hpp"


NS_DJINTERP
NS_TESTING

// dt_swap_eval
//   helper: swaps two ints in a constant-expression context and encodes the
// post-swap state (a*10 + b), used to prove constexpr_swap is usable at
// compile time.  Initial (1, 2) must become (2, 1) -> 21.
constexpr int
dt_swap_eval()
{
    int _a = 1;
    int _b = 2;

    ::djinterp::constexpr_swap(_a, _b);

    return (_a * 10) + _b;
}


/*
tests_constexpr_swap_runtime
  Tests the following:
  - constexpr_swap exchanges two values at runtime for a trivial type (int)
  - works for a non-trivial, allocating type (std::string)
  - works for pointers (the pointers, and thus what they reference, swap)
  - works for a user type with user-provided move operations (throwing_move)
  - swapping two distinct but equal-valued objects leaves both unchanged
*/
bool
tests_constexpr_swap_runtime()
{
    bool _ok = true;

    // ints
    int _ia = 3;
    int _ib = 7;
    ::djinterp::constexpr_swap(_ia, _ib);
    D_DT_CHECK(_ok, _ia == 7);
    D_DT_CHECK(_ok, _ib == 3);

    // strings (non-trivial / allocating)
    std::string _sa = "hello";
    std::string _sb = "world";
    ::djinterp::constexpr_swap(_sa, _sb);
    D_DT_CHECK(_ok, _sa == "world");
    D_DT_CHECK(_ok, _sb == "hello");

    // pointers
    int  _x  = 100;
    int  _y  = 200;
    int* _pa = &_x;
    int* _pb = &_y;
    ::djinterp::constexpr_swap(_pa, _pb);
    D_DT_CHECK(_ok, _pa == &_y);
    D_DT_CHECK(_ok, _pb == &_x);
    D_DT_CHECK(_ok, *_pa == 200);
    D_DT_CHECK(_ok, *_pb == 100);

    // user type with user-provided move operations
    throwing_move _ta(11);
    throwing_move _tb(22);
    ::djinterp::constexpr_swap(_ta, _tb);
    D_DT_CHECK(_ok, _ta.value == 22);
    D_DT_CHECK(_ok, _tb.value == 11);

    // distinct but equal-valued objects remain equal afterwards
    int _ea = 5;
    int _eb = 5;
    ::djinterp::constexpr_swap(_ea, _eb);
    D_DT_CHECK(_ok, _ea == 5);
    D_DT_CHECK(_ok, _eb == 5);

    return _ok;
}

/*
tests_constexpr_swap_constexpr
  Tests the following:
  - constexpr_swap is evaluable in a constant-expression context (the C++20
    path delegates to the now-constexpr std::swap)
*/
bool
tests_constexpr_swap_constexpr()
{
    bool _ok = true;

    static_assert(dt_swap_eval() == 21,
                  "constexpr_swap must be usable at compile time.");

    constexpr int _encoded = dt_swap_eval();
    D_DT_CHECK(_ok, _encoded == 21);
    D_DT_CHECK(_ok, dt_swap_eval() == 21);

    return _ok;
}

/*
tests_constexpr_swap_noexcept
  Tests the following:
  - constexpr_swap is noexcept when the element's move operations are noexcept
    (int)
  - constexpr_swap is NOT noexcept when the element's move operations may throw
    (throwing_move), matching its conditional noexcept specification
  - the driving traits agree with the observed noexcept-ness
*/
bool
tests_constexpr_swap_noexcept()
{
    bool _ok = true;

    int _ia = 0;
    int _ib = 1;
    D_DT_CHECK(_ok, noexcept(::djinterp::constexpr_swap(_ia, _ib)) == true);

    throwing_move _ta(0);
    throwing_move _tb(1);
    D_DT_CHECK(_ok, noexcept(::djinterp::constexpr_swap(_ta, _tb)) == false);

    D_DT_CHECK(_ok, std::is_nothrow_move_constructible<int>::value == true);
    D_DT_CHECK(_ok, std::is_nothrow_move_constructible<throwing_move>::value == false);

    return _ok;
}

NS_END  // testing
NS_END  // djinterp
