// djinterp [test]  font_tests_value_types.cpp
//   Sections 3-6: the plain value types - font_color, the OpenType tag
// packing, opentype_feature, variable_axis, and font_family_info.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

// NOTE ON `!=`
//   font_color declares operator== and nothing else.  Under C++20 the
// compiler rewrites `a != b` as `!(a == b)`, but under C++17 it does NOT -
// so this suite always spells inequality as !(a == b).  That keeps the
// tests compiling on both faces AND documents the asymmetry, which is a
// real constraint on callers, not a stylistic quirk.

/*
tests_font_color_defaults
  Tests the following:
  - r, g, b default to 0
  - a defaults to 255 (opaque), NOT 0 - a value-initialised font_color is a
    visible black, not an invisible one
*/
bool
tests_font_color_defaults()
{
    const font_color c;

    D_FT_CHECK(c.r == 0u);
    D_FT_CHECK(c.g == 0u);
    D_FT_CHECK(c.b == 0u);
    D_FT_CHECK(c.a == 255u);

    // and the same through value-initialisation
    const font_color v {};

    D_FT_CHECK(v.a == 255u);

    return true;
}

/*
tests_font_color_aggregate_init
  Tests the following:
  - font_color is an aggregate, so brace-init fills the channels in order
  - a partial brace-init leaves the REMAINING members at their default,
    which for `a` means 255 rather than 0
*/
bool
tests_font_color_aggregate_init()
{
    D_FT_CHECK(std::is_aggregate<font_color>::value);

    const font_color full { 10u, 20u, 30u, 40u };

    D_FT_CHECK(full.r == 10u);
    D_FT_CHECK(full.g == 20u);
    D_FT_CHECK(full.b == 30u);
    D_FT_CHECK(full.a == 40u);

    // partial init: alpha keeps its default member initialiser
    const font_color rgb_only { 1u, 2u, 3u };

    D_FT_CHECK(rgb_only.r == 1u);
    D_FT_CHECK(rgb_only.g == 2u);
    D_FT_CHECK(rgb_only.b == 3u);
    D_FT_CHECK(rgb_only.a == 255u);

    return true;
}

/*
tests_font_color_equality
  Tests the following:
  - two identically-filled colors compare equal
  - equality is reflexive
  - two default colors compare equal
*/
bool
tests_font_color_equality()
{
    const font_color a { 1u, 2u, 3u, 4u };
    const font_color b { 1u, 2u, 3u, 4u };

    D_FT_CHECK(a == b);
    D_FT_CHECK(b == a);
    D_FT_CHECK(a == a);

    const font_color d1;
    const font_color d2;

    D_FT_CHECK(d1 == d2);

    return true;
}

/*
tests_font_color_inequality_per_channel
  operator== ANDs all four channels; a bug that dropped one term would still
pass a test that only varied a single channel.  So vary each in turn.
  Tests the following:
  - a difference in r alone breaks equality
  - likewise g, b, and a - alpha included
*/
bool
tests_font_color_inequality_per_channel()
{
    const font_color base { 10u, 20u, 30u, 40u };

    const font_color diff_r { 11u, 20u, 30u, 40u };
    const font_color diff_g { 10u, 21u, 30u, 40u };
    const font_color diff_b { 10u, 20u, 31u, 40u };
    const font_color diff_a { 10u, 20u, 30u, 41u };

    D_FT_CHECK(!(base == diff_r));
    D_FT_CHECK(!(base == diff_g));
    D_FT_CHECK(!(base == diff_b));
    D_FT_CHECK(!(base == diff_a));

    return true;
}

/*
tests_font_color_equality_is_constexpr_and_noexcept
  Tests the following:
  - operator== folds in a constant expression
  - it is noexcept
*/
bool
tests_font_color_equality_is_constexpr_and_noexcept()
{
    constexpr font_color a { 1u, 2u, 3u, 4u };
    constexpr font_color b { 1u, 2u, 3u, 4u };
    constexpr font_color c { 9u, 2u, 3u, 4u };

    static_assert((a == b), "font_color::operator== must be constexpr");
    static_assert(!(a == c), "font_color::operator== must be constexpr");
    static_assert(noexcept(a == b), "font_color::operator== must be noexcept");

    D_FT_CHECK(a == b);
    D_FT_CHECK(!(a == c));

    return true;
}

/*
tests_opentype_tag_is_uint32
  Tests the following:
  - opentype_tag is exactly std::uint32_t, so it drops into hb_tag_t /
    DWRITE_FONT_FEATURE_TAG / CTFontFeatureTag with a plain cast
*/
bool
tests_opentype_tag_is_uint32()
{
    static_assert(std::is_same<opentype_tag, std::uint32_t>::value,
                  "opentype_tag must be uint32_t");

    D_FT_CHECK(sizeof(opentype_tag) == 4u);
    D_FT_CHECK((std::is_same<opentype_tag, std::uint32_t>::value));

    return true;
}

