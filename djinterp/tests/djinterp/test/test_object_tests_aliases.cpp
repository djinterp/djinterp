// std
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "test_object_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- convenience aliases                          ///
///////////////////////////////////////////////////////////////////////////////

// basic_test is exactly test_object<> and every equivalent spelling of it
static_assert(std::is_same<basic_test, test_object<> >::value, "");
static_assert(std::is_same<basic_test, test_object<std::uint8_t> >::value, "");
static_assert(std::is_same<basic_test,
                  test_object<std::uint8_t, test_metadata> >::value, "");

// tagged_test is exactly the uint8 / vector<int32> instantiation
static_assert(std::is_same<tagged_test,
                  test_object<std::uint8_t, std::vector<std::int32_t> > >::value,
              "");
static_assert(std::is_same<tagged_test::metadata_container_type,
                           std::vector<std::int32_t> >::value, "");
static_assert(std::is_same<tagged_test::metadata_type, std::int32_t>::value, "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- convenience aliases                                    ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_basic_test
  Verifies the default test-object alias.
  Tests the following:
  - it is the same type as test_object<> and its equivalent spellings
  - it exposes uint8 status and the test_metadata key/value container
  - a representative lifecycle works: build, evaluate, name, query
*/
bool
tests_basic_test()
{
    bool ok = true;

    ok = D_TO_CHECK(std::is_same<basic_test, test_object<> >::value) && ok;
    ok = D_TO_CHECK(std::is_same<basic_test::status_type,
                                 std::uint8_t>::value) && ok;
    ok = D_TO_CHECK(std::is_same<basic_test::metadata_container_type,
                                 test_metadata>::value) && ok;

    // representative lifecycle
    basic_test t(42, true);
    t.metadata().set("name", "my test");
    ok = D_TO_CHECK(t.type_id() == 42)                    && ok;
    ok = D_TO_CHECK(t.passed())                           && ok;
    ok = D_TO_CHECK(t.metadata().get("name") == "my test") && ok;

    return ok;
}


/*
tests_tagged_test
  Verifies the integer-tag test-object alias.
  Tests the following:
  - it is the uint8 / vector<int32> instantiation with int32 rows
  - tags can be appended through the metadata accessor and read back
  - the verdict machinery behaves exactly as for basic_test
*/
bool
tests_tagged_test()
{
    bool ok = true;

    ok = D_TO_CHECK(std::is_same<tagged_test,
                    test_object<std::uint8_t,
                                std::vector<std::int32_t> > >::value) && ok;
    ok = D_TO_CHECK(std::is_same<tagged_test::metadata_type,
                                 std::int32_t>::value) && ok;

    tagged_test t(3);
    ok = D_TO_CHECK(t.type_id() == 3)                         && ok;
    ok = D_TO_CHECK(t.status() == tagged_test::status_pending) && ok;
    ok = D_TO_CHECK(t.metadata().size() == 0u)               && ok;

    t.metadata().push_back(100);
    t.metadata().push_back(-5);
    t.evaluate(true);

    ok = D_TO_CHECK(t.metadata().size() == 2u) && ok;
    ok = D_TO_CHECK(t.metadata()[0] == 100)    && ok;
    ok = D_TO_CHECK(t.metadata()[1] == -5)     && ok;
    ok = D_TO_CHECK(t.passed())                && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
