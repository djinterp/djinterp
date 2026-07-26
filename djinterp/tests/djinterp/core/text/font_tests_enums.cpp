// djinterp [test]  font_tests_enums.cpp
//   Section 2: font_weight, font_slant, font_stretch, font_spacing,
// font_size_unit.  These enums are wire values - they mirror OS/2
// usWeightClass and usWidthClass - so their numbers are a contract with
// every backend, not an internal detail.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_font_weight_underlying_type
  Tests the following:
  - font_weight is backed by std::uint16_t (it must hold 950)
  - it is a scoped enum
*/
bool
tests_font_weight_underlying_type()
{
    static_assert(std::is_same<std::underlying_type<font_weight>::type,
                               std::uint16_t>::value,
                  "font_weight must be backed by uint16_t");

    D_FT_CHECK(std::is_enum<font_weight>::value);
    D_FT_CHECK(!std::is_convertible<font_weight, int>::value);

    return true;
}

/*
tests_font_weight_values
  The numbers ARE the OS/2 usWeightClass / CSS font-weight scale; a renderer
casts the enum straight through.
  Tests the following:
  - all ten symbolic weights against their standard numeric values
  - extra_black is 950, not 1000 (the OS/2 tail value)
*/
bool
tests_font_weight_values()
{
    D_FT_CHECK(weight_of(font_weight::thin)        == 100u);
    D_FT_CHECK(weight_of(font_weight::extra_light) == 200u);
    D_FT_CHECK(weight_of(font_weight::light)       == 300u);
    D_FT_CHECK(weight_of(font_weight::normal)      == 400u);
    D_FT_CHECK(weight_of(font_weight::medium)      == 500u);
    D_FT_CHECK(weight_of(font_weight::semi_bold)   == 600u);
    D_FT_CHECK(weight_of(font_weight::bold)        == 700u);
    D_FT_CHECK(weight_of(font_weight::extra_bold)  == 800u);
    D_FT_CHECK(weight_of(font_weight::black)       == 900u);
    D_FT_CHECK(weight_of(font_weight::extra_black) == 950u);

    return true;
}

/*
tests_font_weight_is_monotonic
  fn_is_bold compares numerically, so the scale must be strictly increasing
in declaration order or the comparison means nothing.
  Tests the following:
  - each weight is strictly greater than its predecessor
  - normal < semi_bold <= bold (the boundary fn_is_bold keys off)
*/
bool
tests_font_weight_is_monotonic()
{
    const std::uint16_t scale[] =
    {
        weight_of(font_weight::thin),        weight_of(font_weight::extra_light),
        weight_of(font_weight::light),       weight_of(font_weight::normal),
        weight_of(font_weight::medium),      weight_of(font_weight::semi_bold),
        weight_of(font_weight::bold),        weight_of(font_weight::extra_bold),
        weight_of(font_weight::black),       weight_of(font_weight::extra_black)
    };

    const std::size_t count = sizeof(scale) / sizeof(scale[0]);

    D_FT_CHECK(count == 10u);

    // strictly increasing
    for (std::size_t i = 1; i < count; ++i)
    {
        D_FT_CHECK(scale[i] > scale[i - 1u]);
    }

    D_FT_CHECK(weight_of(font_weight::normal) <
               weight_of(font_weight::semi_bold));
    D_FT_CHECK(weight_of(font_weight::semi_bold) <
               weight_of(font_weight::bold));

    return true;
}

/*
tests_font_slant_underlying_type
  Tests the following:
  - font_slant is backed by std::uint8_t
  - it is scoped (no implicit int conversion)
*/
bool
tests_font_slant_underlying_type()
{
    static_assert(std::is_same<std::underlying_type<font_slant>::type,
                               std::uint8_t>::value,
                  "font_slant must be backed by uint8_t");

    D_FT_CHECK(!std::is_convertible<font_slant, int>::value);

    return true;
}