/*
tests_ot_tag_packs_big_endian
  The on-disk OpenType layout is big-endian: the FIRST character lands in
the MOST significant byte.  Getting this backwards would produce tags that
look plausible and match nothing.
  Tests the following:
  - character 0 occupies bits 24..31, character 3 bits 0..7
  - each byte can be recovered by shifting back out
*/
bool
tests_ot_tag_packs_big_endian()
{
    const opentype_tag t = ot_tag("abcd");

    D_FT_CHECK(((t >> 24) & 0xFFu) == static_cast<opentype_tag>('a'));
    D_FT_CHECK(((t >> 16) & 0xFFu) == static_cast<opentype_tag>('b'));
    D_FT_CHECK(((t >>  8) & 0xFFu) == static_cast<opentype_tag>('c'));
    D_FT_CHECK(((t      ) & 0xFFu) == static_cast<opentype_tag>('d'));

    // the whole word, spelled out
    D_FT_CHECK(t == 0x61626364u);

    // and the reverse string must NOT collide with it
    D_FT_CHECK(ot_tag("dcba") != t);

    return true;
}

/*
tests_ot_tag_known_tags
  The real tags an adapter will actually pack.
  Tests the following:
  - liga / kern / smcp / ss01 / calt against their canonical words
  - four spaces packs to the all-space tag, not to zero
*/
bool
tests_ot_tag_known_tags()
{
    D_FT_CHECK(ot_tag("liga") == 0x6C696761u);
    D_FT_CHECK(ot_tag("kern") == 0x6B65726Eu);
    D_FT_CHECK(ot_tag("smcp") == 0x736D6370u);
    D_FT_CHECK(ot_tag("ss01") == 0x73733031u);
    D_FT_CHECK(ot_tag("calt") == 0x63616C74u);

    // the all-space tag is 0x20202020 - a real value, distinct from "unset"
    D_FT_CHECK(ot_tag("    ") == 0x20202020u);
    D_FT_CHECK(ot_tag("    ") != 0u);

    return true;
}

/*
tests_ot_tag_is_constexpr_and_noexcept
  Tags are meant to be baked at compile time: `ot_tag("liga")` should cost
nothing at run time.
  Tests the following:
  - ot_tag folds in a constant expression
  - it is usable as a non-type template argument
  - it is noexcept
*/
bool
tests_ot_tag_is_constexpr_and_noexcept()
{
    constexpr opentype_tag liga = ot_tag("liga");

    static_assert((liga == 0x6C696761u), "ot_tag must fold at compile time");
    static_assert(noexcept(ot_tag("liga")), "ot_tag must be noexcept");

    using probe = std::integral_constant<opentype_tag, ot_tag("kern")>;

    D_FT_CHECK(probe::value == 0x6B65726Eu);
    D_FT_CHECK(liga == 0x6C696761u);

    return true;
}

/*
tests_ot_tag_handles_high_bit_bytes
  ot_tag casts each character through `unsigned char` before widening.  On a
platform where `char` is signed - which is most of them - dropping that cast
would sign-extend any byte >= 0x80 and flood the high bits with ones.  This
is the test that would catch its removal.
  Tests the following:
  - a byte of 0xC3 in position 0 packs to 0xC3......, not 0xFFFFFFC3
  - a high byte in the LAST position does not bleed upward either
*/
bool
tests_ot_tag_handles_high_bit_bytes()
{
    constexpr char lead[5] =
    {
        static_cast<char>(0xC3), 'a', 'b', 'c', '\0'
    };

    constexpr opentype_tag t = ot_tag(lead);

    static_assert((t == 0xC3616263u),
                  "ot_tag must widen through unsigned char, not sign-extend");

    D_FT_CHECK(t == 0xC3616263u);

    constexpr char trail[5] =
    {
        'a', 'b', 'c', static_cast<char>(0xFF), '\0'
    };

    constexpr opentype_tag u = ot_tag(trail);

    static_assert((u == 0x616263FFu),
                  "a high trailing byte must not bleed into the upper bits");

    D_FT_CHECK(u == 0x616263FFu);

    return true;
}

