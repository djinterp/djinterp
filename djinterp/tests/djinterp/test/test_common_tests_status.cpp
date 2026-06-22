// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "test_common_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- II. STATUS CLASSIFICATION                    ///
///////////////////////////////////////////////////////////////////////////////

static_assert(std::is_enum<test_status>::value,
              "test_status must be an enumeration");
static_assert(!std::is_convertible<test_status, int>::value,
              "test_status must be scoped (no implicit conversion to int)");
static_assert(std::is_same<std::underlying_type<test_status>::type, int>::value,
              "test_status underlying type must default to int");
static_assert(sizeof(test_status) == sizeof(int),
              "test_status must be the size of its int underlying type");

// enumerator values are fixed by the public contract
static_assert(static_cast<int>(test_status::passed)  == 0, "passed  == 0");
static_assert(static_cast<int>(test_status::failed)  == 1, "failed  == 1");
static_assert(static_cast<int>(test_status::skipped) == 2, "skipped == 2");
static_assert(static_cast<int>(test_status::pending) == 3, "pending == 3");
static_assert(static_cast<int>(test_status::error)   == 4, "error   == 4");

// value-initialization yields the zero enumerator (passed)
static_assert(static_cast<int>(test_status{}) == 0,
              "value-initialized test_status must be passed (0)");

// declaration order is a strict ascending ranking
static_assert(test_status::passed  < test_status::failed,  "");
static_assert(test_status::failed  < test_status::skipped, "");
static_assert(test_status::skipped < test_status::pending, "");
static_assert(test_status::pending < test_status::error,   "");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- II. STATUS CLASSIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

namespace
{
    // classify
    //   helper: maps every named status through a switch so the enumerators
    // are exercised as case labels; returns -1 for any unnamed value.
    int
    classify(
        test_status _status
    )
    {
        switch (_status)
        {
            case test_status::passed:  return 100;
            case test_status::failed:  return 101;
            case test_status::skipped: return 102;
            case test_status::pending: return 103;
            case test_status::error:   return 104;
        }

        return -1;
    }
}  // namespace


/*
tests_test_status
  Verifies the scoped status enumeration.
  Tests the following:
  - enum / scoped classification, int underlying type, and size
  - each named enumerator carries its contracted integer value
  - all five enumerators are mutually distinct
  - declaration order forms a strict ascending ranking
  - bidirectional round-trip between status and its underlying integer
  - value-initialization produces passed
  - an unnamed in-range value equals none of the named enumerators
  - copy and assignment preserve and isolate values
  - enumerators are usable as switch case labels
*/
bool
tests_test_status()
{
    bool ok = true;

    const test_status all[] = {
        test_status::passed,
        test_status::failed,
        test_status::skipped,
        test_status::pending,
        test_status::error
    };
    const std::size_t count = sizeof(all) / sizeof(all[0]);

    // classification
    ok = D_TC_CHECK(std::is_enum<test_status>::value)              && ok;
    ok = D_TC_CHECK(!std::is_convertible<test_status, int>::value) && ok;
    ok = D_TC_CHECK(std::is_same<std::underlying_type<test_status>::type,
                                 int>::value)                      && ok;
    ok = D_TC_CHECK(sizeof(test_status) == sizeof(int))           && ok;

    // contracted enumerator values
    ok = D_TC_CHECK(static_cast<int>(test_status::passed)  == 0) && ok;
    ok = D_TC_CHECK(static_cast<int>(test_status::failed)  == 1) && ok;
    ok = D_TC_CHECK(static_cast<int>(test_status::skipped) == 2) && ok;
    ok = D_TC_CHECK(static_cast<int>(test_status::pending) == 3) && ok;
    ok = D_TC_CHECK(static_cast<int>(test_status::error)   == 4) && ok;

    // mutual distinctness (all C(5,2) pairs)
    for (std::size_t i = 0; i < count; ++i)
    {
        for (std::size_t j = i + 1; j < count; ++j)
        {
            ok = D_TC_CHECK(all[i] != all[j]) && ok;
        }
    }

    // strict ascending ranking, by both the operator and the underlying value
    for (std::size_t i = 0; i + 1 < count; ++i)
    {
        ok = D_TC_CHECK(all[i] < all[i + 1]) && ok;
        ok = D_TC_CHECK(static_cast<int>(all[i])
                            < static_cast<int>(all[i + 1])) && ok;
    }

    // bidirectional round-trip: status -> int -> status, for every value
    for (std::size_t i = 0; i < count; ++i)
    {
        int as_int = static_cast<int>(all[i]);
        ok = D_TC_CHECK(static_cast<test_status>(as_int) == all[i]) && ok;
    }
    for (int v = 0; v <= 4; ++v)
    {
        ok = D_TC_CHECK(static_cast<int>(static_cast<test_status>(v)) == v)
             && ok;
    }

    // named round-trip from the integer side
    ok = D_TC_CHECK(static_cast<test_status>(0) == test_status::passed)  && ok;
    ok = D_TC_CHECK(static_cast<test_status>(1) == test_status::failed)  && ok;
    ok = D_TC_CHECK(static_cast<test_status>(2) == test_status::skipped) && ok;
    ok = D_TC_CHECK(static_cast<test_status>(3) == test_status::pending) && ok;
    ok = D_TC_CHECK(static_cast<test_status>(4) == test_status::error)   && ok;

    // value-initialization -> passed
    {
        test_status def{};
        ok = D_TC_CHECK(def == test_status::passed)    && ok;
        ok = D_TC_CHECK(static_cast<int>(def) == 0)    && ok;
    }

    // an unnamed but in-range value matches none of the named enumerators
    {
        test_status unnamed = static_cast<test_status>(99);
        for (std::size_t i = 0; i < count; ++i)
        {
            ok = D_TC_CHECK(unnamed != all[i]) && ok;
        }
    }

    // copy & assignment preserve the source and isolate the destination
    {
        test_status source = test_status::skipped;
        test_status copy   = source;
        ok = D_TC_CHECK(copy == test_status::skipped) && ok;

        copy = test_status::error;
        ok = D_TC_CHECK(copy   == test_status::error)   && ok;
        ok = D_TC_CHECK(source == test_status::skipped) && ok;
    }

    // enumerators function as switch case labels
    ok = D_TC_CHECK(classify(test_status::passed)  == 100) && ok;
    ok = D_TC_CHECK(classify(test_status::failed)  == 101) && ok;
    ok = D_TC_CHECK(classify(test_status::skipped) == 102) && ok;
    ok = D_TC_CHECK(classify(test_status::pending) == 103) && ok;
    ok = D_TC_CHECK(classify(test_status::error)   == 104) && ok;

    return ok;
}


NS_END  // testing
NS_END  // djinterp