/*
tests_font_slant_values
  Tests the following:
  - upright / italic / oblique are 0 / 1 / 2
  - upright is zero, so a zero-initialised slant is upright
  - the three are pairwise distinct
*/
bool
tests_font_slant_values()
{
    D_FT_CHECK(static_cast<std::uint8_t>(font_slant::upright) == 0u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_slant::italic)  == 1u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_slant::oblique) == 2u);

    D_FT_CHECK(font_slant::upright != font_slant::italic);
    D_FT_CHECK(font_slant::italic  != font_slant::oblique);
    D_FT_CHECK(font_slant::upright != font_slant::oblique);

    return true;
}

/*
tests_font_stretch_underlying_type
  Tests the following:
  - font_stretch is backed by std::uint8_t
*/
bool
tests_font_stretch_underlying_type()
{
    static_assert(std::is_same<std::underlying_type<font_stretch>::type,
                               std::uint8_t>::value,
                  "font_stretch must be backed by uint8_t");

    D_FT_CHECK(!std::is_convertible<font_stretch, int>::value);

    return true;
}

/*
tests_font_stretch_values
  The OS/2 usWidthClass range starts at 1, NOT 0 - so a zero-initialised
font_stretch would be an invalid width class.  That is precisely why the
mixin default-initialises it to `normal` rather than leaving it {}.
  Tests the following:
  - ultra_condensed..ultra_expanded are 1..9
  - no member is 0
*/
bool
tests_font_stretch_values()
{
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::ultra_condensed) == 1u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::extra_condensed) == 2u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::condensed)       == 3u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::semi_condensed)  == 4u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::normal)          == 5u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::semi_expanded)   == 6u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::expanded)        == 7u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::extra_expanded)  == 8u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::ultra_expanded)  == 9u);

    // 1-based: zero is not a width class
    D_FT_CHECK(static_cast<std::uint8_t>(font_stretch::ultra_condensed) != 0u);

    return true;
}

/*
tests_font_stretch_normal_is_the_midpoint
  Tests the following:
  - normal (5) sits exactly between ultra_condensed (1) and
    ultra_expanded (9)
  - the scale is strictly increasing
*/
bool
tests_font_stretch_normal_is_the_midpoint()
{
    const std::uint8_t lo  = static_cast<std::uint8_t>(
        font_stretch::ultra_condensed);
    const std::uint8_t mid = static_cast<std::uint8_t>(font_stretch::normal);
    const std::uint8_t hi  = static_cast<std::uint8_t>(
        font_stretch::ultra_expanded);

    D_FT_CHECK((static_cast<unsigned>(lo) + static_cast<unsigned>(hi)) ==
               (2u * static_cast<unsigned>(mid)));

    const std::uint8_t scale[] =
    {
        static_cast<std::uint8_t>(font_stretch::ultra_condensed),
        static_cast<std::uint8_t>(font_stretch::extra_condensed),
        static_cast<std::uint8_t>(font_stretch::condensed),
        static_cast<std::uint8_t>(font_stretch::semi_condensed),
        static_cast<std::uint8_t>(font_stretch::normal),
        static_cast<std::uint8_t>(font_stretch::semi_expanded),
        static_cast<std::uint8_t>(font_stretch::expanded),
        static_cast<std::uint8_t>(font_stretch::extra_expanded),
        static_cast<std::uint8_t>(font_stretch::ultra_expanded)
    };

    // strictly increasing
    for (std::size_t i = 1; i < 9u; ++i)
    {
        D_FT_CHECK(scale[i] > scale[i - 1u]);
    }

    return true;
}

/*
tests_font_spacing_underlying_type
  Tests the following:
  - font_spacing is backed by std::uint8_t
*/
bool
tests_font_spacing_underlying_type()
{
    static_assert(std::is_same<std::underlying_type<font_spacing>::type,
                               std::uint8_t>::value,
                  "font_spacing must be backed by uint8_t");

    D_FT_CHECK(!std::is_convertible<font_spacing, int>::value);

    return true;
}

