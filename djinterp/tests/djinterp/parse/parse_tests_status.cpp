#include <climits>

#include "parse_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
  A note on why this block asserts at RUNTIME rather than with static_assert.
D_CONSTEXPR is configurable: cfg_qualifiers.h can strip it to nothing for
test instrumentation (D_CFG_TESTING_STRIP_CONSTEXPR), in which case the
DParseStatus* constants are ordinary objects and are not usable in constant
expressions.  Runtime checks hold under every configuration, so the block is
written that way throughout.  Pure type-level facts, which do not depend on
the constants, still use static_assert-equivalent trait checks.
*/

/*
tests_parse_status_underlying_type
  Verifies parse_status is exactly the integral type the module documents.
  Tests the following:
  - parse_status is std::int32_t and not merely convertible to it,
  - it is a signed, four-byte integral type,
  - it is not an enumeration or a class type, so codes are open-ended,
  - the standard constants have that exact type.
*/
bool
tests_parse_status_underlying_type()
{
    // the documented spelling, matched exactly
    D_PA_CHECK((std::is_same<dp::parse_status, std::int32_t>::value));

    // and its properties, so a change of typedef is caught even if the new
    // type happens to be another 32-bit integer
    D_PA_CHECK(std::is_integral<dp::parse_status>::value);
    D_PA_CHECK(std::is_signed<dp::parse_status>::value);
    D_PA_CHECK(sizeof(dp::parse_status) == 4u);

    // an open integral code space, not a closed enumeration
    D_PA_CHECK(!std::is_enum<dp::parse_status>::value);
    D_PA_CHECK(!std::is_class<dp::parse_status>::value);

    // the constants themselves carry the type
    D_PA_CHECK((std::is_same<decltype(dp::DParseStatusSuccess),
                             const dp::parse_status>::value ||
                std::is_same<decltype(dp::DParseStatusSuccess),
                             dp::parse_status>::value));

    return true;
}

/*
tests_parse_status_constant_values
  Verifies each standard code carries its documented numeric value, so a
persisted or transmitted status keeps its meaning.
  Tests the following:
  - Success 0, Failure 1, EndOfInput 2, Overflow 3, Malformed 4,
  - UserBase 64,
  - the five outcome codes are contiguous from zero.
*/
bool
tests_parse_status_constant_values()
{
    // the documented wire values
    D_PA_CHECK(dp::DParseStatusSuccess    == 0);
    D_PA_CHECK(dp::DParseStatusFailure    == 1);
    D_PA_CHECK(dp::DParseStatusEndOfInput == 2);
    D_PA_CHECK(dp::DParseStatusOverflow   == 3);
    D_PA_CHECK(dp::DParseStatusMalformed  == 4);
    D_PA_CHECK(dp::DParseStatusUserBase   == 64);

    // the outcome codes form a contiguous run from zero, so they can index
    // a table of descriptions without a mapping step
    D_PA_CHECK((dp::DParseStatusFailure - dp::DParseStatusSuccess)    == 1);
    D_PA_CHECK((dp::DParseStatusEndOfInput - dp::DParseStatusFailure) == 1);
    D_PA_CHECK((dp::DParseStatusOverflow -
                dp::DParseStatusEndOfInput)                           == 1);
    D_PA_CHECK((dp::DParseStatusMalformed - dp::DParseStatusOverflow) == 1);

    return true;
}

/*
tests_parse_status_constants_distinct
  Verifies no two standard codes share a value, so a switch over them is
well-formed and no outcome can be mistaken for another.
  Tests the following:
  - all fifteen unordered pairs of the six constants differ.
*/
bool
tests_parse_status_constants_distinct()
{
    const dp::parse_status codes[6] =
    {
        dp::DParseStatusSuccess,
        dp::DParseStatusFailure,
        dp::DParseStatusEndOfInput,
        dp::DParseStatusOverflow,
        dp::DParseStatusMalformed,
        dp::DParseStatusUserBase
    };

    std::size_t i;
    std::size_t j;

    // exhaustive pairwise comparison over the whole constant set
    for (i = 0; i < 6u; ++i)
    {
        for (j = (i + 1u); j < 6u; ++j)
        {
            D_PA_CHECK(codes[i] != codes[j]);
        }
    }

    return true;
}

/*
tests_parse_status_standard_range
  Verifies the shape of the standard code range relative to the user base.
  Tests the following:
  - Success is the only zero-valued standard code,
  - every failure code is non-zero, so `status != Success` is a valid
    failure test,
  - every standard code sits strictly below DParseStatusUserBase, leaving
    the range reserved as documented.
*/
bool
tests_parse_status_standard_range()
{
    // success is uniquely zero, so a zero test is a success test
    D_PA_CHECK(dp::DParseStatusSuccess == 0);
    D_PA_CHECK(dp::DParseStatusFailure    != dp::DParseStatusSuccess);
    D_PA_CHECK(dp::DParseStatusEndOfInput != dp::DParseStatusSuccess);
    D_PA_CHECK(dp::DParseStatusOverflow   != dp::DParseStatusSuccess);
    D_PA_CHECK(dp::DParseStatusMalformed  != dp::DParseStatusSuccess);

    // the reserved band below the user base is genuinely reserved
    D_PA_CHECK(dp::DParseStatusSuccess    < dp::DParseStatusUserBase);
    D_PA_CHECK(dp::DParseStatusFailure    < dp::DParseStatusUserBase);
    D_PA_CHECK(dp::DParseStatusEndOfInput < dp::DParseStatusUserBase);
    D_PA_CHECK(dp::DParseStatusOverflow   < dp::DParseStatusUserBase);
    D_PA_CHECK(dp::DParseStatusMalformed  < dp::DParseStatusUserBase);

    // and there is real headroom left between the last standard code and
    // the user base, so codes can be added without a flag day
    D_PA_CHECK((dp::DParseStatusUserBase - dp::DParseStatusMalformed) > 1);

    return true;
}

