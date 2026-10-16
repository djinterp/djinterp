/*******************************************************************************
* djinterp [core]                                     color_rgb_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_rgb_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_rgb_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_rgb_model_type_identity
  Verifies rgb publishes the two nested types the conversion machinery reads.
  Tests the following:
  - model_tag is rgb_tag and derives from color_model_tag
  - value_type is channel_t
*/
bool
tests_color_rgb_model_type_identity()
{
    D_CR_CHECK((std::is_same<rgb::model_tag, rgb_tag>::value));
    D_CR_CHECK((std::is_base_of<color_model_tag, rgb::model_tag>::value));
    D_CR_CHECK((std::is_same<rgb::value_type, channel_t>::value));

    D_CR_CHECK(cr_has_model_tag<rgb>::value);

    return true;
}

/*
tests_color_rgb_is_recognised_as_a_color_model
  Verifies is_color_model accepts rgb, which is what admits it to the
  conversion graph as the hub every other model routes through.
  Tests the following:
  - the trait reports true
  - it has the integral_constant shape a trait is expected to have, so it can
    be used for tag dispatch and not merely read for ::value
*/
bool
tests_color_rgb_is_recognised_as_a_color_model()
{
    D_CR_CHECK(is_color_model<rgb>::value);

    D_CR_CHECK((std::is_base_of<std::integral_constant<bool, true>,
                                is_color_model<rgb> >::value));
    D_CR_CHECK((std::is_same<is_color_model<rgb>::value_type, bool>::value));

    return true;
}

/*
tests_color_rgba_is_not_a_color_model
  Verifies is_color_model REJECTS rgba, and keeps rejecting it through every
  cv-ref qualification.  This is deliberate design, not an oversight: alpha
  has no meaning under a chromatic conversion, so the type system is used to
  make "convert this rgba to hsl" a compile error rather than a silent
  discard.
  Tests the following:
  - the trait reports false, bare and through cv-ref qualifiers
  - it has the integral_constant shape, so the rejection is a proper trait
    answer and not a missing specialisation
  - rgba genuinely lacks the nested model_tag the trait keys on
*/
bool
tests_color_rgba_is_not_a_color_model()
{
    D_CR_CHECK(!is_color_model<rgba>::value);
    D_CR_CHECK(!is_color_model<const rgba>::value);
    D_CR_CHECK(!is_color_model<rgba&>::value);
    D_CR_CHECK(!is_color_model<const rgba&>::value);
    D_CR_CHECK(!is_color_model<rgba&&>::value);

    D_CR_CHECK((std::is_base_of<std::integral_constant<bool, false>,
                                is_color_model<rgba> >::value));

    D_CR_CHECK(!cr_has_model_tag<rgba>::value);

    // and the contrast with rgb, on the very same trait
    D_CR_CHECK(is_color_model<rgb>::value);

    return true;
}

/*
tests_color_rgba_has_value_type_but_no_model_tag
  Verifies rgba still exposes value_type for generic channel code, so the
  exclusion from the conversion graph is specifically the missing model_tag
  and not a general lack of type information.
  Tests the following:
  - rgba::value_type exists and is channel_t, matching rgb
  - rgba nonetheless has no model_tag
*/
bool
tests_color_rgba_has_value_type_but_no_model_tag()
{
    D_CR_CHECK((std::is_same<rgba::value_type, channel_t>::value));
    D_CR_CHECK((std::is_same<rgba::value_type, rgb::value_type>::value));

    D_CR_CHECK(!cr_has_model_tag<rgba>::value);
    D_CR_CHECK(cr_has_model_tag<rgb>::value);

    return true;
}

/*
tests_color_rgb_and_rgba_are_distinct_types
  Verifies neither type derives from nor silently converts to the other, so
  going from rgba to rgb requires the explicit to_rgb call and alpha can never
  be dropped by accident.
  Tests the following:
  - the types are unrelated and differently sized
  - neither is implicitly convertible to the other in either direction
*/
bool
tests_color_rgb_and_rgba_are_distinct_types()
{
    D_CR_CHECK(!(std::is_same<rgb, rgba>::value));
    D_CR_CHECK(!(std::is_base_of<rgb, rgba>::value));
    D_CR_CHECK(!(std::is_base_of<rgba, rgb>::value));

    D_CR_CHECK(!(std::is_convertible<rgba, rgb>::value));
    D_CR_CHECK(sizeof(rgb) != sizeof(rgba));

    return true;
}

/*
tests_color_rgb_detection_sees_through_cv_and_reference
  Verifies is_color_model<rgb> holds through const, lvalue-reference and
  rvalue-reference qualification, so a model passed by const reference is
  still recognised at the call site.
  Tests the following:
  - the trait answers true for every qualified spelling of rgb
*/
bool
tests_color_rgb_detection_sees_through_cv_and_reference()
{
    D_CR_CHECK(is_color_model<rgb>::value);
    D_CR_CHECK(is_color_model<const rgb>::value);
    D_CR_CHECK(is_color_model<rgb&>::value);
    D_CR_CHECK(is_color_model<const rgb&>::value);
    D_CR_CHECK(is_color_model<rgb&&>::value);

    return true;
}

/*
tests_color_rgb_channel_type_is_the_common_scalar
  Verifies rgb::value_type is the same channel_t the rest of the color
  subframework uses, so channels interoperate across models without
  conversion.
  Tests the following:
  - value_type is channel_t, and channel_t is a floating-point scalar
  - the member channels have exactly that type
*/
bool
tests_color_rgb_channel_type_is_the_common_scalar()
{
    D_CR_CHECK((std::is_same<rgb::value_type, channel_t>::value));
    D_CR_CHECK(std::is_floating_point<channel_t>::value);

    // decltype on the member NAME gives its declared type, so this pins the
    // storage type itself rather than the constness of any one object
    D_CR_CHECK((std::is_same<decltype(rgb::r), channel_t>::value));
    D_CR_CHECK((std::is_same<decltype(rgb::g), channel_t>::value));
    D_CR_CHECK((std::is_same<decltype(rgb::b), channel_t>::value));
    D_CR_CHECK((std::is_same<decltype(rgba::a), channel_t>::value));

    return true;
}

NS_END
NS_END
