/*******************************************************************************
* djinterp [re_std]                           move_family_tests_utility_move.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_utility_move.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_utility_move.cpp
//   re_std::move — the canonical cast to an rvalue reference. Covers the result
// type (remove_reference&& for lvalue and const-lvalue inputs), the noexcept
// guarantee, the actual move-enabling behaviour on a move-only type, and
// constexpr usability (the last exercised only where the dialect provides
// relaxed constexpr, C++14+).

// std
#include <type_traits>
#include <utility>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
namespace
{
    // cx_move : a relaxed-constexpr round-trip through re_std::move.
    constexpr int
    cx_move()
    {
        int x = 7;
        int y = re_std::move(x);

        return y;
    }
}
#endif


/*
tests_move_type
  Verifies re_std::move produces the correct rvalue-reference type.
  Tests the following:
  - move(T&) yields T&&
  - move(const T&) yields const T&&
  - move(T&&) yields T&& (identity on an rvalue)
*/
bool
tests_move_type()
{
    bool ok = true;

    ok = ok && (std::is_same<
                    decltype(re_std::move(std::declval<int&>())),
                    int&&>::value);
    ok = ok && (std::is_same<
                    decltype(re_std::move(std::declval<const int&>())),
                    const int&&>::value);
    ok = ok && (std::is_same<
                    decltype(re_std::move(std::declval<int&&>())),
                    int&&>::value);

    return ok;
}

/*
tests_move_noexcept
  Verifies the cast is declared noexcept.
  Tests the following:
  - move(T&) is a noexcept expression
*/
bool
tests_move_noexcept()
{
    return noexcept(re_std::move(std::declval<int&>()));
}

/*
tests_move_semantics
  Verifies re_std::move actually enables move construction.
  Tests the following:
  - constructing from move(src) takes the move path, not the copy path
*/
bool
tests_move_semantics()
{
    counters  c;
    move_only src(&c, 1);

    move_only dst(re_std::move(src));

    bool ok = true;

    ok = ok && (c.moves == 1);
    ok = ok && (c.copies == 0);
    ok = ok && (dst.id == 1);

    return ok;
}

/*
tests_move_constexpr
  Verifies re_std::move is usable in a constant expression.
  Tests the following:
  - a relaxed-constexpr function moving a local evaluates at compile time
    (checked only where the dialect supports it, C++14+)
*/
bool
tests_move_constexpr()
{
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
    static_assert(cx_move() == 7, "re_std::move usable in constexpr");

    return (cx_move() == 7);
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
