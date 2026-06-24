// std
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- construction                                 ///
///////////////////////////////////////////////////////////////////////////////

// -- constexpr construction (literal container) -------------------------------
namespace
{
    constexpr lit_object k_def;
    constexpr lit_object k_typed(11);
    constexpr lit_object k_passed(12, true);
    constexpr lit_object k_failed(13, false);
}
static_assert(k_def.result()    == false, "");
static_assert(k_def.type_id()   == 0,     "");
static_assert(k_def.callable_id() == k_no_callable, "");
static_assert(k_typed.type_id() == 11,    "");
static_assert(k_passed.result() == true,  "");
static_assert(k_passed.passed() == true,  "");
static_assert(k_failed.result() == false, "");
static_assert(k_failed.failed() == true,  "");

// -- noexcept matrix ----------------------------------------------------------
// default / type_id / result: noexcept iff the container's default ctor is
static_assert(std::is_nothrow_default_constructible<basic_test>::value, "");
static_assert(std::is_nothrow_constructible<basic_test, test_type_id>::value,
              "");
static_assert(std::is_nothrow_constructible<basic_test,
                  test_type_id, bool>::value, "");
static_assert(!std::is_nothrow_default_constructible<throw_object>::value, "");
static_assert(!std::is_nothrow_constructible<throw_object, test_type_id>::value,
              "");
static_assert(!std::is_nothrow_constructible<throw_object,
                  test_type_id, bool>::value, "");
// metadata copy ctor: never noexcept (it may allocate)
static_assert(!std::is_nothrow_constructible<basic_test,
                  test_type_id, bool, const test_metadata&>::value, "");
// metadata move ctor: noexcept iff the container's move ctor is
static_assert(std::is_nothrow_constructible<basic_test,
                  test_type_id, bool, test_metadata&&>::value, "");
static_assert(!std::is_nothrow_constructible<throw_object,
                  test_type_id, bool, throwmeta&&>::value, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- construction                                           ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_object_ctor_default
  Verifies the default constructor.
  Tests the following:
  - result is false and status is pending
  - type id is 0 and no deferred callable is attached
  - the metadata container is default-constructed (empty)
*/
bool
tests_object_ctor_default()
{
    bool ok = true;

    basic_test t;

    ok = D_TO_CHECK(t.result() == false)                       && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_pending)  && ok;
    ok = D_TO_CHECK(t.type_id() == 0)                          && ok;
    ok = D_TO_CHECK(t.callable_id() == k_no_callable)          && ok;
    ok = D_TO_CHECK(t.has_callable() == false)                 && ok;
    ok = D_TO_CHECK(t.metadata().size() == 0u)                 && ok;

    return ok;
}


/*
tests_object_ctor_type_id
  Verifies the explicit type-id constructor.
  Tests the following:
  - the supplied type id is stored; the rest defaults (pending, no callable)
  - the full signed range of type ids is accepted (negative, zero, extremes)
*/
bool
tests_object_ctor_type_id()
{
    bool ok = true;

    basic_test t(42);
    ok = D_TO_CHECK(t.type_id() == 42)                         && ok;
    ok = D_TO_CHECK(t.result() == false)                       && ok;
    ok = D_TO_CHECK(t.status() == basic_test::status_pending)  && ok;
    ok = D_TO_CHECK(t.callable_id() == k_no_callable)          && ok;
    ok = D_TO_CHECK(t.metadata().size() == 0u)                 && ok;

    // full signed-rank range
    basic_test lo(INT32_MIN);
    basic_test zero(0);
    basic_test hi(INT32_MAX);
    ok = D_TO_CHECK(lo.type_id()   == INT32_MIN) && ok;
    ok = D_TO_CHECK(zero.type_id() == 0)         && ok;
    ok = D_TO_CHECK(hi.type_id()   == INT32_MAX) && ok;

    return ok;
}


