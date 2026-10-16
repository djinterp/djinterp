/*******************************************************************************
* djinterp [core]                               color_hsv_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsv_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsv_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsv_default_is_zeroed
  Verifies the default constructor zeroes hue, saturation and value.
  Tests the following:
  - all three channels are exactly zero
  - the zeroed value denotes black and is itself canonical
*/
bool
tests_color_hsv_default_is_zeroed()
{
    const hsv v;

    D_CV_CHECK(cv_same(v, channel_t(0), channel_t(0), channel_t(0)));
    D_CV_CHECK(v.h == channel_t(0));
    D_CV_CHECK(v.s == channel_t(0));
    D_CV_CHECK(v.v == channel_t(0));
    D_CV_CHECK(v.is_valid());

    return true;
}

/*
tests_color_hsv_parameterized_stores_channels_verbatim
  Verifies the channel constructor stores its arguments unchanged, imposing no
  range or wrapping policy of its own.
  Tests the following:
  - in-range values land on the matching channel, in order
  - out-of-range hue is NOT wrapped and out-of-unit channels are NOT clamped
*/
bool
tests_color_hsv_parameterized_stores_channels_verbatim()
{
    const hsv v(channel_t(210), channel_t(0.25), channel_t(0.75));

    D_CV_CHECK(cv_same(v, channel_t(210), channel_t(0.25), channel_t(0.75)));

    // argument order is h, s, v - no two channels are transposed
    D_CV_CHECK(v.h != v.s);
    D_CV_CHECK(v.s != v.v);

    const hsv wild(channel_t(720), channel_t(-1), channel_t(3));
    D_CV_CHECK(cv_same(wild, channel_t(720), channel_t(-1), channel_t(3)));
    D_CV_CHECK(!wild.is_valid());

    return true;
}

/*
tests_color_hsv_converting_constructor_wraps_pod
  Verifies a kernel-produced d_color_hsv implicitly wraps into a hsv with
  identical channels - the path every conversion result takes.
  Tests the following:
  - explicit construction from a POD copies all three channels
  - the conversion is implicit (copy-initialisation works)
*/
bool
tests_color_hsv_converting_constructor_wraps_pod()
{
    const d_color_hsv pod =
        d_color_hsv_make(channel_t(120), channel_t(0.5), channel_t(0.5));

    const hsv direct(pod);
    D_CV_CHECK(cv_same(direct, channel_t(120), channel_t(0.5), channel_t(0.5)));

    const hsv implicit = pod;
    D_CV_CHECK(cv_same(implicit, channel_t(120), channel_t(0.5), channel_t(0.5)));

    D_CV_CHECK((std::is_convertible<d_color_hsv, hsv>::value));

    return true;
}

/*
tests_color_hsv_slices_back_to_pod
  Verifies a hsv slices to its d_color_hsv base, so a wrapper can be handed
  straight to a kernel routine expecting the POD by value.
  Tests the following:
  - binding to a base reference exposes the same channels
  - copying to a base value preserves them
  - a kernel routine accepts the wrapper directly
*/
bool
tests_color_hsv_slices_back_to_pod()
{
    const hsv v(channel_t(45), channel_t(0.25), channel_t(0.6));

    const d_color_hsv& ref = v;
    D_CV_CHECK(ref.h == channel_t(45));
    D_CV_CHECK(ref.s == channel_t(0.25));
    D_CV_CHECK(ref.v == channel_t(0.6));

    const d_color_hsv copy = v;
    D_CV_CHECK(copy.h == channel_t(45));
    D_CV_CHECK(copy.v == channel_t(0.6));

    D_CV_CHECK(d_color_hsv_is_valid(v));

    return true;
}

