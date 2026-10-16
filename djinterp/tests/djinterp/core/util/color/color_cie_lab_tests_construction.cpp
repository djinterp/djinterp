/*******************************************************************************
* djinterp [core]                           color_cie_lab_tests_construction.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_cie_lab_tests_construction.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_cie_lab_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_cie_xyz_default_is_zeroed
  Verifies the cie_xyz default constructor zeroes all three tristimulus
  values.
  Tests the following:
  - X, Y and Z are each exactly zero
  - the zeroed tristimulus is itself valid (the orthant is closed at zero)
*/
bool
tests_color_cie_xyz_default_is_zeroed()
{
    const cie_xyz v;

    D_CE_CHECK(ce_xyz_same(v, channel_t(0), channel_t(0), channel_t(0)));
    D_CE_CHECK(v.x == channel_t(0));
    D_CE_CHECK(v.y == channel_t(0));
    D_CE_CHECK(v.z == channel_t(0));
    D_CE_CHECK(v.is_valid());

    return true;
}

/*
tests_color_cie_xyz_parameterized_stores_channels_verbatim
  Verifies the cie_xyz channel constructor stores its arguments unchanged,
  imposing no range policy of its own.
  Tests the following:
  - values land on the matching axis, in X, Y, Z order
  - a negative axis is stored as given rather than lifted to zero
*/
bool
tests_color_cie_xyz_parameterized_stores_channels_verbatim()
{
    const cie_xyz v(channel_t(0.95047), channel_t(1), channel_t(1.08883));

    D_CE_CHECK(ce_xyz_same(v, channel_t(0.95047), channel_t(1),
                              channel_t(1.08883)));

    // argument order is x, y, z - no two axes are transposed
    D_CE_CHECK(v.x != v.y);
    D_CE_CHECK(v.y != v.z);

    const cie_xyz negative(channel_t(-1), channel_t(-2), channel_t(-3));
    D_CE_CHECK(ce_xyz_same(negative, channel_t(-1), channel_t(-2),
                                     channel_t(-3)));
    D_CE_CHECK(!negative.is_valid());

    return true;
}

