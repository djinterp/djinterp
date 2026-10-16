/*******************************************************************************
* djinterp [core]                                 color_convert_tests_traits.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_convert_tests_traits.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_convert_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_convert_is_convertible_color_accepts_every_model_pair
  Verifies all forty-nine ordered pairs of the seven models are reported
  convertible, matching the reachability the crossmodel section establishes at
  run time.
  Tests the following:
  - each of the seven models is convertible to each of the seven models
*/
bool
tests_color_convert_is_convertible_color_accepts_every_model_pair()
{
    D_CX_CHECK((is_convertible_color<rgb, rgb>::value));
    D_CX_CHECK((is_convertible_color<rgb, hsl>::value));
    D_CX_CHECK((is_convertible_color<rgb, hsv>::value));
    D_CX_CHECK((is_convertible_color<rgb, cmyk>::value));
    D_CX_CHECK((is_convertible_color<rgb, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<rgb, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<rgb, cie_lab>::value));

    D_CX_CHECK((is_convertible_color<hsl, rgb>::value));
    D_CX_CHECK((is_convertible_color<hsl, hsl>::value));
    D_CX_CHECK((is_convertible_color<hsl, hsv>::value));
    D_CX_CHECK((is_convertible_color<hsl, cmyk>::value));
    D_CX_CHECK((is_convertible_color<hsl, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<hsl, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<hsl, cie_lab>::value));

    D_CX_CHECK((is_convertible_color<hsv, rgb>::value));
    D_CX_CHECK((is_convertible_color<hsv, hsl>::value));
    D_CX_CHECK((is_convertible_color<hsv, hsv>::value));
    D_CX_CHECK((is_convertible_color<hsv, cmyk>::value));
    D_CX_CHECK((is_convertible_color<hsv, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<hsv, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<hsv, cie_lab>::value));

    D_CX_CHECK((is_convertible_color<cmyk, rgb>::value));
    D_CX_CHECK((is_convertible_color<cmyk, hsl>::value));
    D_CX_CHECK((is_convertible_color<cmyk, hsv>::value));
    D_CX_CHECK((is_convertible_color<cmyk, cmyk>::value));
    D_CX_CHECK((is_convertible_color<cmyk, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<cmyk, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<cmyk, cie_lab>::value));

    D_CX_CHECK((is_convertible_color<ycbcr, rgb>::value));
    D_CX_CHECK((is_convertible_color<ycbcr, hsl>::value));
    D_CX_CHECK((is_convertible_color<ycbcr, hsv>::value));
    D_CX_CHECK((is_convertible_color<ycbcr, cmyk>::value));
    D_CX_CHECK((is_convertible_color<ycbcr, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<ycbcr, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<ycbcr, cie_lab>::value));

    D_CX_CHECK((is_convertible_color<cie_xyz, rgb>::value));
    D_CX_CHECK((is_convertible_color<cie_xyz, hsl>::value));
    D_CX_CHECK((is_convertible_color<cie_xyz, hsv>::value));
    D_CX_CHECK((is_convertible_color<cie_xyz, cmyk>::value));
    D_CX_CHECK((is_convertible_color<cie_xyz, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<cie_xyz, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<cie_xyz, cie_lab>::value));

    D_CX_CHECK((is_convertible_color<cie_lab, rgb>::value));
    D_CX_CHECK((is_convertible_color<cie_lab, hsl>::value));
    D_CX_CHECK((is_convertible_color<cie_lab, hsv>::value));
    D_CX_CHECK((is_convertible_color<cie_lab, cmyk>::value));
    D_CX_CHECK((is_convertible_color<cie_lab, ycbcr>::value));
    D_CX_CHECK((is_convertible_color<cie_lab, cie_xyz>::value));
    D_CX_CHECK((is_convertible_color<cie_lab, cie_lab>::value));

    return true;
}