/*
tests_parse_status_user_range
  Verifies that derived parsers can define their own codes above the base
without colliding with the standard ones.
  Tests the following:
  - codes at UserBase and above differ from every standard code,
  - they round-trip through parse_error unchanged,
  - they round-trip through a parse_result's error unchanged.
*/
bool
tests_parse_status_user_range()
{
    const dp::parse_status user_first = dp::DParseStatusUserBase;
    const dp::parse_status user_tenth = dp::DParseStatusUserBase + 10;

    // no user code can alias a standard one
    D_PA_CHECK(user_first != dp::DParseStatusSuccess);
    D_PA_CHECK(user_first != dp::DParseStatusFailure);
    D_PA_CHECK(user_first != dp::DParseStatusEndOfInput);
    D_PA_CHECK(user_first != dp::DParseStatusOverflow);
    D_PA_CHECK(user_first != dp::DParseStatusMalformed);
    D_PA_CHECK(user_tenth > dp::DParseStatusMalformed);

    // a user code survives storage in an error descriptor
    dp::parse_error custom(user_tenth, 12u, "domain-specific failure");

    D_PA_CHECK(custom.status() == user_tenth);
    D_PA_CHECK(custom.status() == (dp::DParseStatusUserBase + 10));
    D_PA_CHECK(custom.offset() == 12u);

    // and through the result carrier the parser actually returns
    dp::parse_result<int> outcome(custom);

    D_PA_CHECK(!outcome.ok());
    D_PA_CHECK(outcome.error().status() == user_tenth);

    return true;
}

/*
tests_parse_status_extremes_round_trip
  Verifies the code space really is the whole of int32_t, so a derived
parser is free to use negative or very large codes.
  Tests the following:
  - INT32_MIN and INT32_MAX are storable and readable,
  - negative codes are distinct from all standard codes,
  - the extremes survive a parse_result round trip.
*/
bool
tests_parse_status_extremes_round_trip()
{
    const dp::parse_status lowest  = INT32_MIN;
    const dp::parse_status highest = INT32_MAX;

    // the extremes are representable in the type
    D_PA_CHECK(lowest  < 0);
    D_PA_CHECK(highest > dp::DParseStatusUserBase);

    // negative codes cannot collide with the (non-negative) standard set
    D_PA_CHECK(lowest != dp::DParseStatusSuccess);
    D_PA_CHECK(lowest != dp::DParseStatusFailure);

    // both survive a descriptor round trip unchanged
    dp::parse_error low_error(lowest, 0u, "lowest");
    dp::parse_error high_error(highest, 1u, "highest");

    D_PA_CHECK(low_error.status()  == INT32_MIN);
    D_PA_CHECK(high_error.status() == INT32_MAX);

    // and a result round trip
    dp::parse_result<char> low_result(low_error);
    dp::parse_result<char> high_result(high_error);

    D_PA_CHECK(low_result.error().status()  == INT32_MIN);
    D_PA_CHECK(high_result.error().status() == INT32_MAX);

    return true;
}

/*
tests_parse_status_classification
  Verifies that the code space supports the classification a caller
naturally writes over it.
  Tests the following:
  - a success / failure split keyed on DParseStatusSuccess,
  - a standard / user split keyed on DParseStatusUserBase,
  - an unknown standard-range code is classified as standard-but-unnamed
    rather than as a user code.
*/
bool
tests_parse_status_classification()
{
    const dp::parse_status samples[7] =
    {
        dp::DParseStatusSuccess,
        dp::DParseStatusFailure,
        dp::DParseStatusEndOfInput,
        dp::DParseStatusOverflow,
        dp::DParseStatusMalformed,
        32,                                   // reserved, unnamed
        dp::DParseStatusUserBase + 3          // derived parser's own code
    };

    std::size_t i;
    std::size_t successes;
    std::size_t user_codes;

    successes  = 0;
    user_codes = 0;

    // classify each sample the way a caller would
    for (i = 0; i < 7u; ++i)
    {
        if (samples[i] == dp::DParseStatusSuccess)
        {
            ++successes;
        }

        if (samples[i] >= dp::DParseStatusUserBase)
        {
            ++user_codes;
        }
    }

    // exactly one sample is a success and exactly one is a user code
    D_PA_CHECK(successes  == 1u);
    D_PA_CHECK(user_codes == 1u);

    // the unnamed reserved code is neither a named standard code nor a
    // user code -- it belongs to the reserved band
    D_PA_CHECK(samples[5] != dp::DParseStatusMalformed);
    D_PA_CHECK(samples[5] <  dp::DParseStatusUserBase);
    D_PA_CHECK(samples[5] >  dp::DParseStatusMalformed);

    return true;
}

NS_END  // testing
NS_END  // djinterp