/*
tests_color_cie_xyz_converting_constructor_wraps_pod
  Verifies a kernel-produced d_color_xyz implicitly wraps into a cie_xyz with
  identical channels - the path every conversion result takes.
  Tests the following:
  - explicit construction from a POD copies all three axes
  - the conversion is implicit, and the wrapper slices back to the POD
*/
bool
tests_color_cie_xyz_converting_constructor_wraps_pod()
{
    const d_color_xyz pod =
        d_color_xyz_make(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    const cie_xyz direct(pod);
    D_CE_CHECK(ce_xyz_same(direct, channel_t(0.25), channel_t(0.5),
                                   channel_t(0.75)));

    const cie_xyz implicit = pod;
    D_CE_CHECK(ce_xyz_same(implicit, channel_t(0.25), channel_t(0.5),
                                     channel_t(0.75)));

    D_CE_CHECK((std::is_convertible<d_color_xyz, cie_xyz>::value));

    const d_color_xyz& sliced = direct;
    D_CE_CHECK(sliced.y == channel_t(0.5));
    D_CE_CHECK(d_color_xyz_is_valid(direct));

    return true;
}

/*
tests_color_cie_xyz_adds_no_state_to_the_pod
  Verifies cie_xyz derives publicly from d_color_xyz and adds no members, so
  the wrapper is a pure ergonomics layer over the shared kernel type.
  Tests the following:
  - public derivation from the POD
  - identical size and alignment, standard layout preserved
  - trivially copyable and destructible, but not trivially default
    constructible
*/
bool
tests_color_cie_xyz_adds_no_state_to_the_pod()
{
    D_CE_CHECK((std::is_base_of<d_color_xyz, cie_xyz>::value));
    D_CE_CHECK((std::is_convertible<cie_xyz*, d_color_xyz*>::value));

    D_CE_CHECK(sizeof(cie_xyz) == sizeof(d_color_xyz));
    D_CE_CHECK(sizeof(cie_xyz) == 3u * sizeof(channel_t));
    D_CE_CHECK(alignof(cie_xyz) == alignof(d_color_xyz));

    D_CE_CHECK(std::is_standard_layout<cie_xyz>::value);
    D_CE_CHECK(std::is_trivially_copyable<cie_xyz>::value);
    D_CE_CHECK(std::is_trivially_destructible<cie_xyz>::value);
    D_CE_CHECK(!std::is_polymorphic<cie_xyz>::value);
    D_CE_CHECK(!std::is_trivially_default_constructible<cie_xyz>::value);

    return true;
}

/*
tests_color_cie_xyz_construction_is_constexpr
  Verifies every cie_xyz constructor is usable in a constant expression.
  Tests the following:
  - default, channel and POD-converting construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_cie_xyz_construction_is_constexpr()
{
    D_CONSTEXPR cie_xyz zero;
    D_CONSTEXPR cie_xyz made(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    D_CONSTEXPR cie_xyz wrapped(
        d_color_xyz_make(channel_t(1), channel_t(2), channel_t(3)));

    static_assert(zero.x == channel_t(0), "default ctor must be constexpr");
    static_assert(made.y == channel_t(0.5), "channel ctor must be constexpr");
    static_assert(wrapped.z == channel_t(3), "POD ctor must be constexpr");

    D_CE_CHECK(zero.x == channel_t(0));
    D_CE_CHECK(made.y == channel_t(0.5));
    D_CE_CHECK(wrapped.z == channel_t(3));

    return true;
}

/*
tests_color_cie_xyz_model_type_identity
  Verifies the type identity that enrols cie_xyz in the conversion graph as
  the device-independent hub.
  Tests the following:
  - model_tag is cie_xyz_tag and derives from color_model_tag
  - value_type is channel_t
  - is_color_model accepts cie_xyz through cv-ref qualifiers; the bare POD is
    not itself a model
*/
bool
tests_color_cie_xyz_model_type_identity()
{
    D_CE_CHECK((std::is_same<cie_xyz::model_tag, cie_xyz_tag>::value));
    D_CE_CHECK((std::is_base_of<color_model_tag, cie_xyz::model_tag>::value));
    D_CE_CHECK((std::is_same<cie_xyz::value_type, channel_t>::value));

    D_CE_CHECK(is_color_model<cie_xyz>::value);
    D_CE_CHECK(is_color_model<const cie_xyz&>::value);
    D_CE_CHECK(is_color_model<cie_xyz&&>::value);

    D_CE_CHECK(!(is_color_model<d_color_xyz>::value));

    return true;
}

/*
tests_color_cie_lab_default_is_zeroed
  Verifies the cie_lab default constructor zeroes L*, a* and b*.
  Tests the following:
  - all three axes are exactly zero
  - the zeroed value denotes black and is itself valid
*/
bool
tests_color_cie_lab_default_is_zeroed()
{
    const cie_lab v;

    D_CE_CHECK(ce_lab_same(v, channel_t(0), channel_t(0), channel_t(0)));
    D_CE_CHECK(v.l == channel_t(0));
    D_CE_CHECK(v.a == channel_t(0));
    D_CE_CHECK(v.b == channel_t(0));
    D_CE_CHECK(v.is_valid());

    return true;
}

/*
tests_color_cie_lab_parameterized_stores_channels_verbatim
  Verifies the cie_lab channel constructor stores its arguments unchanged,
  including chroma far outside any displayable gamut.
  Tests the following:
  - values land on the matching axis, in L*, a*, b* order
  - out-of-range lightness and absurd chroma are both stored as given
*/
bool
tests_color_cie_lab_parameterized_stores_channels_verbatim()
{
    const cie_lab v(channel_t(53.2408), channel_t(80.0925), channel_t(67.2032));

    D_CE_CHECK(ce_lab_same(v, channel_t(53.2408), channel_t(80.0925),
                              channel_t(67.2032)));

    // argument order is l, a, b - no two axes are transposed
    D_CE_CHECK(v.l != v.a);
    D_CE_CHECK(v.a != v.b);

    const cie_lab wild(channel_t(-50), channel_t(1000), channel_t(-1000));
    D_CE_CHECK(ce_lab_same(wild, channel_t(-50), channel_t(1000),
                                 channel_t(-1000)));

    return true;
}

/*
tests_color_cie_lab_converting_constructor_wraps_pod
  Verifies a kernel-produced d_color_lab implicitly wraps into a cie_lab with
  identical channels - the path every conversion result takes.
  Tests the following:
  - explicit construction from a POD copies all three axes
  - the conversion is implicit, and the wrapper slices back to the POD
*/
bool
tests_color_cie_lab_converting_constructor_wraps_pod()
{
    const d_color_lab pod =
        d_color_lab_make(channel_t(50), channel_t(25), channel_t(-25));

    const cie_lab direct(pod);
    D_CE_CHECK(ce_lab_same(direct, channel_t(50), channel_t(25),
                                   channel_t(-25)));

    const cie_lab implicit = pod;
    D_CE_CHECK(ce_lab_same(implicit, channel_t(50), channel_t(25),
                                     channel_t(-25)));

    D_CE_CHECK((std::is_convertible<d_color_lab, cie_lab>::value));

    const d_color_lab& sliced = direct;
    D_CE_CHECK(sliced.a == channel_t(25));
    D_CE_CHECK(d_color_lab_is_valid(direct));

    return true;
}

/*
tests_color_cie_lab_adds_no_state_to_the_pod
  Verifies cie_lab derives publicly from d_color_lab and adds no members, so
  the wrapper is a pure ergonomics layer over the shared kernel type.
  Tests the following:
  - public derivation from the POD
  - identical size and alignment, standard layout preserved
  - trivially copyable and destructible, but not trivially default
    constructible
*/
bool
tests_color_cie_lab_adds_no_state_to_the_pod()
{
    D_CE_CHECK((std::is_base_of<d_color_lab, cie_lab>::value));
    D_CE_CHECK((std::is_convertible<cie_lab*, d_color_lab*>::value));

    D_CE_CHECK(sizeof(cie_lab) == sizeof(d_color_lab));
    D_CE_CHECK(sizeof(cie_lab) == 3u * sizeof(channel_t));
    D_CE_CHECK(alignof(cie_lab) == alignof(d_color_lab));

    D_CE_CHECK(std::is_standard_layout<cie_lab>::value);
    D_CE_CHECK(std::is_trivially_copyable<cie_lab>::value);
    D_CE_CHECK(std::is_trivially_destructible<cie_lab>::value);
    D_CE_CHECK(!std::is_polymorphic<cie_lab>::value);
    D_CE_CHECK(!std::is_trivially_default_constructible<cie_lab>::value);

    return true;
}

/*
tests_color_cie_lab_construction_is_constexpr
  Verifies every cie_lab constructor is usable in a constant expression.
  Tests the following:
  - default, channel and POD-converting construction all evaluate at compile
    time, checked by static_assert on the resulting channels
*/
bool
tests_color_cie_lab_construction_is_constexpr()
{
    D_CONSTEXPR cie_lab zero;
    D_CONSTEXPR cie_lab made(channel_t(50), channel_t(25), channel_t(-25));
    D_CONSTEXPR cie_lab wrapped(
        d_color_lab_make(channel_t(100), channel_t(0), channel_t(0)));

    static_assert(zero.l == channel_t(0), "default ctor must be constexpr");
    static_assert(made.a == channel_t(25), "channel ctor must be constexpr");
    static_assert(made.b == channel_t(-25), "channel ctor must be constexpr");
    static_assert(wrapped.l == channel_t(100), "POD ctor must be constexpr");

    D_CE_CHECK(zero.l == channel_t(0));
    D_CE_CHECK(made.a == channel_t(25));
    D_CE_CHECK(wrapped.l == channel_t(100));

    return true;
}

/*
tests_color_cie_lab_model_type_identity
  Verifies the type identity that enrols cie_lab in the conversion graph.
  Tests the following:
  - model_tag is cie_lab_tag and derives from color_model_tag
  - value_type is channel_t
  - is_color_model accepts cie_lab through cv-ref qualifiers; the bare POD is
    not itself a model
*/
bool
tests_color_cie_lab_model_type_identity()
{
    D_CE_CHECK((std::is_same<cie_lab::model_tag, cie_lab_tag>::value));
    D_CE_CHECK((std::is_base_of<color_model_tag, cie_lab::model_tag>::value));
    D_CE_CHECK((std::is_same<cie_lab::value_type, channel_t>::value));

    D_CE_CHECK(is_color_model<cie_lab>::value);
    D_CE_CHECK(is_color_model<const cie_lab&>::value);
    D_CE_CHECK(is_color_model<cie_lab&&>::value);

    D_CE_CHECK(!(is_color_model<d_color_lab>::value));

    return true;
}

/*
tests_color_cie_models_are_distinct_types
  Verifies cie_xyz and cie_lab are unrelated types carrying different tags, so
  neither converts silently to the other despite their identical shape.  The
  two are connected only through the conversion graph.
  Tests the following:
  - the types are not the same and neither derives from the other
  - their model tags differ, though both derive from color_model_tag
  - neither POD cross-converts into the other wrapper
*/
bool
tests_color_cie_models_are_distinct_types()
{
    D_CE_CHECK(!(std::is_same<cie_xyz, cie_lab>::value));
    D_CE_CHECK(!(std::is_base_of<cie_xyz, cie_lab>::value));
    D_CE_CHECK(!(std::is_base_of<cie_lab, cie_xyz>::value));

    D_CE_CHECK(!(std::is_same<cie_xyz::model_tag, cie_lab::model_tag>::value));
    D_CE_CHECK((std::is_base_of<color_model_tag, cie_xyz::model_tag>::value));
    D_CE_CHECK((std::is_base_of<color_model_tag, cie_lab::model_tag>::value));

    D_CE_CHECK(!(std::is_convertible<cie_xyz, cie_lab>::value));
    D_CE_CHECK(!(std::is_convertible<cie_lab, cie_xyz>::value));
    D_CE_CHECK(!(std::is_convertible<d_color_xyz, cie_lab>::value));
    D_CE_CHECK(!(std::is_convertible<d_color_lab, cie_xyz>::value));

    // both are nonetheless models, and identically shaped
    D_CE_CHECK(is_color_model<cie_xyz>::value);
    D_CE_CHECK(is_color_model<cie_lab>::value);
    D_CE_CHECK(sizeof(cie_xyz) == sizeof(cie_lab));

    return true;
}

NS_END
NS_END
