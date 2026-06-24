// std
#include <cstdint>
#include <type_traits>
// djinterp
#include "test_kind_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- III. contains dispatch selector              ///
///////////////////////////////////////////////////////////////////////////////

// the trait that selects the dispatch: id_set exposes contains(), id_set_nc
// does not -- so the two contains_dispatch overloads are both reachable
static_assert(::djinterp::internal::has_set_contains<id_set>::value,
              "id_set must expose a native contains()");
static_assert(!::djinterp::internal::has_set_contains<id_set_nc>::value,
              "id_set_nc must NOT expose contains()");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- III. contains + dispatch                               ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_set_contains_native
  Verifies contains() over a container that has a native contains().
  Tests the following:
  - this routes through the native-contains dispatch branch
  - a present key returns true, an absent key returns false
*/
bool
tests_set_contains_native()
{
    bool ok = true;

    test_kind_set<id_set> ks;       // id_set HAS contains() -> true_type branch
    ks.insert(1);
    ks.insert(2);

    ok = D_TK_CHECK(ks.contains(1))      && ok;
    ok = D_TK_CHECK(ks.contains(2))      && ok;
    ok = D_TK_CHECK(!ks.contains(3))     && ok;
    ok = D_TK_CHECK(!ks.contains(-1))    && ok;

    // tracks erasure
    ks.erase(1);
    ok = D_TK_CHECK(!ks.contains(1))     && ok;

    return ok;
}


/*
tests_set_contains_fallback
  Verifies contains() over a container that lacks a native contains().
  Tests the following:
  - this routes through the find()-based fallback dispatch branch
  - a present key returns true, an absent key returns false
  - the result matches find() != end() on the same wrapper
*/
bool
tests_set_contains_fallback()
{
    bool ok = true;

    test_kind_set<id_set_nc> ks;    // no contains() -> false_type branch
    ks.insert(10);
    ks.insert(20);

    ok = D_TK_CHECK(ks.contains(10))   && ok;
    ok = D_TK_CHECK(ks.contains(20))   && ok;
    ok = D_TK_CHECK(!ks.contains(30))  && ok;

    // agrees with the find()-based definition it falls back to
    ok = D_TK_CHECK(ks.contains(10) == (ks.find(10) != ks.end())) && ok;
    ok = D_TK_CHECK(ks.contains(30) == (ks.find(30) != ks.end())) && ok;

    ks.erase(10);
    ok = D_TK_CHECK(!ks.contains(10))  && ok;

    return ok;
}


/*
tests_set_contains_detection
  Verifies the has_set_contains trait that selects the dispatch.
  Tests the following:
  - it is true for the contains-bearing container and false for the other
*/
bool
tests_set_contains_detection()
{
    bool ok = true;

    ok = D_TK_CHECK(::djinterp::internal::has_set_contains<id_set>::value)
         && ok;
    ok = D_TK_CHECK(!::djinterp::internal::has_set_contains<id_set_nc>::value)
         && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
