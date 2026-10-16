/*******************************************************************************
* djinterp [re_std]                                 move_family_tests_traits.cpp
*
*
* path:      /tests/djinterp/re_std/move_family_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : move_family_tests_traits.cpp
//   The six move-family type traits: is_move_constructible / is_move_assignable
// and their nothrow_ and trivially_ variants. Each is exercised over a battery
// of types spanning the meaningful cases (trivial, user-provided nothrow,
// user-provided throwing, move-deleted) and cross-checked against the
// corresponding <type_traits> trait so the re_std composition is verified to
// match the standard exactly. The _v variable-template forms are checked where
// the dialect provides them (C++14+).

// std
#include <type_traits>
// djinterp
#include "move_family_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace
{
    // trivial       : trivially movable and assignable.
    struct trivial
    {
        int x;
    };

    // nothrow_user  : user-provided, noexcept move operations (non-trivial).
    struct nothrow_user
    {
        int x;
        nothrow_user(nothrow_user&&) noexcept            : x(0) {}
        nothrow_user& operator=(nothrow_user&&) noexcept { return *this; }
    };

    // throwing_user : user-provided, potentially-throwing move operations.
    struct throwing_user
    {
        int x;
        throwing_user(throwing_user&&)            : x(0) {}
        throwing_user& operator=(throwing_user&&) { return *this; }
    };

    // no_move       : copyable but with move operations deleted.
    struct no_move
    {
        no_move(const no_move&)            {}
        no_move& operator=(const no_move&) { return *this; }
        no_move(no_move&&)                 = delete;
        no_move& operator=(no_move&&)      = delete;
    };
}


/*
tests_is_move_constructible
  Verifies is_move_constructible against a battery of types.
  Tests the following:
  - true for int, trivial, nothrow_user, throwing_user (all constructible from T&&)
  - false for no_move (move ctor deleted)
  - agrees with std::is_move_constructible for every case
*/
bool
tests_is_move_constructible()
{
    bool ok = true;

    ok = ok && ( re_std::is_move_constructible<int>::value);
    ok = ok && ( re_std::is_move_constructible<trivial>::value);
    ok = ok && ( re_std::is_move_constructible<nothrow_user>::value);
    ok = ok && ( re_std::is_move_constructible<throwing_user>::value);
    ok = ok && (!re_std::is_move_constructible<no_move>::value);

    ok = ok && (re_std::is_move_constructible<int>::value           == std::is_move_constructible<int>::value);
    ok = ok && (re_std::is_move_constructible<nothrow_user>::value  == std::is_move_constructible<nothrow_user>::value);
    ok = ok && (re_std::is_move_constructible<throwing_user>::value == std::is_move_constructible<throwing_user>::value);
    ok = ok && (re_std::is_move_constructible<no_move>::value       == std::is_move_constructible<no_move>::value);

    return ok;
}

/*
tests_is_move_assignable
  Verifies is_move_assignable against a battery of types.
  Tests the following:
  - true for int, trivial, nothrow_user, throwing_user
  - false for no_move (move assignment deleted)
  - agrees with std::is_move_assignable for every case
*/
bool
tests_is_move_assignable()
{
    bool ok = true;

    ok = ok && ( re_std::is_move_assignable<int>::value);
    ok = ok && ( re_std::is_move_assignable<trivial>::value);
    ok = ok && ( re_std::is_move_assignable<nothrow_user>::value);
    ok = ok && ( re_std::is_move_assignable<throwing_user>::value);
    ok = ok && (!re_std::is_move_assignable<no_move>::value);

    ok = ok && (re_std::is_move_assignable<int>::value           == std::is_move_assignable<int>::value);
    ok = ok && (re_std::is_move_assignable<nothrow_user>::value  == std::is_move_assignable<nothrow_user>::value);
    ok = ok && (re_std::is_move_assignable<throwing_user>::value == std::is_move_assignable<throwing_user>::value);
    ok = ok && (re_std::is_move_assignable<no_move>::value       == std::is_move_assignable<no_move>::value);

    return ok;
}

/*
tests_is_nothrow_move_constructible
  Verifies is_nothrow_move_constructible against a battery of types.
  Tests the following:
  - true for int, trivial, nothrow_user
  - false for throwing_user (move ctor can throw) and no_move (deleted)
  - agrees with std::is_nothrow_move_constructible for every case
*/
bool
tests_is_nothrow_move_constructible()
{
    bool ok = true;

    ok = ok && ( re_std::is_nothrow_move_constructible<int>::value);
    ok = ok && ( re_std::is_nothrow_move_constructible<trivial>::value);
    ok = ok && ( re_std::is_nothrow_move_constructible<nothrow_user>::value);
    ok = ok && (!re_std::is_nothrow_move_constructible<throwing_user>::value);
    ok = ok && (!re_std::is_nothrow_move_constructible<no_move>::value);

    ok = ok && (re_std::is_nothrow_move_constructible<nothrow_user>::value  == std::is_nothrow_move_constructible<nothrow_user>::value);
    ok = ok && (re_std::is_nothrow_move_constructible<throwing_user>::value == std::is_nothrow_move_constructible<throwing_user>::value);

    return ok;
}