/*
tests_object_ctor_type_result
  Verifies the type-id + result constructor.
  Tests the following:
  - a true result yields passed; a false result yields failed
  - the type id is stored and no deferred callable is attached
*/
bool
tests_object_ctor_type_result()
{
    bool ok = true;

    basic_test pass(1, true);
    ok = D_TO_CHECK(pass.result() == true)                      && ok;
    ok = D_TO_CHECK(pass.status() == basic_test::status_passed) && ok;
    ok = D_TO_CHECK(pass.passed())                              && ok;
    ok = D_TO_CHECK(pass.type_id() == 1)                        && ok;
    ok = D_TO_CHECK(pass.callable_id() == k_no_callable)        && ok;

    basic_test fail(2, false);
    ok = D_TO_CHECK(fail.result() == false)                     && ok;
    ok = D_TO_CHECK(fail.status() == basic_test::status_failed) && ok;
    ok = D_TO_CHECK(fail.failed())                              && ok;
    ok = D_TO_CHECK(fail.type_id() == 2)                        && ok;

    return ok;
}


/*
tests_object_ctor_metadata
  Verifies the metadata-carrying constructors (copy and move).
  Tests the following:
  - the copy form duplicates the supplied container and leaves the source intact
  - the move form transfers the supplied container's contents into the object
  - result/status/type are still derived as in the simpler constructors
  - the same works for a vector-backed (tagged) container
*/
bool
tests_object_ctor_metadata()
{
    bool ok = true;

    // copy form: source survives unchanged
    {
        test_metadata m;
        m.set("name", "alpha");
        m.set("message", "hi");

        basic_test t(7, true, m);

        ok = D_TO_CHECK(t.type_id() == 7)                      && ok;
        ok = D_TO_CHECK(t.passed())                            && ok;
        ok = D_TO_CHECK(t.metadata().get("name") == "alpha")   && ok;
        ok = D_TO_CHECK(t.metadata().get("message") == "hi")   && ok;
        // the source is a copy source, so it must be untouched
        ok = D_TO_CHECK(m.size() == 2u)                        && ok;
        ok = D_TO_CHECK(m.get("name") == "alpha")              && ok;
    }

    // move form: contents land in the object
    {
        test_metadata m;
        m.set("k", "v");

        basic_test t(8, false, std::move(m));

        ok = D_TO_CHECK(t.type_id() == 8)                && ok;
        ok = D_TO_CHECK(t.failed())                      && ok;
        ok = D_TO_CHECK(t.metadata().get("k") == "v")    && ok;
        ok = D_TO_CHECK(t.metadata().size() == 1u)       && ok;
    }

    // vector-backed container, copy form
    {
        std::vector<std::int32_t> tags;
        tags.push_back(10);
        tags.push_back(20);

        tagged_test t(3, true, tags);

        ok = D_TO_CHECK(t.metadata().size() == 2u) && ok;
        ok = D_TO_CHECK(t.metadata()[0] == 10)     && ok;
        ok = D_TO_CHECK(t.metadata()[1] == 20)     && ok;
        ok = D_TO_CHECK(tags.size() == 2u)         && ok;  // source intact
    }

    return ok;
}


/*
tests_object_ctor_noexcept
  Verifies that the constructors' exception specifications track the metadata
  container.
  Tests the following:
  - default / type_id / result ctors are noexcept for a noexcept-default
    container and throwing for a throwing one
  - the metadata copy ctor is never noexcept
  - the metadata move ctor is noexcept iff the container's move ctor is
*/
bool
tests_object_ctor_noexcept()
{
    bool ok = true;

    ok = D_TO_CHECK(std::is_nothrow_default_constructible<basic_test>::value)
         && ok;
    ok = D_TO_CHECK(std::is_nothrow_constructible<basic_test,
                        test_type_id>::value) && ok;
    ok = D_TO_CHECK(std::is_nothrow_constructible<basic_test,
                        test_type_id, bool>::value) && ok;

    ok = D_TO_CHECK(!std::is_nothrow_default_constructible<throw_object>::value)
         && ok;
    ok = D_TO_CHECK(!std::is_nothrow_constructible<throw_object,
                        test_type_id, bool>::value) && ok;

    ok = D_TO_CHECK(!std::is_nothrow_constructible<basic_test,
                        test_type_id, bool, const test_metadata&>::value) && ok;

    ok = D_TO_CHECK(std::is_nothrow_constructible<basic_test,
                        test_type_id, bool, test_metadata&&>::value) && ok;
    ok = D_TO_CHECK(!std::is_nothrow_constructible<throw_object,
                        test_type_id, bool, throwmeta&&>::value) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
