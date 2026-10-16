/*******************************************************************************
* djinterp [core]                                           color_tests_cast.cpp
*
*
* path:      /tests/djinterp/core/util/color/color_tests_cast.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "color_tests.hpp"

NS_DJINTERP
NS_TESTING

/*
tests_color_umbrella_color_cast_deduces_the_source_type
  Verifies only the target need be named, and that the result matches the
  fully spelled-out call.
  Tests the following:
  - the deduced and explicit spellings agree exactly
  - the return type is the named target
*/
bool
tests_color_umbrella_color_cast_deduces_the_source_type()
{
    const cmyk ink(channel_t(0), channel_t(1), channel_t(1), channel_t(0));

    const rgb deduced   = color_cast<rgb>(ink);
    const rgb explicit_ = color_convert<rgb, cmyk>::apply(ink);

    D_CU_CHECK(deduced == explicit_);
    D_CU_CHECK((std::is_same<decltype(color_cast<rgb>(ink)), rgb>::value));

    const rgb source(channel_t(1), channel_t(0), channel_t(0));
    D_CU_CHECK((std::is_same<decltype(color_cast<cmyk>(source)),
                             cmyk>::value));

    return true;
}

/*
tests_color_umbrella_color_cast_strips_const_and_reference
  Verifies const, reference and const-reference sources all convert
  identically - the point of routing through color_clean before dispatching.
  Tests the following:
  - a mutable value, a const value, a const reference and an rvalue all yield
    the same result
*/
bool
tests_color_umbrella_color_cast_strips_const_and_reference()
{
    cmyk        mutable_source(channel_t(0), channel_t(1), channel_t(1),
                               channel_t(0));
    const cmyk  const_source(channel_t(0), channel_t(1), channel_t(1),
                             channel_t(0));
    const cmyk& ref_source = const_source;

    const rgb from_mutable = color_cast<rgb>(mutable_source);
    const rgb from_const   = color_cast<rgb>(const_source);
    const rgb from_ref     = color_cast<rgb>(ref_source);
    const rgb from_rvalue  = color_cast<rgb>(
        cmyk(channel_t(0), channel_t(1), channel_t(1), channel_t(0)));

    D_CU_CHECK(from_mutable == from_const);
    D_CU_CHECK(from_const == from_ref);
    D_CU_CHECK(from_ref == from_rvalue);

    D_CU_CHECK(cu_rgb_near(from_const, channel_t(1), channel_t(0),
                           channel_t(0)));

    return true;
}

/*
tests_color_umbrella_color_cast_agrees_with_color_convert
  Verifies color_cast adds no arithmetic of its own - it is pure routing over
  the same specialisations.
  Tests the following:
  - both edges and the identity agree bit-for-bit between the two spellings
*/
bool
tests_color_umbrella_color_cast_agrees_with_color_convert()
{
    const cmyk ink(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                   channel_t(0.4));
    const rgb  col(channel_t(0.25), channel_t(0.5), channel_t(0.75));

    D_CU_CHECK(color_cast<rgb>(ink) == color_convert<rgb, cmyk>::apply(ink));
    D_CU_CHECK(color_cast<cmyk>(col) == color_convert<cmyk, rgb>::apply(col));
    D_CU_CHECK(color_cast<rgb>(col) == color_convert<rgb, rgb>::apply(col));

    return true;
}

/*
tests_color_umbrella_color_clean_strips_cv_and_reference
  Verifies the color_clean helper reduces every cv-ref spelling of a type to
  the bare type, which is what lets color_cast accept references and const
  models without a specialisation for each.
  Tests the following:
  - const, volatile, lvalue-reference, rvalue-reference and combinations all
    reduce to the bare type
  - a bare type is left alone
*/
bool
tests_color_umbrella_color_clean_strips_cv_and_reference()
{
    D_CU_CHECK((std::is_same<internal::color_clean<rgb>::type, rgb>::value));
    D_CU_CHECK((std::is_same<internal::color_clean<const rgb>::type,
                             rgb>::value));
    D_CU_CHECK((std::is_same<internal::color_clean<volatile rgb>::type,
                             rgb>::value));
    D_CU_CHECK((std::is_same<internal::color_clean<rgb&>::type, rgb>::value));
    D_CU_CHECK((std::is_same<internal::color_clean<const rgb&>::type,
                             rgb>::value));
    D_CU_CHECK((std::is_same<internal::color_clean<rgb&&>::type, rgb>::value));
    D_CU_CHECK((std::is_same<internal::color_clean<const volatile rgb&>::type,
                             rgb>::value));

    D_CU_CHECK((std::is_same<internal::color_clean<cmyk&>::type,
                             cmyk>::value));

    return true;
}

/*
tests_color_umbrella_color_cast_round_trips_through_both_edges
  Verifies casting out to CMYK and back recovers the original colour within
  tolerance, exercising both declared edges through the public convenience.
  Tests the following:
  - several colours survive the round trip
  - the round trip is close but need not be bit-exact
*/
bool
tests_color_umbrella_color_cast_round_trips_through_both_edges()
{
    const rgb probes[] = {
        rgb(channel_t(1), channel_t(0), channel_t(0)),
        rgb(channel_t(0), channel_t(1), channel_t(0)),
        rgb(channel_t(0), channel_t(0), channel_t(1)),
        rgb(channel_t(1), channel_t(1), channel_t(1)),
        rgb(channel_t(0), channel_t(0), channel_t(0)),
        rgb(channel_t(0.25), channel_t(0.5), channel_t(0.75)),
    };

    std::size_t i = 0;
    for (i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
    {
        const rgb back = color_cast<rgb>(color_cast<cmyk>(probes[i]));

        D_CU_CHECK(cu_rgb_near(back, probes[i].r, probes[i].g, probes[i].b));
    }

    return true;
}

/*
tests_color_umbrella_color_cast_identity_needs_no_edge
  Verifies a same-type cast resolves to the identity and so works for any
  type, including the transport type rgba which has no edge to anything.
  Tests the following:
  - rgb and cmyk cast to themselves exactly
  - rgba casts to itself, alpha intact, despite not being a colour model
*/
bool
tests_color_umbrella_color_cast_identity_needs_no_edge()
{
    const rgb  col(channel_t(0.25), channel_t(0.5), channel_t(0.75));
    const cmyk ink(channel_t(0.1), channel_t(0.2), channel_t(0.3),
                   channel_t(0.4));
    const rgba transport(channel_t(0.25), channel_t(0.5), channel_t(0.75),
                         channel_t(0.5));

    D_CU_CHECK(color_cast<rgb>(col) == col);
    D_CU_CHECK(color_cast<cmyk>(ink) == ink);

    const rgba back = color_cast<rgba>(transport);
    D_CU_CHECK(back == transport);
    D_CU_CHECK(back.a == channel_t(0.5));

    return true;
}

NS_END
NS_END
