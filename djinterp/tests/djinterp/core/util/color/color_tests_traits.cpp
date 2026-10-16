/*******************************************************************************
* djinterp [core]                                         color_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_umbrella_is_convertible_color_accepts_model_pairs
  Verifies every ordered pair of the two models this header defines is
  reported convertible.
  Tests the following:
  - all four ordered pairs over rgb and cmyk report true
*/
bool
tests_color_umbrella_is_convertible_color_accepts_model_pairs()
{
    D_CU_CHECK((is_convertible_color<rgb, rgb>::value));
    D_CU_CHECK((is_convertible_color<rgb, cmyk>::value));
    D_CU_CHECK((is_convertible_color<cmyk, rgb>::value));
    D_CU_CHECK((is_convertible_color<cmyk, cmyk>::value));

    return true;
}

/*
tests_color_umbrella_is_convertible_color_rejects_rgba
  Verifies rgba is rejected in either position, because it carries no
  model_tag.  Unrelated types are rejected for the same reason.
  Tests the following:
  - rgba is rejected as source, as target, and paired with itself
  - non-colour types are rejected too
*/
bool
tests_color_umbrella_is_convertible_color_rejects_rgba()
{
    D_CU_CHECK(!(is_convertible_color<rgba, rgb>::value));
    D_CU_CHECK(!(is_convertible_color<rgb, rgba>::value));
    D_CU_CHECK(!(is_convertible_color<rgba, cmyk>::value));
    D_CU_CHECK(!(is_convertible_color<rgba, rgba>::value));

    D_CU_CHECK(!(is_convertible_color<int, rgb>::value));
    D_CU_CHECK(!(is_convertible_color<rgb, channel_t>::value));

    return true;
}

/*
tests_color_umbrella_is_convertible_color_has_integral_constant_shape
  Verifies the trait derives from integral_constant, so it is usable for tag
  dispatch and not merely readable through ::value.
  Tests the following:
  - a true and a false instantiation derive from the matching bool constant
  - value_type and the conversion operator behave as a trait's should
*/
bool
tests_color_umbrella_is_convertible_color_has_integral_constant_shape()
{
    D_CU_CHECK((std::is_base_of<std::integral_constant<bool, true>,
                                is_convertible_color<rgb, cmyk> >::value));
    D_CU_CHECK((std::is_base_of<std::integral_constant<bool, false>,
                                is_convertible_color<rgba, rgb> >::value));

    D_CU_CHECK((std::is_same<is_convertible_color<rgb, cmyk>::value_type,
                             bool>::value));

    // usable as a value, not merely as a member
    D_CU_CHECK((is_convertible_color<rgb, cmyk>()));

    return true;
}

/*
tests_color_umbrella_is_convertible_color_strips_cv_and_reference
  Verifies the trait answers the same however its operands are cv-ref
  qualified, so an ordinary `const rgb&` parameter is not accidentally
  rejected.
  Tests the following:
  - qualified spellings of a model still report convertible, in either
    position
  - qualification does not rescue a type that was never a model
*/
bool
tests_color_umbrella_is_convertible_color_strips_cv_and_reference()
{
    D_CU_CHECK((is_convertible_color<const rgb, cmyk>::value));
    D_CU_CHECK((is_convertible_color<rgb&, cmyk>::value));
    D_CU_CHECK((is_convertible_color<const rgb&, cmyk>::value));
    D_CU_CHECK((is_convertible_color<rgb&&, cmyk>::value));

    D_CU_CHECK((is_convertible_color<rgb, const cmyk&>::value));
    D_CU_CHECK((is_convertible_color<const rgb&, const cmyk&>::value));

    D_CU_CHECK(!(is_convertible_color<const rgba&, const rgb&>::value));

    return true;
}

/*
tests_color_umbrella_trait_and_edge_existence_can_disagree
  Verifies the trait is NOT a proxy for edge existence.  The header's own note
  says as much - it asks only whether both endpoints are models - and the two
  answers genuinely diverge on rgba, which is reported non-convertible yet has
  a perfectly working identity edge.  Pinning this down keeps a future reader
  from treating the trait as a compile-time guard it was never meant to be.
  Tests the following:
  - for the two real models, trait and edge agree in all four ordered pairs
  - for rgba paired with itself they DISAGREE: the trait says no, the edge
    exists, and the conversion runs
  - for rgba paired with a model they agree again, both saying no
*/
bool
tests_color_umbrella_trait_and_edge_existence_can_disagree()
{
    // where both endpoints are models, the two questions coincide
    D_CU_CHECK((is_convertible_color<rgb, cmyk>::value)
               == (cu_edge_exists<color_convert<cmyk, rgb> >::value));
    D_CU_CHECK((is_convertible_color<cmyk, rgb>::value)
               == (cu_edge_exists<color_convert<rgb, cmyk> >::value));
    D_CU_CHECK((is_convertible_color<rgb, rgb>::value)
               == (cu_edge_exists<color_convert<rgb, rgb> >::value));

    // ... but on rgba they part company
    D_CU_CHECK(!(is_convertible_color<rgba, rgba>::value));
    D_CU_CHECK((cu_edge_exists<color_convert<rgba, rgba> >::value));

    const rgba v(channel_t(0.25), channel_t(0.5), channel_t(0.75),
                 channel_t(0.5));
    D_CU_CHECK(color_cast<rgba>(v) == v);

    // where rgba is paired with a model, both answers are no again
    D_CU_CHECK(!(is_convertible_color<rgba, rgb>::value));
    D_CU_CHECK(!(cu_edge_exists<color_convert<rgb, rgba> >::value));

    return true;
}

/*
tests_color_umbrella_exposes_the_foundation
  Verifies including the umbrella alone brings in everything the PDF layer
  needs: the channel scalar, the model tags, the detection trait and both
  transport types.  A caller should never have to include the pieces
  individually.
  Tests the following:
  - channel_t and the tag hierarchy are visible
  - is_color_model is visible and answers correctly for all three types
  - rgb, rgba and cmyk are all complete types here
*/
bool
tests_color_umbrella_exposes_the_foundation()
{
    D_CU_CHECK(std::is_floating_point<channel_t>::value);

    D_CU_CHECK((std::is_base_of<color_model_tag, rgb_tag>::value));
    D_CU_CHECK((std::is_base_of<color_model_tag, cmyk_tag>::value));

    D_CU_CHECK(is_color_model<rgb>::value);
    D_CU_CHECK(is_color_model<cmyk>::value);
    D_CU_CHECK(!is_color_model<rgba>::value);

    D_CU_CHECK(sizeof(rgb) == 3u * sizeof(channel_t));
    D_CU_CHECK(sizeof(rgba) == 4u * sizeof(channel_t));
    D_CU_CHECK(sizeof(cmyk) == 4u * sizeof(channel_t));

    // the transport bridge is reachable from the umbrella too
    const rgba transport(channel_t(1), channel_t(0), channel_t(0),
                         channel_t(0.5));
    D_CU_CHECK(transport.to_rgb() == rgb(channel_t(1), channel_t(0),
                                         channel_t(0)));

    return true;
}

NS_END
NS_END
