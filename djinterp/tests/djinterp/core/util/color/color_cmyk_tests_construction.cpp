/*******************************************************************************
* djinterp [core]                              color_cmyk_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cmyk_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cmyk_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cmyk_default_is_zeroed
  Verifies the default constructor zeroes all four channels.
  Tests the following:
  - c, m, y and k are each exactly zero
  - the zero is positive zero
*/
bool
tests_color_cmyk_default_is_zeroed()
{
    const cmyk v;

    D_CK_CHECK(ck_same(v, channel_t(0), channel_t(0),
                          channel_t(0), channel_t(0)));
    D_CK_CHECK(v.c == channel_t(0));
    D_CK_CHECK(v.m == channel_t(0));
    D_CK_CHECK(v.y == channel_t(0));
    D_CK_CHECK(v.k == channel_t(0));

    return true;
}

/*
tests_color_cmyk_parameterized_stores_channels_verbatim
  Verifies the channel constructor stores its arguments unchanged, imposing no
  range policy of its own.
  Tests the following:
  - in-gamut values land on the matching channel, in order
  - negative and above-unity values are stored as given, not clamped
*/
bool
tests_color_cmyk_parameterized_stores_channels_verbatim()
{
    const cmyk v(channel_t(0.125), channel_t(0.25),
                 channel_t(0.5),   channel_t(0.75));

    D_CK_CHECK(ck_same(v, channel_t(0.125), channel_t(0.25),
                          channel_t(0.5),   channel_t(0.75)));

    // argument order is c, m, y, k - no two channels are transposed
    D_CK_CHECK(v.c != v.m);
    D_CK_CHECK(v.m != v.y);
    D_CK_CHECK(v.y != v.k);

    const cmyk wild(channel_t(-1), channel_t(2),
                    channel_t(-0.5), channel_t(1000));
    D_CK_CHECK(ck_same(wild, channel_t(-1), channel_t(2),
                             channel_t(-0.5), channel_t(1000)));

    return true;
}

/*
tests_color_cmyk_converting_constructor_wraps_pod
  Verifies a kernel-produced d_color_cmyk implicitly wraps into a cmyk with
  identical channels - the path every conversion result takes.
  Tests the following:
  - explicit construction from a POD copies all four channels
  - the conversion is implicit (copy-initialisation and by-value passing work)
*/
bool
tests_color_cmyk_converting_constructor_wraps_pod()
{
    const d_color_cmyk pod = d_color_cmyk_make(channel_t(0.1), channel_t(0.2),
                                               channel_t(0.3), channel_t(0.4));

    const cmyk direct(pod);
    D_CK_CHECK(ck_same(direct, channel_t(0.1), channel_t(0.2),
                               channel_t(0.3), channel_t(0.4)));

    // implicit: copy-initialisation from the POD
    const cmyk implicit = pod;
    D_CK_CHECK(ck_same(implicit, channel_t(0.1), channel_t(0.2),
                                 channel_t(0.3), channel_t(0.4)));

    D_CK_CHECK((std::is_convertible<d_color_cmyk, cmyk>::value));

    return true;
}

/*
tests_color_cmyk_slices_back_to_pod
  Verifies a cmyk slices to its d_color_cmyk base, so a wrapper can be handed
  straight to a kernel routine expecting the POD by value.
  Tests the following:
  - binding to a base reference exposes the same channels
  - copying to a base value preserves them
  - a kernel routine accepts the wrapper directly
*/
bool
tests_color_cmyk_slices_back_to_pod()
{
    const cmyk v(channel_t(0.2), channel_t(0.4),
                 channel_t(0.6), channel_t(0.8));

    const d_color_cmyk& ref = v;
    D_CK_CHECK(ref.c == channel_t(0.2));
    D_CK_CHECK(ref.m == channel_t(0.4));
    D_CK_CHECK(ref.y == channel_t(0.6));
    D_CK_CHECK(ref.k == channel_t(0.8));

    const d_color_cmyk copy = v;
    D_CK_CHECK(copy.c == channel_t(0.2));
    D_CK_CHECK(copy.k == channel_t(0.8));

    // the kernel takes the POD by value and accepts the wrapper unchanged
    D_CK_CHECK(d_color_cmyk_is_valid(v));

    return true;
}