/*
tests_ot_tag_space_padding_is_the_callers_job
  A doc/implementation divergence worth stating out loud.  The header's
comment says shorter strings "are padded with space (0x20) in OpenType
convention", but the body unconditionally reads _s[0]..[3] - it does no
padding.  Handing it a 3-character literal would read one past the end of
the array.  So the OpenType convention is real, but the CALLER must spell
the padding: ot_tag("aa  "), never ot_tag("aa").
  Tests the following:
  - an explicitly space-padded tag packs to the OpenType-conventional word
  - a two-letter padded tag is distinct from the four-letter tags
  - the padding sits in the LOW bytes (right-padding, per the convention)
*/
bool
tests_ot_tag_space_padding_is_the_callers_job()
{
    // "aa" in OpenType convention == "aa  "
    const opentype_tag padded = ot_tag("aa  ");

    D_FT_CHECK(padded == 0x61612020u);

    // the pad bytes are the LOW ones
    D_FT_CHECK(((padded      ) & 0xFFu) == 0x20u);
    D_FT_CHECK(((padded >>  8) & 0xFFu) == 0x20u);
    D_FT_CHECK(((padded >> 16) & 0xFFu) == static_cast<opentype_tag>('a'));
    D_FT_CHECK(((padded >> 24) & 0xFFu) == static_cast<opentype_tag>('a'));

    // a three-letter tag, padded by hand
    D_FT_CHECK(ot_tag("dlig") != ot_tag("dli "));

    return true;
}

/*
tests_ot_tag_distinct_tags_are_distinct_values
  The packing must be injective or two different features would collide in
the feature vector, and fn_set_opentype_feature would overwrite the wrong
entry.
  Tests the following:
  - a set of real tags are pairwise distinct
  - tags differing in ONE character differ in the packed word
*/
bool
tests_ot_tag_distinct_tags_are_distinct_values()
{
    const opentype_tag tags[] =
    {
        ot_tag("liga"), ot_tag("kern"), ot_tag("smcp"), ot_tag("ss01"),
        ot_tag("ss02"), ot_tag("calt"), ot_tag("dlig"), ot_tag("wght")
    };

    const std::size_t count = sizeof(tags) / sizeof(tags[0]);

    // pairwise distinct
    for (std::size_t i = 0; i < count; ++i)
    {
        for (std::size_t j = (i + 1u); j < count; ++j)
        {
            D_FT_CHECK(tags[i] != tags[j]);
        }
    }

    // a single-character difference is visible
    D_FT_CHECK(ot_tag("ss01") != ot_tag("ss02"));
    D_FT_CHECK(ot_tag("ss01") != ot_tag("ss11"));

    return true;
}

/*
tests_opentype_feature_defaults
  The default VALUE is 1, not 0: a default-constructed opentype_feature is
an ENABLED feature.  Zero means disabled, so the defaults are chosen so that
`push_back({tag})` turns something on.
  Tests the following:
  - tag defaults to 0
  - value defaults to 1 (enabled), not 0
*/
bool
tests_opentype_feature_defaults()
{
    const opentype_feature f;

    D_FT_CHECK(f.tag   == 0u);
    D_FT_CHECK(f.value == 1u);

    const opentype_feature v {};

    D_FT_CHECK(v.value == 1u);

    return true;
}

/*
tests_opentype_feature_aggregate_init
  Tests the following:
  - the struct is an aggregate; {tag, value} fills in order
  - {tag} alone leaves value at its default of 1
  - an explicit 0 value is representable (a DISABLED entry)
*/
bool
tests_opentype_feature_aggregate_init()
{
    D_FT_CHECK(std::is_aggregate<opentype_feature>::value);

    const opentype_feature both { ot_tag("liga"), 3u };

    D_FT_CHECK(both.tag   == ot_tag("liga"));
    D_FT_CHECK(both.value == 3u);

    const opentype_feature tag_only { ot_tag("kern") };

    D_FT_CHECK(tag_only.tag   == ot_tag("kern"));
    D_FT_CHECK(tag_only.value == 1u);

    const opentype_feature disabled { ot_tag("liga"), 0u };

    D_FT_CHECK(disabled.value == 0u);

    return true;
}

/*
tests_variable_axis_defaults
  Unlike opentype_feature, variable_axis defaults its value to 0.0f - an
axis is a coordinate, not a switch, so there is no "enabled" to default to.
  Tests the following:
  - tag defaults to 0
  - value defaults to 0.0f
*/
bool
tests_variable_axis_defaults()
{
    const variable_axis a;

    D_FT_CHECK(a.tag == 0u);
    D_FT_CHECK_NEAR(a.value, 0.0f);

    const variable_axis v {};

    D_FT_CHECK_NEAR(v.value, 0.0f);

    return true;
}

