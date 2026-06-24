// std
#include <cstdint>
#include <string>
#include <type_traits>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- factory functions                           ///
///////////////////////////////////////////////////////////////////////////////

// make_test and the unnamed make_interior are noexcept; the named overload is
// not (storing a metadata entry may allocate)
static_assert(noexcept(make_test(test_type_id(0), true)),
              "make_test must be noexcept");
static_assert(noexcept(make_interior(test_type_id(0))),
              "unnamed make_interior must be noexcept");
static_assert(!noexcept(make_interior(test_type_id(0), "x")),
              "named make_interior must not be noexcept");

// all three yield a basic_test
static_assert(std::is_same<decltype(make_test(test_type_id(0), true)),
                           basic_test>::value, "");
static_assert(std::is_same<decltype(make_interior(test_type_id(0))),
                           basic_test>::value, "");
static_assert(std::is_same<
                  decltype(make_interior(test_type_id(0), "x")),
                  basic_test>::value, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- factory functions                                      ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_make_test
  Verifies make_test().
  Tests the following:
  - the returned object carries the supplied type id and result
  - a true result is passed, a false result is failed
  - no deferred callable is attached and the metadata starts empty
  - the full signed type-id range is accepted
*/
bool
tests_make_test()
{
    bool ok = true;

    basic_test pass = make_test(5, true);
    ok = D_TO_CHECK(pass.type_id() == 5)                && ok;
    ok = D_TO_CHECK(pass.result() == true)              && ok;
    ok = D_TO_CHECK(pass.passed())                      && ok;
    ok = D_TO_CHECK(pass.callable_id() == k_no_callable) && ok;
    ok = D_TO_CHECK(pass.metadata().size() == 0u)       && ok;

    basic_test fail = make_test(6, false);
    ok = D_TO_CHECK(fail.type_id() == 6) && ok;
    ok = D_TO_CHECK(fail.failed())       && ok;

    // extremes
    ok = D_TO_CHECK(make_test(INT32_MIN, true).type_id()  == INT32_MIN) && ok;
    ok = D_TO_CHECK(make_test(INT32_MAX, false).type_id() == INT32_MAX) && ok;

    return ok;
}


/*
tests_make_interior
  Verifies the unnamed make_interior().
  Tests the following:
  - the returned object is pending with the supplied type id
  - its result is false and no deferred callable is attached
  - the metadata container starts empty
*/
bool
tests_make_interior()
{
    bool ok = true;

    basic_test t = make_interior(7);
    ok = D_TO_CHECK(t.type_id() == 7)                         && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_pending) && ok;
    ok = D_TO_CHECK(t.result() == false)                      && ok;
    ok = D_TO_CHECK(t.passed() == false)                      && ok;
    ok = D_TO_CHECK(t.failed() == false)                      && ok;
    ok = D_TO_CHECK(t.callable_id() == k_no_callable)         && ok;
    ok = D_TO_CHECK(t.metadata().size() == 0u)                && ok;

    return ok;
}


/*
tests_make_interior_named
  Verifies the named make_interior().
  Tests the following:
  - a non-null name is stored under the "name" metadata key (the store branch)
  - a null name leaves the metadata empty (the skip branch)
  - an empty-string name is still non-null, so it is stored (and reads back "")
  - the object is otherwise a pending interior of the supplied type
*/
bool
tests_make_interior_named()
{
    bool ok = true;

    // non-null name -> stored
    {
        basic_test t = make_interior(8, "node");
        ok = D_TO_CHECK(t.type_id() == 8)                         && ok;
        ok = D_TO_CHECK(t.status() == basic_test::status_pending) && ok;
        ok = D_TO_CHECK(t.metadata().size() == 1u)               && ok;
        ok = D_TO_CHECK(t.metadata().get("name") == "node")      && ok;
    }

    // null name -> nothing stored (skip branch)
    {
        basic_test t = make_interior(9, nullptr);
        ok = D_TO_CHECK(t.type_id() == 9)                         && ok;
        ok = D_TO_CHECK(t.status() == basic_test::status_pending) && ok;
        ok = D_TO_CHECK(t.metadata().size() == 0u)               && ok;
        ok = D_TO_CHECK(t.metadata().contains("name") == false)  && ok;
    }

    // empty-string name is non-null -> stored, reads back empty
    {
        basic_test t = make_interior(10, "");
        ok = D_TO_CHECK(t.metadata().contains("name"))    && ok;
        ok = D_TO_CHECK(t.metadata().get("name") == "")   && ok;
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