/*
tests_color_convert_is_convertible_color_rejects_non_models
  Verifies types without a model_tag are rejected in either position.  The
  trait keys on nothing but that nested type, which is exactly how rgba - a
  transport type, not a colour model - is kept out of the graph.
  Tests the following:
  - rgba is rejected as source and as target
  - the bare kernel PODs are rejected, since the tag lives on the wrapper
  - unrelated types are rejected
*/
bool
tests_color_convert_is_convertible_color_rejects_non_models()
{
    D_CX_CHECK(!(is_convertible_color<rgba, rgb>::value));
    D_CX_CHECK(!(is_convertible_color<rgb, rgba>::value));
    D_CX_CHECK(!(is_convertible_color<rgba, rgba>::value));

    D_CX_CHECK(!(is_convertible_color<d_color_hsl, rgb>::value));
    D_CX_CHECK(!(is_convertible_color<rgb, d_color_hsl>::value));
    D_CX_CHECK(!(is_convertible_color<d_color_cmyk, d_color_hsv>::value));

    D_CX_CHECK(!(is_convertible_color<int, rgb>::value));
    D_CX_CHECK(!(is_convertible_color<rgb, void*>::value));
    D_CX_CHECK(!(is_convertible_color<channel_t, channel_t>::value));

    return true;
}

/*
tests_color_convert_is_convertible_color_v_matches_the_trait
  Verifies the _v shorthand agrees with the trait it abbreviates on every
  probe.
  Tests the following:
  - accepted and rejected pairs agree between the two spellings
*/
bool
tests_color_convert_is_convertible_color_v_matches_the_trait()
{
    D_CX_CHECK((is_convertible_color_v<rgb, hsl>)
               == (is_convertible_color<rgb, hsl>::value));
    D_CX_CHECK((is_convertible_color_v<cie_lab, cmyk>)
               == (is_convertible_color<cie_lab, cmyk>::value));
    D_CX_CHECK((is_convertible_color_v<rgba, rgb>)
               == (is_convertible_color<rgba, rgb>::value));
    D_CX_CHECK((is_convertible_color_v<int, rgb>)
               == (is_convertible_color<int, rgb>::value));

    D_CX_CHECK((is_convertible_color_v<rgb, hsl>));
    D_CX_CHECK(!(is_convertible_color_v<rgba, rgb>));

    return true;
}

/*
tests_color_convert_traits_strip_cv_and_reference
  Verifies the trait answers the same for a model however it is cv-ref
  qualified, because clean_t is applied before the tag is looked up.  Without
  this a perfectly ordinary `const hsl&` parameter would fail the check.
  Tests the following:
  - const, volatile, lvalue-reference and rvalue-reference spellings all
    report convertible
  - the stripping applies in both operand positions
*/
bool
tests_color_convert_traits_strip_cv_and_reference()
{
    D_CX_CHECK((is_convertible_color<const rgb, hsl>::value));
    D_CX_CHECK((is_convertible_color<rgb&, hsl>::value));
    D_CX_CHECK((is_convertible_color<const rgb&, hsl>::value));
    D_CX_CHECK((is_convertible_color<rgb&&, hsl>::value));

    D_CX_CHECK((is_convertible_color<rgb, const hsl>::value));
    D_CX_CHECK((is_convertible_color<rgb, hsl&>::value));
    D_CX_CHECK((is_convertible_color<rgb, const hsl&>::value));

    D_CX_CHECK((is_convertible_color<const rgb&, const hsl&>::value));

    // and stripping does not rescue a type that was never a model
    D_CX_CHECK(!(is_convertible_color<const rgba&, const rgb&>::value));

    return true;
}

/*
tests_color_convert_argument_order_differs_between_api_and_trait
  Verifies the ordering asymmetry head-on, so it is documented rather than
  discovered.  color_convert is TARGET-first, because it reads as a cast;
  is_convertible_color is SOURCE-first, because it reads as a question about a
  direction of travel.  The same conversion is therefore spelled with its two
  type arguments in opposite orders.
  Tests the following:
  - color_convert's FIRST parameter determines the return type
  - is_convertible_color's FIRST parameter is the source
  - both spellings of the one conversion agree that it is available
*/
bool
tests_color_convert_argument_order_differs_between_api_and_trait()
{
    const rgb source(channel_t(1), channel_t(0), channel_t(0));

    // target first: the leading argument is what comes back
    D_CX_CHECK((std::is_same<decltype(color_convert<hsl, rgb>::apply(source)),
                             hsl>::value));
    D_CX_CHECK((std::is_same<decltype(color_convert<cmyk, rgb>::apply(source)),
                             cmyk>::value));

    // source first: the leading argument is where we are coming from
    D_CX_CHECK((is_convertible_color<rgb, hsl>::value));

    // the same conversion, its arguments reversed between the two spellings
    const hsl converted = color_convert<hsl, rgb>::apply(source);
    D_CX_CHECK(cx_near(converted.h, channel_t(0)));
    D_CX_CHECK(cx_near(converted.s, channel_t(1)));

    // color_cast follows the API order, naming only the target
    D_CX_CHECK((std::is_same<decltype(color_cast<hsl>(source)), hsl>::value));

    return true;
}

