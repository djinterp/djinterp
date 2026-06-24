// std
#include <cstdint>
#include <type_traits>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- read-only query surface                      ///
///////////////////////////////////////////////////////////////////////////////

// operator bool is non-explicit, so an object is implicitly bool-convertible
static_assert(std::is_convertible<basic_test, bool>::value,
              "operator bool must be implicit");

namespace
{
    constexpr lit_object k_pass(1, true);
    constexpr lit_object k_fail(2, false);
    constexpr lit_object k_pend;            // default -> pending
}
static_assert(static_cast<bool>(k_pass) == true,  "");
static_assert(k_pass.result()  == true,           "");
static_assert(k_pass.passed()  == true,           "");
static_assert(k_pass.failed()  == false,          "");
static_assert(static_cast<int>(k_pass.status()) == 0, "");
static_assert(k_pass.type_id() == 1,              "");
static_assert(k_pass.callable_id() == k_no_callable, "");
static_assert(k_pass.has_callable() == false,     "");
static_assert(static_cast<bool>(k_fail) == false, "");
static_assert(k_fail.failed() == true,            "");
static_assert(k_pend.passed() == false,           "");
static_assert(k_pend.failed() == false,           "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- read-only query surface                                ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_object_bool_conversion
  Verifies the boolean conversion operator.
  Tests the following:
  - a passing object is truthy, a failing object falsy
  - it works in if/&&/|| contexts and via implicit copy-initialization
  - it tracks the raw result, NOT the status (a status edited away from
    passed/failed leaves the boolean reflecting the original result)
*/
bool
tests_object_bool_conversion()
{
    bool ok = true;

    basic_test pass(1, true);
    basic_test fail(2, false);

    ok = D_TO_CHECK(static_cast<bool>(pass) == true)  && ok;
    ok = D_TO_CHECK(static_cast<bool>(fail) == false) && ok;

    // usable directly in boolean contexts
    if (pass) { ok = D_TO_CHECK(true)  && ok; } else { ok = D_TO_CHECK(false) && ok; }
    if (fail) { ok = D_TO_CHECK(false) && ok; } else { ok = D_TO_CHECK(true)  && ok; }
    ok = D_TO_CHECK((pass && true))  && ok;
    ok = D_TO_CHECK((fail || true))  && ok;
    ok = D_TO_CHECK(!fail)           && ok;

    // implicit conversion
    bool b = pass;
    ok = D_TO_CHECK(b == true) && ok;

    // tracks m_result, not status
    basic_test t(1, true);
    t.set_status(basic_test::status_failed);
    ok = D_TO_CHECK(static_cast<bool>(t) == true) && ok;  // result still true

    return ok;
}


/*
tests_object_status_result
  Verifies the status() and result() accessors.
  Tests the following:
  - both reflect the values established at construction
  - result() mirrors the boolean result for passing and failing objects
  - status() reports pending for a default-constructed object
*/
bool
tests_object_status_result()
{
    bool ok = true;

    basic_test pass(1, true);
    ok = D_TO_CHECK(pass.result() == true)                      && ok;
    ok = D_TO_CHECK(pass.status() == basic_test::status_passed) && ok;

    basic_test fail(2, false);
    ok = D_TO_CHECK(fail.result() == false)                     && ok;
    ok = D_TO_CHECK(fail.status() == basic_test::status_failed) && ok;

    basic_test def;
    ok = D_TO_CHECK(def.result() == false)                       && ok;
    ok = D_TO_CHECK(def.status() == basic_test::status_pending)  && ok;

    return ok;
}


/*
tests_object_passed_failed
  Verifies the passed() / failed() predicates across every status.
  Tests the following:
  - passed() is true only for the passed status
  - failed() is true only for the failed status
  - skipped, pending, and error report false for BOTH predicates
*/
bool
tests_object_passed_failed()
{
    bool ok = true;

    basic_test t(1, true);  // passed
    ok = D_TO_CHECK(t.passed() == true)  && ok;
    ok = D_TO_CHECK(t.failed() == false) && ok;

    t.evaluate(false);      // failed
    ok = D_TO_CHECK(t.passed() == false) && ok;
    ok = D_TO_CHECK(t.failed() == true)  && ok;

    t.skip();               // skipped
    ok = D_TO_CHECK(t.passed() == false) && ok;
    ok = D_TO_CHECK(t.failed() == false) && ok;

    t.set_status(basic_test::status_pending);   // pending
    ok = D_TO_CHECK(t.passed() == false) && ok;
    ok = D_TO_CHECK(t.failed() == false) && ok;

    t.set_status(basic_test::status_error);     // error
    ok = D_TO_CHECK(t.passed() == false) && ok;
    ok = D_TO_CHECK(t.failed() == false) && ok;

    return ok;
}


/*
tests_object_type_id
  Verifies the type_id() accessor.
  Tests the following:
  - it returns the id supplied at construction
  - the full signed range round-trips (negative, zero, extremes)
*/
bool
tests_object_type_id()
{
    bool ok = true;

    ok = D_TO_CHECK(basic_test(0).type_id()         == 0)         && ok;
    ok = D_TO_CHECK(basic_test(123).type_id()       == 123)       && ok;
    ok = D_TO_CHECK(basic_test(-7).type_id()        == -7)        && ok;
    ok = D_TO_CHECK(basic_test(INT32_MIN).type_id() == INT32_MIN) && ok;
    ok = D_TO_CHECK(basic_test(INT32_MAX).type_id() == INT32_MAX) && ok;

    return ok;
}


/*
tests_object_callable_query
  Verifies callable_id() and has_callable().
  Tests the following:
  - a freshly built object holds the k_no_callable sentinel and reports none
  - after binding a non-zero id the accessors report it and has_callable()
    becomes true (first id, and the maximum id)
  - rebinding to k_no_callable detaches the node again
*/
bool
tests_object_callable_query()
{
    bool ok = true;

    basic_test t(1, true);
    ok = D_TO_CHECK(t.callable_id() == k_no_callable) && ok;
    ok = D_TO_CHECK(t.has_callable() == false)        && ok;

    t.set_callable_id(1);                       // first valid id
    ok = D_TO_CHECK(t.callable_id() == 1u)      && ok;
    ok = D_TO_CHECK(t.has_callable() == true)   && ok;

    t.set_callable_id(UINT32_MAX);              // maximum id
    ok = D_TO_CHECK(t.callable_id() == UINT32_MAX) && ok;
    ok = D_TO_CHECK(t.has_callable() == true)      && ok;

    t.set_callable_id(k_no_callable);           // detach
    ok = D_TO_CHECK(t.callable_id() == k_no_callable) && ok;
    ok = D_TO_CHECK(t.has_callable() == false)        && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
