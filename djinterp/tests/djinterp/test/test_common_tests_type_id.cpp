// std
#include <cstdint>
#include <climits>
#include <limits>
#include <type_traits>
// djinterp
#include "test_common_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///   compile-time invariants -- I. TEST TYPE IDENTIFICATION                  ///
///////////////////////////////////////////////////////////////////////////////

// -- test_type_id : exactly std::int32_t --------------------------------------
static_assert(std::is_same<test_type_id, std::int32_t>::value,
              "test_type_id must be std::int32_t");
static_assert(std::is_integral<test_type_id>::value,
              "test_type_id must be integral");
static_assert(std::is_signed<test_type_id>::value,
              "test_type_id must be signed");
static_assert(sizeof(test_type_id) == 4,
              "test_type_id must occupy 4 bytes");
static_assert(CHAR_BIT * sizeof(test_type_id) == 32,
              "test_type_id must be exactly 32 bits wide");
static_assert(std::numeric_limits<test_type_id>::is_signed,
              "test_type_id must be a signed arithmetic type");
static_assert(std::numeric_limits<test_type_id>::digits == 31,
              "test_type_id must expose 31 value bits (+ sign)");
static_assert(std::numeric_limits<test_type_id>::min() == INT32_MIN,
              "test_type_id min must equal INT32_MIN");
static_assert(std::numeric_limits<test_type_id>::max() == INT32_MAX,
              "test_type_id max must equal INT32_MAX");

// -- test_callable_id : exactly std::uint32_t ---------------------------------
static_assert(std::is_same<test_callable_id, std::uint32_t>::value,
              "test_callable_id must be std::uint32_t");
static_assert(std::is_integral<test_callable_id>::value,
              "test_callable_id must be integral");
static_assert(std::is_unsigned<test_callable_id>::value,
              "test_callable_id must be unsigned");
static_assert(sizeof(test_callable_id) == 4,
              "test_callable_id must occupy 4 bytes");
static_assert(CHAR_BIT * sizeof(test_callable_id) == 32,
              "test_callable_id must be exactly 32 bits wide");
static_assert(std::numeric_limits<test_callable_id>::digits == 32,
              "test_callable_id must expose 32 value bits");
static_assert(std::numeric_limits<test_callable_id>::max() == UINT32_MAX,
              "test_callable_id max must equal UINT32_MAX");

// -- k_no_callable : constexpr zero sentinel of type test_callable_id ---------
static_assert(k_no_callable == 0,
              "k_no_callable must equal 0");
static_assert(std::is_same<std::remove_cv<decltype(k_no_callable)>::type,
                           test_callable_id>::value,
              "k_no_callable must be of type test_callable_id");
// usable as a non-type template argument -> proves it is a constant expression
static_assert(std::integral_constant<test_callable_id, k_no_callable>::value
                  == 0,
              "k_no_callable must be a compile-time constant");


///////////////////////////////////////////////////////////////////////////////
///   runtime tests -- I. TEST TYPE IDENTIFICATION                            ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_test_type_id
  Verifies the structural-identity id type.
  Tests the following:
  - exact identity with std::int32_t
  - integral / signed classification and 32-bit width
  - numeric range matches INT32_MIN / INT32_MAX
  - negative, zero, and positive values round-trip intact
  - the rank ordering relied on by isolated nodes (child <= parent) holds
    across the extremes of the range