/*
tests_color_cmyk_adds_no_state_to_the_pod
  Verifies cmyk derives publicly from d_color_cmyk and adds no members, so the
  wrapper is a pure ergonomics layer over the shared kernel type.
  Tests the following:
  - public derivation from the POD
  - identical size, and standard layout preserved
  - trivially copyable and destructible; not trivially default constructible
    (the default constructor is user-provided and zeroes the channels)
*/
bool
tests_color_cmyk_adds_no_state_to_the_pod()
{
    D_CK_CHECK((std::is_base_of<d_color_cmyk, cmyk>::value));
    D_CK_CHECK((std::is_convertible<cmyk*, d_color_cmyk*>::value));

    D_CK_CHECK(sizeof(cmyk) == sizeof(d_color_cmyk));
    D_CK_CHECK(sizeof(cmyk) == 4u * sizeof(channel_t));
    D_CK_CHECK(alignof(cmyk) == alignof(d_color_cmyk));

    D_CK_CHECK(std::is_standard_layout<cmyk>::value);
    D_CK_CHECK(std::is_trivially_copyable<cmyk>::value);
    D_CK_CHECK(std::is_trivially_destructible<cmyk>::value);
    D_CK_CHECK(!std::is_polymorphic<cmyk>::value);
    D_CK_CHECK(!std::is_trivially_default_constructible<cmyk>::value);

    return true;
}

/*
tests_color_cmyk_copy_and_assignment
  Verifies copy construction and copy assignment reproduce every channel.
  Tests the following:
  - a copy compares equal to its source and is independent of it
  - assignment overwrites all four channels
*/
bool
tests_color_cmyk_copy_and_assignment()
{
    const cmyk src(channel_t(0.11), channel_t(0.22),
                   channel_t(0.33), channel_t(0.44));

    const cmyk copied(src);
    D_CK_CHECK(ck_same(copied, channel_t(0.11), channel_t(0.22),
                               channel_t(0.33), channel_t(0.44)));

    cmyk target;
    target = src;
    D_CK_CHECK(ck_same(target, channel_t(0.11), channel_t(0.22),
                               channel_t(0.33), channel_t(0.44)));

    // the copy is independent: mutating it leaves the source alone
    target.c = channel_t(0.99);
    D_CK_CHECK(target.c == channel_t(0.99));
    D_CK_CHECK(src.c == channel_t(0.11));

    D_CK_CHECK(std::is_copy_constructible<cmyk>::value);
    D_CK_CHECK(std::is_copy_assignable<cmyk>::value);

    return true;
}

/*
tests_color_cmyk_construction_is_constexpr
  Verifies all three constructors are usable in a constant expression, so a
  cmyk can be a compile-time constant.
  Tests the following:
  - default, channel and POD-converting construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_cmyk_construction_is_constexpr()
{
    D_CONSTEXPR cmyk zero;
    D_CONSTEXPR cmyk made(channel_t(0.25), channel_t(0.5),
                          channel_t(0.75), channel_t(1));
    D_CONSTEXPR cmyk wrapped(d_color_cmyk_make(channel_t(0.5), channel_t(0.5),
                                               channel_t(0.5), channel_t(0.5)));

    static_assert(zero.c == channel_t(0), "default ctor must be constexpr");
    static_assert(zero.k == channel_t(0), "default ctor must be constexpr");
    static_assert(made.c == channel_t(0.25), "channel ctor must be constexpr");
    static_assert(made.k == channel_t(1), "channel ctor must be constexpr");
    static_assert(wrapped.y == channel_t(0.5), "POD ctor must be constexpr");

    D_CK_CHECK(zero.c == channel_t(0));
    D_CK_CHECK(made.m == channel_t(0.5));
    D_CK_CHECK(wrapped.k == channel_t(0.5));

    return true;
}

/*
tests_color_cmyk_model_type_identity
  Verifies the type identity that enrols cmyk in the conversion graph.
  Tests the following:
  - model_tag is cmyk_tag and derives from color_model_tag
  - value_type is channel_t
  - is_color_model accepts cmyk, including through cv-ref qualifiers
*/
bool
tests_color_cmyk_model_type_identity()
{
    D_CK_CHECK((std::is_same<cmyk::model_tag, cmyk_tag>::value));
    D_CK_CHECK((std::is_base_of<color_model_tag, cmyk::model_tag>::value));
    D_CK_CHECK((std::is_same<cmyk::value_type, channel_t>::value));

    D_CK_CHECK(is_color_model<cmyk>::value);
    D_CK_CHECK(is_color_model<const cmyk&>::value);
    D_CK_CHECK(is_color_model<cmyk&&>::value);

    // the underlying POD is NOT itself a model - the tag lives on the wrapper
    D_CK_CHECK(!(is_color_model<d_color_cmyk>::value));

    return true;
}

NS_END
NS_END
