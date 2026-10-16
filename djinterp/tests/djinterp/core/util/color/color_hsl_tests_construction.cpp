/*******************************************************************************
* djinterp [core]                               color_hsl_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_hsl_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_hsl_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_hsl_default_is_zeroed
  Verifies the default constructor zeroes hue, saturation and lightness.
  Tests the following:
  - all three channels are exactly zero
  - the zeroed value denotes black and is itself canonical
*/
bool
tests_color_hsl_default_is_zeroed()
{
    const hsl v;

    D_CL_CHECK(cl_same(v, channel_t(0), channel_t(0), channel_t(0)));
    D_CL_CHECK(v.h == channel_t(0));
    D_CL_CHECK(v.s == channel_t(0));
    D_CL_CHECK(v.l == channel_t(0));
    D_CL_CHECK(v.is_valid());

    return true;
}

/*
tests_color_hsl_parameterized_stores_channels_verbatim
  Verifies the channel constructor stores its arguments unchanged, imposing no
  range or wrapping policy of its own.
  Tests the following:
  - in-range values land on the matching channel, in order
  - out-of-range hue is NOT wrapped and out-of-unit channels are NOT clamped
*/
bool
tests_color_hsl_parameterized_stores_channels_verbatim()
{
    const hsl v(channel_t(210), channel_t(0.25), channel_t(0.75));

    D_CL_CHECK(cl_same(v, channel_t(210), channel_t(0.25), channel_t(0.75)));

    // argument order is h, s, l - no two channels are transposed
    D_CL_CHECK(v.h != v.s);
    D_CL_CHECK(v.s != v.l);

    const hsl wild(channel_t(720), channel_t(-1), channel_t(3));
    D_CL_CHECK(cl_same(wild, channel_t(720), channel_t(-1), channel_t(3)));
    D_CL_CHECK(!wild.is_valid());

    return true;
}

/*
tests_color_hsl_converting_constructor_wraps_pod
  Verifies a kernel-produced d_color_hsl implicitly wraps into a hsl with
  identical channels - the path every conversion result takes.
  Tests the following:
  - explicit construction from a POD copies all three channels
  - the conversion is implicit (copy-initialisation works)
*/
bool
tests_color_hsl_converting_constructor_wraps_pod()
{
    const d_color_hsl pod =
        d_color_hsl_make(channel_t(120), channel_t(0.5), channel_t(0.5));

    const hsl direct(pod);
    D_CL_CHECK(cl_same(direct, channel_t(120), channel_t(0.5), channel_t(0.5)));

    const hsl implicit = pod;
    D_CL_CHECK(cl_same(implicit, channel_t(120), channel_t(0.5), channel_t(0.5)));

    D_CL_CHECK((std::is_convertible<d_color_hsl, hsl>::value));

    return true;
}

/*
tests_color_hsl_slices_back_to_pod
  Verifies a hsl slices to its d_color_hsl base, so a wrapper can be handed
  straight to a kernel routine expecting the POD by value.
  Tests the following:
  - binding to a base reference exposes the same channels
  - copying to a base value preserves them
  - a kernel routine accepts the wrapper directly
*/
bool
tests_color_hsl_slices_back_to_pod()
{
    const hsl v(channel_t(45), channel_t(0.25), channel_t(0.6));

    const d_color_hsl& ref = v;
    D_CL_CHECK(ref.h == channel_t(45));
    D_CL_CHECK(ref.s == channel_t(0.25));
    D_CL_CHECK(ref.l == channel_t(0.6));

    const d_color_hsl copy = v;
    D_CL_CHECK(copy.h == channel_t(45));
    D_CL_CHECK(copy.l == channel_t(0.6));

    D_CL_CHECK(d_color_hsl_is_valid(v));

    return true;
}