/*
tests_color_hsv_adds_no_state_to_the_pod
  Verifies hsv derives publicly from d_color_hsv and adds no members, so the
  wrapper is a pure ergonomics layer over the shared kernel type.
  Tests the following:
  - public derivation from the POD
  - identical size and alignment, standard layout preserved
  - trivially copyable and destructible, but not trivially default
    constructible (the default constructor zeroes the channels)
*/
bool
tests_color_hsv_adds_no_state_to_the_pod()
{
    D_CV_CHECK((std::is_base_of<d_color_hsv, hsv>::value));
    D_CV_CHECK((std::is_convertible<hsv*, d_color_hsv*>::value));

    D_CV_CHECK(sizeof(hsv) == sizeof(d_color_hsv));
    D_CV_CHECK(sizeof(hsv) == 3u * sizeof(channel_t));
    D_CV_CHECK(alignof(hsv) == alignof(d_color_hsv));

    D_CV_CHECK(std::is_standard_layout<hsv>::value);
    D_CV_CHECK(std::is_trivially_copyable<hsv>::value);
    D_CV_CHECK(std::is_trivially_destructible<hsv>::value);
    D_CV_CHECK(!std::is_polymorphic<hsv>::value);
    D_CV_CHECK(!std::is_trivially_default_constructible<hsv>::value);

    return true;
}

/*
tests_color_hsv_copy_and_assignment
  Verifies copy construction and copy assignment reproduce every channel.
  Tests the following:
  - a copy carries the same channels and is independent of its source
  - assignment overwrites all three channels
*/
bool
tests_color_hsv_copy_and_assignment()
{
    const hsv src(channel_t(300), channel_t(0.4), channel_t(0.7));

    const hsv copied(src);
    D_CV_CHECK(cv_same(copied, channel_t(300), channel_t(0.4), channel_t(0.7)));

    hsv target;
    target = src;
    D_CV_CHECK(cv_same(target, channel_t(300), channel_t(0.4), channel_t(0.7)));

    target.h = channel_t(15);
    D_CV_CHECK(target.h == channel_t(15));
    D_CV_CHECK(src.h == channel_t(300));

    D_CV_CHECK(std::is_copy_constructible<hsv>::value);
    D_CV_CHECK(std::is_copy_assignable<hsv>::value);

    return true;
}

/*
tests_color_hsv_construction_is_constexpr
  Verifies all three constructors are usable in a constant expression, so a
  hsv can be a compile-time constant.
  Tests the following:
  - default, channel and POD-converting construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_hsv_construction_is_constexpr()
{
    D_CONSTEXPR hsv zero;
    D_CONSTEXPR hsv made(channel_t(180), channel_t(0.25), channel_t(0.5));
    D_CONSTEXPR hsv wrapped(
        d_color_hsv_make(channel_t(90), channel_t(1), channel_t(0)));

    static_assert(zero.h == channel_t(0), "default ctor must be constexpr");
    static_assert(made.h == channel_t(180), "channel ctor must be constexpr");
    static_assert(made.v == channel_t(0.5), "channel ctor must be constexpr");
    static_assert(wrapped.s == channel_t(1), "POD ctor must be constexpr");

    D_CV_CHECK(zero.h == channel_t(0));
    D_CV_CHECK(made.s == channel_t(0.25));
    D_CV_CHECK(wrapped.h == channel_t(90));

    return true;
}

/*
tests_color_hsv_model_type_identity
  Verifies the type identity that enrols hsv in the conversion graph.
  Tests the following:
  - model_tag is hsv_tag and derives from color_model_tag
  - value_type is channel_t
  - is_color_model accepts hsv, including through cv-ref qualifiers, while
    the bare POD is not itself a model
*/
bool
tests_color_hsv_model_type_identity()
{
    D_CV_CHECK((std::is_same<hsv::model_tag, hsv_tag>::value));
    D_CV_CHECK((std::is_base_of<color_model_tag, hsv::model_tag>::value));
    D_CV_CHECK((std::is_same<hsv::value_type, channel_t>::value));

    D_CV_CHECK(is_color_model<hsv>::value);
    D_CV_CHECK(is_color_model<const hsv&>::value);
    D_CV_CHECK(is_color_model<hsv&&>::value);

    D_CV_CHECK(!(is_color_model<d_color_hsv>::value));

    return true;
}

NS_END
NS_END