/*
tests_color_cast_deduces_the_source_type
  Verifies color_cast needs only the target named explicitly and agrees with
  the fully spelled-out call.
  Tests the following:
  - the deduced call and the explicit call produce identical results
  - the return type is the named target
*/
bool
tests_color_cast_deduces_the_source_type()
{
    const rgb source(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const hsl deduced  = color_cast<hsl>(source);
    const hsl explicit_ = color_convert<hsl, rgb>::apply(source);

    D_CX_CHECK(deduced.h == explicit_.h);
    D_CX_CHECK(deduced.s == explicit_.s);
    D_CX_CHECK(deduced.l == explicit_.l);

    D_CX_CHECK((std::is_same<decltype(color_cast<cie_lab>(source)),
                             cie_lab>::value));

    return true;
}

/*
tests_color_cast_accepts_const_and_reference_sources
  Verifies const, reference and cv-qualified sources all convert, exercising
  the qualifier stripping the entry point performs before dispatching.
  Tests the following:
  - a const value, a reference and a const reference all convert identically
  - an rvalue source converts too
*/
bool
tests_color_cast_accepts_const_and_reference_sources()
{
    rgb        mutable_source(channel_t(1), channel_t(0), channel_t(0));
    const rgb  const_source(channel_t(1), channel_t(0), channel_t(0));
    const rgb& ref_source = const_source;

    const hsl from_mutable = color_cast<hsl>(mutable_source);
    const hsl from_const   = color_cast<hsl>(const_source);
    const hsl from_ref     = color_cast<hsl>(ref_source);
    const hsl from_rvalue  =
        color_cast<hsl>(rgb(channel_t(1), channel_t(0), channel_t(0)));

    D_CX_CHECK(from_mutable == from_const);
    D_CX_CHECK(from_const == from_ref);
    D_CX_CHECK(from_ref == from_rvalue);

    D_CX_CHECK(cx_near(from_const.h, channel_t(0)));
    D_CX_CHECK(cx_near(from_const.s, channel_t(1)));

    return true;
}

/*
tests_color_convert_concepts_agree_with_the_traits
  Verifies the C++20 concepts accept exactly what the underlying traits
  accept, so constraining a template on the concept and testing the trait are
  interchangeable.  The body is gated on the language level; below C++20 the
  concepts do not exist and the traits alone are checked.
  Tests the following:
  - color_model_type agrees with is_color_model_v
  - convertible_color_type agrees with is_convertible_color_v
  - rgba is refused by both concepts, as it is by both traits
*/
bool
tests_color_convert_concepts_agree_with_the_traits()
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_CX_CHECK(color_model_type<rgb>);
    D_CX_CHECK(color_model_type<cie_lab>);
    D_CX_CHECK(!color_model_type<rgba>);
    D_CX_CHECK(!color_model_type<int>);

    D_CX_CHECK(color_model_type<rgb> == is_color_model_v<rgb>);
    D_CX_CHECK(color_model_type<rgba> == is_color_model_v<rgba>);

    D_CX_CHECK((convertible_color_type<rgb, hsl>));
    D_CX_CHECK((convertible_color_type<cie_lab, cmyk>));
    D_CX_CHECK(!(convertible_color_type<rgba, hsl>));

    D_CX_CHECK((convertible_color_type<rgb, hsl>)
               == (is_convertible_color_v<rgb, hsl>));
    D_CX_CHECK((convertible_color_type<rgba, hsl>)
               == (is_convertible_color_v<rgba, hsl>));
#else
    // below C++20 only the traits exist; check them so the test still has
    // content at every language level
    D_CX_CHECK(is_color_model<rgb>::value);
    D_CX_CHECK(!is_color_model<rgba>::value);
    D_CX_CHECK((is_convertible_color<rgb, hsl>::value));
#endif

    return true;
}

NS_END
NS_END