/*
tests_color_hsl_adds_no_state_to_the_pod
  Verifies hsl derives publicly from d_color_hsl and adds no members, so the
  wrapper is a pure ergonomics layer over the shared kernel type.
  Tests the following:
  - public derivation from the POD
  - identical size and alignment, standard layout preserved
  - trivially copyable and destructible, but not trivially default
    constructible (the default constructor zeroes the channels)
*/
bool
tests_color_hsl_adds_no_state_to_the_pod()
{
    D_CL_CHECK((std::is_base_of<d_color_hsl, hsl>::value));
    D_CL_CHECK((std::is_convertible<hsl*, d_color_hsl*>::value));

    D_CL_CHECK(sizeof(hsl) == sizeof(d_color_hsl));
    D_CL_CHECK(sizeof(hsl) == 3u * sizeof(channel_t));
    D_CL_CHECK(alignof(hsl) == alignof(d_color_hsl));

    D_CL_CHECK(std::is_standard_layout<hsl>::value);
    D_CL_CHECK(std::is_trivially_copyable<hsl>::value);
    D_CL_CHECK(std::is_trivially_destructible<hsl>::value);
    D_CL_CHECK(!std::is_polymorphic<hsl>::value);
    D_CL_CHECK(!std::is_trivially_default_constructible<hsl>::value);

    return true;
}

/*
tests_color_hsl_copy_and_assignment
  Verifies copy construction and copy assignment reproduce every channel.
  Tests the following:
  - a copy carries the same channels and is independent of its source
  - assignment overwrites all three channels
*/
bool
tests_color_hsl_copy_and_assignment()
{
    const hsl src(channel_t(300), channel_t(0.4), channel_t(0.7));

    const hsl copied(src);
    D_CL_CHECK(cl_same(copied, channel_t(300), channel_t(0.4), channel_t(0.7)));

    hsl target;
    target = src;
    D_CL_CHECK(cl_same(target, channel_t(300), channel_t(0.4), channel_t(0.7)));

    target.h = channel_t(15);
    D_CL_CHECK(target.h == channel_t(15));
    D_CL_CHECK(src.h == channel_t(300));

    D_CL_CHECK(std::is_copy_constructible<hsl>::value);
    D_CL_CHECK(std::is_copy_assignable<hsl>::value);

    return true;
}

/*
tests_color_hsl_construction_is_constexpr
  Verifies all three constructors are usable in a constant expression, so a
  hsl can be a compile-time constant.
  Tests the following:
  - default, channel and POD-converting construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_hsl_construction_is_constexpr()
{
    D_CONSTEXPR hsl zero;
    D_CONSTEXPR hsl made(channel_t(180), channel_t(0.25), channel_t(0.5));
    D_CONSTEXPR hsl wrapped(
        d_color_hsl_make(channel_t(90), channel_t(1), channel_t(0)));

    static_assert(zero.h == channel_t(0), "default ctor must be constexpr");
    static_assert(made.h == channel_t(180), "channel ctor must be constexpr");
    static_assert(made.l == channel_t(0.5), "channel ctor must be constexpr");
    static_assert(wrapped.s == channel_t(1), "POD ctor must be constexpr");

    D_CL_CHECK(zero.h == channel_t(0));
    D_CL_CHECK(made.s == channel_t(0.25));
    D_CL_CHECK(wrapped.h == channel_t(90));

    return true;
}

/*
tests_color_hsl_model_type_identity
  Verifies the type identity that enrols hsl in the conversion graph.
  Tests the following:
  - model_tag is hsl_tag and derives from color_model_tag
  - value_type is channel_t
  - is_color_model accepts hsl, including through cv-ref qualifiers, while
    the bare POD is not itself a model
*/
bool
tests_color_hsl_model_type_identity()
{
    D_CL_CHECK((std::is_same<hsl::model_tag, hsl_tag>::value));
    D_CL_CHECK((std::is_base_of<color_model_tag, hsl::model_tag>::value));
    D_CL_CHECK((std::is_same<hsl::value_type, channel_t>::value));

    D_CL_CHECK(is_color_model<hsl>::value);
    D_CL_CHECK(is_color_model<const hsl&>::value);
    D_CL_CHECK(is_color_model<hsl&&>::value);

    D_CL_CHECK(!(is_color_model<d_color_hsl>::value));

    return true;
}

NS_END
NS_END