/*
tests_font_spacing_values
  `any` is zero, which is what makes "don't care" the zero-initialised
default: the mixin's spacing member starts at font_spacing::any and an
adapter reading it will not filter.
  Tests the following:
  - any / proportional / monospace / dual_width / charcell are 0..4
  - any == 0
*/
bool
tests_font_spacing_values()
{
    D_FT_CHECK(static_cast<std::uint8_t>(font_spacing::any)          == 0u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_spacing::proportional) == 1u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_spacing::monospace)    == 2u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_spacing::dual_width)   == 3u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_spacing::charcell)     == 4u);

    return true;
}

/*
tests_font_size_unit_underlying_type
  Tests the following:
  - font_size_unit is backed by std::uint8_t
*/
bool
tests_font_size_unit_underlying_type()
{
    static_assert(std::is_same<std::underlying_type<font_size_unit>::type,
                               std::uint8_t>::value,
                  "font_size_unit must be backed by uint8_t");

    D_FT_CHECK(!std::is_convertible<font_size_unit, int>::value);

    return true;
}

/*
tests_font_size_unit_values
  fn_convert_size switches over every one of these; the switch has no
default label, so a new unit added here without a case would be a silently
unconverted value.
  Tests the following:
  - points / pixels / em / percent / device_units are 0..4
  - points == 0, matching font<>'s size_unit default
  - all five are pairwise distinct
*/
bool
tests_font_size_unit_values()
{
    D_FT_CHECK(static_cast<std::uint8_t>(font_size_unit::points)       == 0u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_size_unit::pixels)       == 1u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_size_unit::em)           == 2u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_size_unit::percent)      == 3u);
    D_FT_CHECK(static_cast<std::uint8_t>(font_size_unit::device_units) == 4u);

    const font_size_unit units[] =
    {
        font_size_unit::points,  font_size_unit::pixels,
        font_size_unit::em,      font_size_unit::percent,
        font_size_unit::device_units
    };

    // pairwise distinct
    for (std::size_t i = 0; i < 5u; ++i)
    {
        for (std::size_t j = (i + 1u); j < 5u; ++j)
        {
            D_FT_CHECK(units[i] != units[j]);
        }
    }

    return true;
}

/*
tests_enums_are_scoped
  All five are `enum class`, so none of them decays to an integer.  That is
what stops a caller passing a raw 700 where a font_weight belongs.
  Tests the following:
  - none of the five converts implicitly to int
  - each is still explicitly castable to its underlying type
*/
bool
tests_enums_are_scoped()
{
    D_FT_CHECK(!std::is_convertible<font_weight,    int>::value);
    D_FT_CHECK(!std::is_convertible<font_slant,     int>::value);
    D_FT_CHECK(!std::is_convertible<font_stretch,   int>::value);
    D_FT_CHECK(!std::is_convertible<font_spacing,   int>::value);
    D_FT_CHECK(!std::is_convertible<font_size_unit, int>::value);

    // but the explicit cast is always available (adapters need it)
    D_FT_CHECK(static_cast<int>(font_weight::bold) == 700);
    D_FT_CHECK(static_cast<int>(font_slant::oblique) == 2);

    // font_feat, by contrast, is deliberately UNSCOPED
    D_FT_CHECK(std::is_convertible<font_feat, unsigned>::value);

    return true;
}

/*
tests_enum_defaults_match_the_model
  The enum values a default-constructed font actually lands on.
  Tests the following:
  - font<>::weight defaults to normal (400)
  - font<>::slant defaults to upright
  - font<>::size_unit defaults to points
  - the stretch mixin defaults to normal (5), NOT to a zero width class
  - the spacing mixin defaults to any (0)
*/
bool
tests_enum_defaults_match_the_model()
{
    const bare_font base;

    D_FT_CHECK(base.weight    == font_weight::normal);
    D_FT_CHECK(base.slant     == font_slant::upright);
    D_FT_CHECK(base.size_unit == font_size_unit::points);

    const axes_font ax;

    D_FT_CHECK(ax.stretch == font_stretch::normal);
    D_FT_CHECK(ax.spacing == font_spacing::any);

    // the stretch default is a VALID width class; zero-init would not be
    D_FT_CHECK(static_cast<std::uint8_t>(ax.stretch) == 5u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
