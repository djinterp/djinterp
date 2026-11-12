/*******************************************************************************
* djinterp [cpp][test]                             test_common_tests_type_id.cpp
*
*
* path:      /tests/djinterp/cpp/modules/test/test_common_tests_type_id.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <climits>
#include <cstdint>
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

// -- DTestTypeId : exactly std::int32_t --------------------------------------
static_assert(std::is_same<DTestTypeId, std::int32_t>::value,
              "test_type_id must be std::int32_t");
static_assert(std::is_integral<DTestTypeId>::value,
              "test_type_id must be integral");
static_assert(std::is_signed<DTestTypeId>::value,
              "test_type_id must be signed");
static_assert(sizeof(DTestTypeId) == 4,
              "test_type_id must occupy 4 bytes");
static_assert(CHAR_BIT * sizeof(DTestTypeId) == 32,
              "test_type_id must be exactly 32 bits wide");
static_assert(std::numeric_limits<DTestTypeId>::is_signed,
              "test_type_id must be a signed arithmetic type");
static_assert(std::numeric_limits<DTestTypeId>::digits == 31,
              "test_type_id must expose 31 value bits (+ sign)");
static_assert(std::numeric_limits<DTestTypeId>::min() == INT32_MIN,
              "test_type_id min must equal INT32_MIN");
static_assert(std::numeric_limits<DTestTypeId>::max() == INT32_MAX,
              "test_type_id max must equal INT32_MAX");

// -- DTestCallableId : exactly std::uint32_t ---------------------------------
static_assert(std::is_same<DTestCallableId, std::uint32_t>::value,
              "test_callable_id must be std::uint32_t");
static_assert(std::is_integral<DTestCallableId>::value,
              "test_callable_id must be integral");
static_assert(std::is_unsigned<DTestCallableId>::value,
              "test_callable_id must be unsigned");
static_assert(sizeof(DTestCallableId) == 4,
              "test_callable_id must occupy 4 bytes");
static_assert(CHAR_BIT * sizeof(DTestCallableId) == 32,
              "test_callable_id must be exactly 32 bits wide");
static_assert(std::numeric_limits<DTestCallableId>::digits == 32,
              "test_callable_id must expose 32 value bits");
static_assert(std::numeric_limits<DTestCallableId>::max() == UINT32_MAX,
              "test_callable_id max must equal UINT32_MAX");

// -- k_no_callable : constexpr zero sentinel of type DTestCallableId ---------
static_assert(k_no_callable == 0,
              "k_no_callable must equal 0");
static_assert(std::is_same<std::remove_cv<decltype(k_no_callable)>::type,
                           DTestCallableId>::value,
              "k_no_callable must be of type test_callable_id");
// usable as a non-type template argument -> proves it is a constant expression
static_assert(std::integral_constant<DTestCallableId, k_no_callable>::value
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
bool tests_test_type_id()
{
    bool ok = true;

    // identity & classification
    ok = D_TC_CHECK(std::is_same<DTestTypeId, std::int32_t>::value) && ok;
    ok = D_TC_CHECK(std::is_integral<DTestTypeId>::value)           && ok;
    ok = D_TC_CHECK(std::is_signed<DTestTypeId>::value)             && ok;
    ok = D_TC_CHECK(sizeof(DTestTypeId) == 4)                       && ok;
    ok = D_TC_CHECK(CHAR_BIT * sizeof(DTestTypeId) == 32)           && ok;

    // numeric range
    ok = D_TC_CHECK(std::numeric_limits<DTestTypeId>::min() == INT32_MIN)
         && ok;
    ok = D_TC_CHECK(std::numeric_limits<DTestTypeId>::max() == INT32_MAX)
         && ok;

    // value round-trips across the sign boundary and the extremes
    {
        DTestTypeId zero = 0;
        DTestTypeId neg  = -42;
        DTestTypeId pos  = 42;
        DTestTypeId lo   = INT32_MIN;
        DTestTypeId hi   = INT32_MAX;

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
        DTestTypeId child  = -1;
        DTestTypeId parent = 0;

        ok = D_TC_CHECK(child <= parent)            && ok;  // strictly lower
        ok = D_TC_CHECK(parent <= parent)           && ok;  // equal ranks ok
        ok = D_TC_CHECK(DTestTypeId(INT32_MIN)
                            <= DTestTypeId(INT32_MAX)) && ok;  // extremes
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
bool tests_test_callable_id()
{
    bool ok = true;

    // identity & classification
    ok = D_TC_CHECK(std::is_same<DTestCallableId, std::uint32_t>::value) && ok;
    ok = D_TC_CHECK(std::is_integral<DTestCallableId>::value)            && ok;
    ok = D_TC_CHECK(std::is_unsigned<DTestCallableId>::value)            && ok;
    ok = D_TC_CHECK(sizeof(DTestCallableId) == 4)                        && ok;
    ok = D_TC_CHECK(CHAR_BIT * sizeof(DTestCallableId) == 32)            && ok;

    // capacity: documented as "up to ~4 billion distinct callables"
    ok = D_TC_CHECK(std::numeric_limits<DTestCallableId>::max() == UINT32_MAX)
         && ok;
    ok = D_TC_CHECK(std::numeric_limits<DTestCallableId>::max()
                        >= 4000000000u) && ok;

    // representative values round-trip
    {
        DTestCallableId zero = 0;
        DTestCallableId one  = 1;
        DTestCallableId big  = UINT32_MAX;

        ok = D_TC_CHECK(zero == 0u)          && ok;
        ok = D_TC_CHECK(one  == 1u)          && ok;
        ok = D_TC_CHECK(big  == UINT32_MAX)  && ok;
        ok = D_TC_CHECK(zero <  one)         && ok;
        ok = D_TC_CHECK(one  <  big)         && ok;
    }

    // unsigned arithmetic wraps modulo 2^32 (well-defined, not UB)
    {
        DTestCallableId big = UINT32_MAX;

        ok = D_TC_CHECK(static_cast<DTestCallableId>(big + 1u) == 0u) && ok;
    }

    return ok;
}


/*
tests_k_no_callable
  Verifies the reserved "no deferred callable" sentinel.
  Tests the following:
  - the constant equals 0 and is typed as DTestCallableId
  - it is a genuine compile-time constant (usable where one is required)
  - it is interchangeable with a freshly zero-cast handle
  - it is distinct from the first valid (non-zero) handle value
*/
bool tests_k_no_callable()
{
    bool ok = true;

    // value & type
    ok = D_TC_CHECK(k_no_callable == 0u)                                && ok;
    ok = D_TC_CHECK(k_no_callable == static_cast<DTestCallableId>(0))  && ok;
    ok = D_TC_CHECK(std::is_same<std::remove_cv<decltype(k_no_callable)>::type,
                                 DTestCallableId>::value)              && ok;

    // compile-time constant: a local constexpr must accept it
    {
        D_CONSTEXPR DTestCallableId mirror = k_no_callable;
        ok = D_TC_CHECK(mirror == 0u) && ok;
    }

    // sentinel is distinct from the first real handle (id 1)
    {
        DTestCallableId first_real = k_no_callable + 1u;
        ok = D_TC_CHECK(first_real == 1u)            && ok;
        ok = D_TC_CHECK(k_no_callable != first_real) && ok;
    }

    return ok;
}


NS_END  // testing
NS_END  // djinterp
