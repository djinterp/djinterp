/*******************************************************************************
* djinterp [test]                                 color_common_tests_channel.cpp
*
*   The color_common suite's channel block: channel_t is floating point, the
* C kernel's float, keeps arithmetic's results, and holds values outside the
* gamut.
*
*
* path:      /tests/djinterp/cpp/util/color/color_common_tests_channel.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.18
*******************************************************************************/
// djinterp
#include "color_common_tests.hpp"  // the suite's declarations
// djinterp
#include "../../../../../inc/djinterp/c/util/color/color_rgb.h"  // dj_color_rgb

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
bool tests_color_common_channel_is_floating_point()
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
tests_color_common_channel_is_the_kernels_float
  Verifies channel_t is float, the C kernel's channel, which every model
  stores. (D_COLOR_CHANNEL_TYPE, which once offered to change it, is retired:
  defined as anything but float it stops the build.)
  Tests the following:
  - the alias is float
  - it is the type of the C structs' channels
*/
bool tests_color_common_channel_is_the_kernels_float()
{
    D_CC_CHECK(std::is_same<channel_t, float>::value);
    D_CC_CHECK((std::is_same<channel_t,
                             decltype(dj_color_rgb::r)>::value));
    D_CC_CHECK((std::is_same<channel_t,
                             decltype(dj_color_rgba::a)>::value));

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
bool tests_color_common_channel_arithmetic_round_trips()
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
bool tests_color_common_channel_holds_out_of_gamut_values()
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
