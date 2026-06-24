// std
#include <cstdint>
#include <type_traits>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- mutation (C++14+ relaxed constexpr)          ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
namespace
{
    constexpr lit_object eval_true_obj()
    {
        lit_object t(1);
        t.evaluate(true);
        return t;
    }
    constexpr lit_object eval_false_obj()
    {
        lit_object t(1);
        t.evaluate(false);
        return t;
    }
    constexpr lit_object skipped_obj()
    {
        lit_object t(1);
        t.evaluate(true);
        t.skip();
        return t;
    }
    constexpr lit_object retyped_obj()
    {
        lit_object t(1);
        t.set_type_id(99);
        return t;
    }
    constexpr lit_object bound_obj()
    {
        lit_object t(1);
        t.set_callable_id(5);
        return t;
    }
    constexpr lit_object statused_obj()
    {
        lit_object t(1);
        t.set_status(lit_object::status_error);
        return t;
    }
}
static_assert(eval_true_obj().passed(),                            "");
static_assert(eval_false_obj().failed(),                           "");
static_assert(skipped_obj().result() == false,                     "");
static_assert(static_cast<int>(skipped_obj().status())  == 2,      "");
static_assert(retyped_obj().type_id() == 99,                       "");
static_assert(bound_obj().callable_id() == 5u,                     "");
static_assert(bound_obj().has_callable(),                          "");
static_assert(static_cast<int>(statused_obj().status()) == 4,      "");
#endif


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- mutation                                               ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_object_evaluate
  Verifies evaluate().
  Tests the following:
  - a true result sets result true and status passed
  - a false result sets result false and status failed
  - repeated calls flip the verdict each time
  - evaluate overrides any prior non-pass/fail status (e.g. pending, skipped)
*/
bool
tests_object_evaluate()
{
    bool ok = true;

    basic_test t;                 // pending, result false

    t.evaluate(true);
    ok = D_TO_CHECK(t.result() == true)                      && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_passed) && ok;
    ok = D_TO_CHECK(t.passed())                              && ok;

    t.evaluate(false);
    ok = D_TO_CHECK(t.result() == false)                     && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_failed) && ok;
    ok = D_TO_CHECK(t.failed())                              && ok;

    // flips back
    t.evaluate(true);
    ok = D_TO_CHECK(t.passed()) && ok;

    // overrides a skipped state
    t.skip();
    t.evaluate(true);
    ok = D_TO_CHECK(t.result() == true) && ok;
    ok = D_TO_CHECK(t.passed())         && ok;

    return ok;
}


/*
tests_object_skip
  Verifies skip().
  Tests the following:
  - status becomes skipped and result is forced false
  - neither passed() nor failed() holds afterwards
  - skipping a previously-passing object clears its result
*/
bool
tests_object_skip()
{
    bool ok = true;

    basic_test t;
    t.skip();
    ok = D_TO_CHECK(t.result() == false)                       && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_skipped)  && ok;
    ok = D_TO_CHECK(t.passed() == false)                       && ok;
    ok = D_TO_CHECK(t.failed() == false)                       && ok;

    // forces result false even when it was true
    basic_test was_true(1, true);
    ok = D_TO_CHECK(was_true.result() == true) && ok;
    was_true.skip();
    ok = D_TO_CHECK(was_true.result() == false)                      && ok;
    ok = D_TO_CHECK(was_true.status() == basic_test::status_skipped) && ok;

    return ok;
}


/*
tests_object_set_status
  Verifies set_status().
  Tests the following:
  - the status is replaced by the supplied code
  - the raw result is left untouched (unlike skip(), which clears it)
  - every one of the five status codes can be set
*/
bool
tests_object_set_status()
{
    bool ok = true;

    // result is preserved across a status change
    basic_test t(1, true);          // result true, status passed
    t.set_status(basic_test::status_error);
    ok = D_TO_CHECK(t.status() == basic_test::status_error) && ok;
    ok = D_TO_CHECK(t.result() == true) && ok;   // result untouched

    // each code can be set
    basic_test s;
    s.set_status(basic_test::status_passed);
    ok = D_TO_CHECK(s.status() == basic_test::status_passed)  && ok;
    s.set_status(basic_test::status_failed);
    ok = D_TO_CHECK(s.status() == basic_test::status_failed)  && ok;
    s.set_status(basic_test::status_skipped);
    ok = D_TO_CHECK(s.status() == basic_test::status_skipped) && ok;
    s.set_status(basic_test::status_pending);
    ok = D_TO_CHECK(s.status() == basic_test::status_pending) && ok;
    s.set_status(basic_test::status_error);
    ok = D_TO_CHECK(s.status() == basic_test::status_error)   && ok;
    // result never changed throughout (default false)
    ok = D_TO_CHECK(s.result() == false) && ok;

    return ok;
}