/*
tests_variable_axis_aggregate_init
  Tests the following:
  - the struct is an aggregate; {tag, value} fills in order
  - the value is a float and carries fractional coordinates
  - the registered axis tags pack as expected
*/
bool
tests_variable_axis_aggregate_init()
{
    D_FT_CHECK(std::is_aggregate<variable_axis>::value);

    const variable_axis wght { ot_tag("wght"), 650.0f };

    D_FT_CHECK(wght.tag == ot_tag("wght"));
    D_FT_CHECK_NEAR(wght.value, 650.0f);

    const variable_axis opsz { ot_tag("opsz"), 14.5f };

    D_FT_CHECK_NEAR(opsz.value, 14.5f);

    const variable_axis tag_only { ot_tag("wdth") };

    D_FT_CHECK_NEAR(tag_only.value, 0.0f);

    return true;
}

/*
tests_font_family_info_defaults
  The catalogue entry a font manager hands back.
  Tests the following:
  - the string members start empty
  - the three vectors start empty
  - is_scalable defaults to TRUE while the other three flags default to false
  - coverage_bits defaults to 0
*/
bool
tests_font_family_info_defaults()
{
    const font_family_info info;

    D_FT_CHECK(info.family.empty());
    D_FT_CHECK(info.foundry.empty());
    D_FT_CHECK(info.styles.empty());
    D_FT_CHECK(info.fixed_sizes.empty());
    D_FT_CHECK(info.writing_systems.empty());

    D_FT_CHECK(info.is_scalable);       // the odd one out
    D_FT_CHECK(!info.is_monospace);
    D_FT_CHECK(!info.is_symbol);
    D_FT_CHECK(!info.is_variable);

    D_FT_CHECK(info.coverage_bits == 0u);

    return true;
}

/*
tests_font_family_info_scalable_is_the_odd_default
  Naming the asymmetry so nobody "tidies" it away: every other flag on
font_family_info defaults false, but is_scalable defaults TRUE.  A
zero-initialised entry would otherwise claim to be a bitmap-only face.
  Tests the following:
  - is_scalable is the ONLY bool member that starts true
  - a default entry with an empty fixed_sizes list is consistent with the
    documented "empty => scalable" contract
*/
bool
tests_font_family_info_scalable_is_the_odd_default()
{
    const font_family_info info;

    D_FT_CHECK(info.is_scalable);
    D_FT_CHECK(!info.is_monospace);
    D_FT_CHECK(!info.is_symbol);
    D_FT_CHECK(!info.is_variable);

    // the documented pairing: no fixed sizes means a scalable outline face
    D_FT_CHECK(info.fixed_sizes.empty());
    D_FT_CHECK(info.is_scalable);

    // a bitmap face is the deliberate opposite: fixed sizes, not scalable
    font_family_info bitmap;
    bitmap.fixed_sizes = { 8.0f, 10.0f, 12.0f };
    bitmap.is_scalable = false;

    D_FT_CHECK(bitmap.fixed_sizes.size() == 3u);
    D_FT_CHECK(!bitmap.is_scalable);

    return true;
}

/*
tests_font_family_info_population
  Tests the following:
  - every member is writable and round-trips its value
  - the vectors hold their contents in insertion order
*/
bool
tests_font_family_info_population()
{
    font_family_info info;

    info.family          = "Inter";
    info.styles          = { "Regular", "Bold", "Italic" };
    info.fixed_sizes     = { 9.0f, 12.0f };
    info.is_scalable     = true;
    info.is_monospace    = false;
    info.is_symbol       = false;
    info.is_variable     = true;
    info.writing_systems = { "Latin", "Cyrillic", "Greek" };
    info.foundry         = "Rasmus Andersson";
    info.coverage_bits   = 0xDEADBEEFu;

    D_FT_CHECK(info.family == "Inter");
    D_FT_CHECK(info.foundry == "Rasmus Andersson");

    D_FT_CHECK(info.styles.size() == 3u);
    D_FT_CHECK(info.styles[0] == "Regular");
    D_FT_CHECK(info.styles[1] == "Bold");
    D_FT_CHECK(info.styles[2] == "Italic");

    D_FT_CHECK(info.fixed_sizes.size() == 2u);
    D_FT_CHECK_NEAR(info.fixed_sizes[0], 9.0f);
    D_FT_CHECK_NEAR(info.fixed_sizes[1], 12.0f);

    D_FT_CHECK(info.writing_systems.size() == 3u);
    D_FT_CHECK(info.writing_systems[2] == "Greek");

    D_FT_CHECK(info.is_variable);
    D_FT_CHECK(info.coverage_bits == 0xDEADBEEFu);

    // copies are independent
    font_family_info copy = info;
    copy.styles.push_back("Black");

    D_FT_CHECK(copy.styles.size() == 4u);
    D_FT_CHECK(info.styles.size() == 3u);

    return true;
}

NS_END  // testing
NS_END  // djinterp