*/
bool
tests_test_type_id()
{
    bool ok = true;

    // identity & classification
    ok = D_TC_CHECK(std::is_same<test_type_id, std::int32_t>::value) && ok;
    ok = D_TC_CHECK(std::is_integral<test_type_id>::value)           && ok;
    ok = D_TC_CHECK(std::is_signed<test_type_id>::value)             && ok;
    ok = D_TC_CHECK(sizeof(test_type_id) == 4)                       && ok;
    ok = D_TC_CHECK(CHAR_BIT * sizeof(test_type_id) == 32)           && ok;

    // numeric range
    ok = D_TC_CHECK(std::numeric_limits<test_type_id>::min() == INT32_MIN)
         && ok;
    ok = D_TC_CHECK(std::numeric_limits<test_type_id>::max() == INT32_MAX)
         && ok;

    // value round-trips across the sign boundary and the extremes
    {
        test_type_id zero = 0;
        test_type_id neg  = -42;
        test_type_id pos  = 42;
        test_type_id lo   = INT32_MIN;
        test_type_id hi   = INT32_MAX;

        ok = D_TC_CHECK(zero == 0)          && ok;
        ok = D_TC_CHECK(neg  == -42)        && ok;
        ok = D_TC_CHECK(pos  == 42)         && ok;
        ok = D_TC_CHECK(lo   == INT32_MIN)  && ok;
        ok = D_TC_CHECK(hi   == INT32_MAX)  && ok;
        ok = D_TC_CHECK(neg  <  zero)       && ok;
        ok = D_TC_CHECK(zero <  pos)        && ok;
    }

    // rank ordering used when no test_kind registry is attached:
    // a child's id must be <= its parent's id.
    {
        test_type_id child  = -1;
        test_type_id parent = 0;

        ok = D_TC_CHECK(child <= parent)            && ok;  // strictly lower
        ok = D_TC_CHECK(parent <= parent)           && ok;  // equal ranks ok
        ok = D_TC_CHECK(test_type_id(INT32_MIN)
                            <= test_type_id(INT32_MAX)) && ok;  // extremes
    }

    return ok;
}


/*
tests_test_callable_id
  Verifies the opaque callable-handle type.
  Tests the following:
  - exact identity with std::uint32_t
  - integral / unsigned classification and 32-bit width
  - capacity covers the documented ~4 billion distinct handles
  - representative handle values round-trip intact
  - unsigned wrap-around at the maximum is well defined (max + 1 == 0)
*/
bool
tests_test_callable_id()
{
    bool ok = true;

    // identity & classification
    ok = D_TC_CHECK(std::is_same<test_callable_id, std::uint32_t>::value) && ok;
    ok = D_TC_CHECK(std::is_integral<test_callable_id>::value)            && ok;
    ok = D_TC_CHECK(std::is_unsigned<test_callable_id>::value)            && ok;
    ok = D_TC_CHECK(sizeof(test_callable_id) == 4)                        && ok;
    ok = D_TC_CHECK(CHAR_BIT * sizeof(test_callable_id) == 32)            && ok;

    // capacity: documented as "up to ~4 billion distinct callables"
    ok = D_TC_CHECK(std::numeric_limits<test_callable_id>::max() == UINT32_MAX)
         && ok;
    ok = D_TC_CHECK(std::numeric_limits<test_callable_id>::max()
                        >= 4000000000u) && ok;

    // representative values round-trip
    {
        test_callable_id zero = 0;
        test_callable_id one  = 1;
        test_callable_id big  = UINT32_MAX;

        ok = D_TC_CHECK(zero == 0u)          && ok;
        ok = D_TC_CHECK(one  == 1u)          && ok;
        ok = D_TC_CHECK(big  == UINT32_MAX)  && ok;
        ok = D_TC_CHECK(zero <  one)         && ok;
        ok = D_TC_CHECK(one  <  big)         && ok;
    }

    // unsigned arithmetic wraps modulo 2^32 (well-defined, not UB)
    {
        test_callable_id big = UINT32_MAX;

        ok = D_TC_CHECK(static_cast<test_callable_id>(big + 1u) == 0u) && ok;
    }

    return ok;
}


/*
tests_k_no_callable
  Verifies the reserved "no deferred callable" sentinel.
  Tests the following:
  - the constant equals 0 and is typed as test_callable_id
  - it is a genuine compile-time constant (usable where one is required)
  - it is interchangeable with a freshly zero-cast handle
  - it is distinct from the first valid (non-zero) handle value
*/
bool
tests_k_no_callable()
{
    bool ok = true;

    // value & type
    ok = D_TC_CHECK(k_no_callable == 0u)                                && ok;
    ok = D_TC_CHECK(k_no_callable == static_cast<test_callable_id>(0))  && ok;
    ok = D_TC_CHECK(std::is_same<std::remove_cv<decltype(k_no_callable)>::type,
                                 test_callable_id>::value)              && ok;

    // compile-time constant: a local constexpr must accept it
    {
        D_CONSTEXPR test_callable_id mirror = k_no_callable;
        ok = D_TC_CHECK(mirror == 0u) && ok;
    }

    // sentinel is distinct from the first real handle (id 1)
    {
        test_callable_id first_real = k_no_callable + 1u;
        ok = D_TC_CHECK(first_real == 1u)            && ok;
        ok = D_TC_CHECK(k_no_callable != first_real) && ok;
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