/*
tests_is_nothrow_move_assignable
  Verifies is_nothrow_move_assignable against a battery of types.
  Tests the following:
  - true for int, trivial, nothrow_user
  - false for throwing_user and no_move
  - agrees with std::is_nothrow_move_assignable for every case
*/
bool
tests_is_nothrow_move_assignable()
{
    bool ok = true;

    ok = ok && ( re_std::is_nothrow_move_assignable<int>::value);
    ok = ok && ( re_std::is_nothrow_move_assignable<trivial>::value);
    ok = ok && ( re_std::is_nothrow_move_assignable<nothrow_user>::value);
    ok = ok && (!re_std::is_nothrow_move_assignable<throwing_user>::value);
    ok = ok && (!re_std::is_nothrow_move_assignable<no_move>::value);

    ok = ok && (re_std::is_nothrow_move_assignable<nothrow_user>::value  == std::is_nothrow_move_assignable<nothrow_user>::value);
    ok = ok && (re_std::is_nothrow_move_assignable<throwing_user>::value == std::is_nothrow_move_assignable<throwing_user>::value);

    return ok;
}

/*
tests_is_trivially_move_constructible
  Verifies is_trivially_move_constructible against a battery of types.
  Tests the following:
  - true for int and trivial
  - false for nothrow_user and throwing_user (user-provided move ctor)
  - agrees with std::is_trivially_move_constructible for every case
*/
bool
tests_is_trivially_move_constructible()
{
    bool ok = true;

    ok = ok && ( re_std::is_trivially_move_constructible<int>::value);
    ok = ok && ( re_std::is_trivially_move_constructible<trivial>::value);
    ok = ok && (!re_std::is_trivially_move_constructible<nothrow_user>::value);
    ok = ok && (!re_std::is_trivially_move_constructible<throwing_user>::value);

    ok = ok && (re_std::is_trivially_move_constructible<trivial>::value      == std::is_trivially_move_constructible<trivial>::value);
    ok = ok && (re_std::is_trivially_move_constructible<nothrow_user>::value == std::is_trivially_move_constructible<nothrow_user>::value);

    return ok;
}

/*
tests_is_trivially_move_assignable
  Verifies is_trivially_move_assignable against a battery of types.
  Tests the following:
  - true for int and trivial
  - false for nothrow_user and throwing_user
  - agrees with std::is_trivially_move_assignable for every case
*/
bool
tests_is_trivially_move_assignable()
{
    bool ok = true;

    ok = ok && ( re_std::is_trivially_move_assignable<int>::value);
    ok = ok && ( re_std::is_trivially_move_assignable<trivial>::value);
    ok = ok && (!re_std::is_trivially_move_assignable<nothrow_user>::value);
    ok = ok && (!re_std::is_trivially_move_assignable<throwing_user>::value);

    ok = ok && (re_std::is_trivially_move_assignable<trivial>::value      == std::is_trivially_move_assignable<trivial>::value);
    ok = ok && (re_std::is_trivially_move_assignable<nothrow_user>::value == std::is_trivially_move_assignable<nothrow_user>::value);

    return ok;
}

/*
tests_traits_v_forms
  Verifies the _v variable-template aliases mirror the class-template ::value.
  Tests the following:
  - each of the six _v aliases equals its trait's ::value (checked only where
    variable templates are available, C++14+)
*/
bool
tests_traits_v_forms()
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    bool ok = true;

    ok = ok && (re_std::is_move_constructible_v<throwing_user> ==
                re_std::is_move_constructible<throwing_user>::value);
    ok = ok && (re_std::is_move_assignable_v<throwing_user> ==
                re_std::is_move_assignable<throwing_user>::value);
    ok = ok && (re_std::is_nothrow_move_constructible_v<throwing_user> ==
                re_std::is_nothrow_move_constructible<throwing_user>::value);
    ok = ok && (re_std::is_nothrow_move_assignable_v<nothrow_user> ==
                re_std::is_nothrow_move_assignable<nothrow_user>::value);
    ok = ok && (re_std::is_trivially_move_constructible_v<trivial> ==
                re_std::is_trivially_move_constructible<trivial>::value);
    ok = ok && (re_std::is_trivially_move_assignable_v<nothrow_user> ==
                re_std::is_trivially_move_assignable<nothrow_user>::value);

    return ok;
#else
    return true;
#endif
}


NS_END  // testing
NS_END  // djinterp
