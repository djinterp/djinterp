/*******************************************************************************
* djinterp [core]                             color_ycbcr_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_ycbcr_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_ycbcr_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_ycbcr_default_is_zeroed
  Verifies the default constructor zeroes luma and both chroma channels.
  Tests the following:
  - y, cb and cr are each exactly zero
  - the zeroed value is itself in the canonical domain
*/
bool
tests_color_ycbcr_default_is_zeroed()
{
    const ycbcr v;

    D_CY_CHECK(cy_same(v, channel_t(0), channel_t(0), channel_t(0)));
    D_CY_CHECK(v.y == channel_t(0));
    D_CY_CHECK(v.cb == channel_t(0));
    D_CY_CHECK(v.cr == channel_t(0));
    D_CY_CHECK(v.is_valid());

    return true;
}

/*
tests_color_ycbcr_parameterized_stores_channels_verbatim
  Verifies the channel constructor stores its arguments unchanged, imposing no
  range policy of its own.
  Tests the following:
  - in-domain values land on the matching channel, in order
  - out-of-domain values are stored as given, not clamped
*/
bool
tests_color_ycbcr_parameterized_stores_channels_verbatim()
{
    const ycbcr v(channel_t(0.75), channel_t(-0.25), channel_t(0.5));

    D_CY_CHECK(cy_same(v, channel_t(0.75), channel_t(-0.25), channel_t(0.5)));

    // argument order is y, cb, cr - no two channels are transposed
    D_CY_CHECK(v.y != v.cb);
    D_CY_CHECK(v.cb != v.cr);

    const ycbcr wild(channel_t(-3), channel_t(9), channel_t(-9));
    D_CY_CHECK(cy_same(wild, channel_t(-3), channel_t(9), channel_t(-9)));
    D_CY_CHECK(!wild.is_valid());

    return true;
}

/*
tests_color_ycbcr_converting_constructor_wraps_pod
  Verifies a kernel-produced d_color_ycbcr implicitly wraps into a ycbcr with
  identical channels - the path every conversion result takes.
  Tests the following:
  - explicit construction from a POD copies all three channels
  - the conversion is implicit (copy-initialisation works)
*/
bool
tests_color_ycbcr_converting_constructor_wraps_pod()
{
    const d_color_ycbcr pod =
        d_color_ycbcr_make(channel_t(0.5), channel_t(0.25), channel_t(-0.25));

    const ycbcr direct(pod);
    D_CY_CHECK(cy_same(direct, channel_t(0.5), channel_t(0.25),
                               channel_t(-0.25)));

    const ycbcr implicit = pod;
    D_CY_CHECK(cy_same(implicit, channel_t(0.5), channel_t(0.25),
                                 channel_t(-0.25)));

    D_CY_CHECK((std::is_convertible<d_color_ycbcr, ycbcr>::value));

    return true;
}

/*
tests_color_ycbcr_slices_back_to_pod
  Verifies a ycbcr slices to its d_color_ycbcr base, so a wrapper can be
  handed straight to a kernel routine expecting the POD by value.
  Tests the following:
  - binding to a base reference exposes the same channels
  - copying to a base value preserves them
  - a kernel routine accepts the wrapper directly
*/
bool
tests_color_ycbcr_slices_back_to_pod()
{
    const ycbcr v(channel_t(0.6), channel_t(-0.4), channel_t(0.2));

    const d_color_ycbcr& ref = v;
    D_CY_CHECK(ref.y == channel_t(0.6));
    D_CY_CHECK(ref.cb == channel_t(-0.4));
    D_CY_CHECK(ref.cr == channel_t(0.2));

    const d_color_ycbcr copy = v;
    D_CY_CHECK(copy.y == channel_t(0.6));
    D_CY_CHECK(copy.cr == channel_t(0.2));

    D_CY_CHECK(d_color_ycbcr_is_valid(v));

    return true;
}

