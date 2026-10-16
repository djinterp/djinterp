/*******************************************************************************
* djinterp [core]                                 color_common_tests_channel.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_common_tests_channel.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_common_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_common_channel_is_floating_point
  Verifies channel_t is a floating-point arithmetic type.
  Tests the following:
  - std::is_floating_point / is_arithmetic hold
  - the type is not integral and not const/volatile qualified
  - it carries an IEEE-style epsilon and infinity
*/
bool
tests_color_common_channel_is_floating_point()
{
    D_CC_CHECK(std::is_floating_point<channel_t>::value);
    D_CC_CHECK(std::is_arithmetic<channel_t>::value);
    D_CC_CHECK(!std::is_integral<channel_t>::value);
    D_CC_CHECK(std::is_same<channel_t, clean_t<channel_t>>::value);
    D_CC_CHECK(std::numeric_limits<channel_t>::is_iec559);
    D_CC_CHECK(std::numeric_limits<channel_t>::epsilon() > channel_t(0));
    D_CC_CHECK(std::numeric_limits<channel_t>::has_infinity);

    return true;
}

/*
tests_color_common_channel_default_is_float
  Verifies channel_t defaults to float when D_COLOR_CHANNEL_TYPE is not
  pre-defined by the includer.
  Tests the following:
  - the alias resolves to float
  - the override macro is nevertheless defined (the hook exists)
  - the macro and the alias agree
*/
bool
tests_color_common_channel_default_is_float()
{
#ifndef D_COLOR_CHANNEL_TYPE
    D_CC_CHECK(false && "D_COLOR_CHANNEL_TYPE must be defined by the header");
#endif

    D_CC_CHECK(std::is_same<channel_t, D_COLOR_CHANNEL_TYPE>::value);
    D_CC_CHECK(std::is_same<channel_t, float>::value);
    D_CC_CHECK(sizeof(channel_t) == sizeof(float));

    return true;
}

/*
tests_color_common_channel_arithmetic_round_trips
  Verifies exactly representable channel arithmetic round-trips without drift.
  Tests the following:
  - binary fractions survive add/subtract and multiply/divide
  - unity and zero are exact
  - the type distinguishes values one epsilon apart
*/
bool
tests_color_common_channel_arithmetic_round_trips()
{
    const channel_t half    = channel_t(0.5);
    const channel_t quarter = channel_t(0.25);

    D_CC_CHECK((half + quarter) - quarter == half);
    D_CC_CHECK((half * channel_t(2)) == channel_t(1));
    D_CC_CHECK((half / channel_t(2)) == quarter);
    D_CC_CHECK(channel_t(1) - channel_t(1) == channel_t(0));
    D_CC_CHECK(channel_t(0) == -channel_t(0));

    const channel_t eps = std::numeric_limits<channel_t>::epsilon();
    D_CC_CHECK((channel_t(1) + eps) != channel_t(1));

    return true;
}

/*
tests_color_common_channel_holds_out_of_gamut_values
  Verifies channel_t stores negative and above-unity values unchanged - the
  scalar itself imposes no range policy; clamping is a separate decision.
  Tests the following:
  - negative, above-unity and large magnitudes survive storage
  - infinities and NaN are representable
*/
bool
tests_color_common_channel_holds_out_of_gamut_values()
{
    const channel_t negative = channel_t(-3.5);
    const channel_t over     = channel_t(17.25);

    D_CC_CHECK(negative == channel_t(-3.5));
    D_CC_CHECK(over == channel_t(17.25));
    D_CC_CHECK(negative < channel_t(0));
    D_CC_CHECK(over > channel_t(1));

    const channel_t inf = cc_inf();
    D_CC_CHECK(inf > channel_t(0));
    D_CC_CHECK(-inf < channel_t(0));

    const channel_t nan = cc_nan();
    D_CC_CHECK(nan != nan);

    return true;
}

NS_END
NS_END