/*
tests_object_set_type_id
  Verifies set_type_id().
  Tests the following:
  - the type id is replaced, overriding any constructor value
  - the full signed range can be set
  - result, status, and callable id are left untouched
*/
bool
tests_object_set_type_id()
{
    bool ok = true;

    basic_test t(7, true);
    t.set_callable_id(3);

    t.set_type_id(123);
    ok = D_TO_CHECK(t.type_id() == 123) && ok;
    // orthogonality: other fields unchanged
    ok = D_TO_CHECK(t.result() == true)                       && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_passed)  && ok;
    ok = D_TO_CHECK(t.callable_id() == 3u)                    && ok;

    // full range
    t.set_type_id(INT32_MIN);
    ok = D_TO_CHECK(t.type_id() == INT32_MIN) && ok;
    t.set_type_id(0);
    ok = D_TO_CHECK(t.type_id() == 0)         && ok;
    t.set_type_id(INT32_MAX);
    ok = D_TO_CHECK(t.type_id() == INT32_MAX) && ok;

    return ok;
}


/*
tests_object_set_callable_id
  Verifies set_callable_id().
  Tests the following:
  - the id is replaced and has_callable() updates accordingly
  - binding the maximum id works; binding k_no_callable detaches
  - result, status, and type id are left untouched
*/
bool
tests_object_set_callable_id()
{
    bool ok = true;

    basic_test t(9, false);

    t.set_callable_id(2);
    ok = D_TO_CHECK(t.callable_id() == 2u)  && ok;
    ok = D_TO_CHECK(t.has_callable())       && ok;
    // orthogonality
    ok = D_TO_CHECK(t.result() == false)                      && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_failed)  && ok;
    ok = D_TO_CHECK(t.type_id() == 9)                         && ok;

    t.set_callable_id(UINT32_MAX);
    ok = D_TO_CHECK(t.callable_id() == UINT32_MAX) && ok;
    ok = D_TO_CHECK(t.has_callable())              && ok;

    t.set_callable_id(k_no_callable);
    ok = D_TO_CHECK(t.callable_id() == k_no_callable) && ok;
    ok = D_TO_CHECK(t.has_callable() == false)        && ok;

    return ok;
}


/*
tests_object_mutation_sequence
  Verifies combined mutation sequences and field orthogonality.
  Tests the following:
  - independent setters compose without interfering with one another
  - evaluate after binding leaves the binding intact
  - a full lifecycle (build -> classify -> bind -> evaluate) lands correctly
*/
bool
tests_object_mutation_sequence()
{
    bool ok = true;

    basic_test t;                       // pending / false / type 0 / no callable

    t.set_type_id(5);
    t.set_callable_id(3);
    t.evaluate(true);

    ok = D_TO_CHECK(t.type_id() == 5)      && ok;
    ok = D_TO_CHECK(t.callable_id() == 3u) && ok;  // binding survives evaluate
    ok = D_TO_CHECK(t.has_callable())      && ok;
    ok = D_TO_CHECK(t.result() == true)    && ok;
    ok = D_TO_CHECK(t.passed())            && ok;

    // evaluate must not disturb type or callable id
    t.evaluate(false);
    ok = D_TO_CHECK(t.type_id() == 5)      && ok;
    ok = D_TO_CHECK(t.callable_id() == 3u) && ok;
    ok = D_TO_CHECK(t.failed())            && ok;

    // skip then re-evaluate
    t.skip();
    ok = D_TO_CHECK(t.status() == basic_test::status_skipped) && ok;
    ok = D_TO_CHECK(t.type_id() == 5)      && ok;   // still untouched
    t.evaluate(true);
    ok = D_TO_CHECK(t.passed())            && ok;
    ok = D_TO_CHECK(t.callable_id() == 3u) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