/*
tests_color_ycbcr_adds_no_state_to_the_pod
  Verifies ycbcr derives publicly from d_color_ycbcr and adds no members, so
  the wrapper is a pure ergonomics layer over the shared kernel type.
  Tests the following:
  - public derivation from the POD
  - identical size and alignment, standard layout preserved
  - trivially copyable and destructible, but not trivially default
    constructible
*/
bool
tests_color_ycbcr_adds_no_state_to_the_pod()
{
    D_CY_CHECK((std::is_base_of<d_color_ycbcr, ycbcr>::value));
    D_CY_CHECK((std::is_convertible<ycbcr*, d_color_ycbcr*>::value));

    D_CY_CHECK(sizeof(ycbcr) == sizeof(d_color_ycbcr));
    D_CY_CHECK(sizeof(ycbcr) == 3u * sizeof(channel_t));
    D_CY_CHECK(alignof(ycbcr) == alignof(d_color_ycbcr));

    D_CY_CHECK(std::is_standard_layout<ycbcr>::value);
    D_CY_CHECK(std::is_trivially_copyable<ycbcr>::value);
    D_CY_CHECK(std::is_trivially_destructible<ycbcr>::value);
    D_CY_CHECK(!std::is_polymorphic<ycbcr>::value);
    D_CY_CHECK(!std::is_trivially_default_constructible<ycbcr>::value);

    return true;
}

/*
tests_color_ycbcr_copy_and_assignment
  Verifies copy construction and copy assignment reproduce every channel.
  Tests the following:
  - a copy carries the same channels and is independent of its source
  - assignment overwrites all three channels
*/
bool
tests_color_ycbcr_copy_and_assignment()
{
    const ycbcr src(channel_t(0.3), channel_t(0.1), channel_t(-0.2));

    const ycbcr copied(src);
    D_CY_CHECK(cy_same(copied, channel_t(0.3), channel_t(0.1),
                               channel_t(-0.2)));

    ycbcr target;
    target = src;
    D_CY_CHECK(cy_same(target, channel_t(0.3), channel_t(0.1),
                               channel_t(-0.2)));

    target.y = channel_t(0.9);
    D_CY_CHECK(target.y == channel_t(0.9));
    D_CY_CHECK(src.y == channel_t(0.3));

    D_CY_CHECK(std::is_copy_constructible<ycbcr>::value);
    D_CY_CHECK(std::is_copy_assignable<ycbcr>::value);

    return true;
}

/*
tests_color_ycbcr_construction_is_constexpr
  Verifies all three constructors are usable in a constant expression.
  Tests the following:
  - default, channel and POD-converting construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_ycbcr_construction_is_constexpr()
{
    D_CONSTEXPR ycbcr zero;
    D_CONSTEXPR ycbcr made(channel_t(0.5), channel_t(-0.5), channel_t(0.5));
    D_CONSTEXPR ycbcr wrapped(
        d_color_ycbcr_make(channel_t(1), channel_t(0), channel_t(0)));

    static_assert(zero.y == channel_t(0), "default ctor must be constexpr");
    static_assert(made.cb == channel_t(-0.5), "channel ctor must be constexpr");
    static_assert(made.cr == channel_t(0.5), "channel ctor must be constexpr");
    static_assert(wrapped.y == channel_t(1), "POD ctor must be constexpr");

    D_CY_CHECK(zero.y == channel_t(0));
    D_CY_CHECK(made.cb == channel_t(-0.5));
    D_CY_CHECK(wrapped.y == channel_t(1));

    return true;
}

/*
tests_color_ycbcr_model_type_identity
  Verifies the type identity that enrols ycbcr in the conversion graph.
  Tests the following:
  - model_tag is ycbcr_tag and derives from color_model_tag
  - value_type is channel_t
  - is_color_model accepts ycbcr through cv-ref qualifiers; the bare POD is
    not itself a model
*/
bool
tests_color_ycbcr_model_type_identity()
{
    D_CY_CHECK((std::is_same<ycbcr::model_tag, ycbcr_tag>::value));
    D_CY_CHECK((std::is_base_of<color_model_tag, ycbcr::model_tag>::value));
    D_CY_CHECK((std::is_same<ycbcr::value_type, channel_t>::value));

    D_CY_CHECK(is_color_model<ycbcr>::value);
    D_CY_CHECK(is_color_model<const ycbcr&>::value);
    D_CY_CHECK(is_color_model<ycbcr&&>::value);

    D_CY_CHECK(!(is_color_model<d_color_ycbcr>::value));

    return true;
}

NS_END
NS_END
