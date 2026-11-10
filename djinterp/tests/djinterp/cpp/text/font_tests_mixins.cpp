/*******************************************************************************
* djinterp [cpp]                                           font_tests_mixins.cpp
*
*
* path:      /tests/djinterp/cpp/text/font_tests_mixins.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  font_tests_mixins.cpp
//   Section 7: the font_mixin EBO layer.  Every optional capability is a
// template <bool _Enable> struct whose false face is empty; the header's
// central promise - "disabled capabilities consume zero bytes" - lives or
// dies here.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

// A NOTE ON HOW ZERO-COST IS MEASURED
//   The tempting test - compare sizeof(DFont<ff_none>) against a hand-rolled
// struct holding only the core members - is NOT portable.  Whether a class
// with seventeen distinct empty bases folds them all to offset 0 is an ABI
// decision: the Itanium ABI does, and MSVC does so only under
// __declspec(empty_bases).  A test written that way would pass on gcc/clang
// and fail on the toolchain this project actually ships with.
//
//   So the suite proves the claim from an angle no ABI can argue with:
// hold the mixin arrangement FIXED and vary only the payload.  If a
// disabled color mixin truly costs nothing, then DFont<ff_none, probe_color>
// and DFont<ff_none, wide_color> - identical in every respect except a color
// type that differs by 60-odd bytes - must be exactly the same size.  Any
// layout that charged for the disabled mixin would leak that difference.

/*
tests_mixin_disabled_are_empty
  The precondition for EBO: every <false> face must actually be empty.  A
stray member on any one of them silently costs bytes in every font that
disables that feature.
  Tests the following:
  - all seventeen disabled mixins satisfy std::is_empty
  - the two color mixins are empty regardless of the color type they are
    handed - even a fat one
*/
bool tests_mixin_disabled_are_empty()
{
    using namespace font_mixin;

    D_FT_CHECK(std::is_empty<DUnderlineData<false>>::value);
    D_FT_CHECK(std::is_empty<DStrikethroughData<false>>::value);
    D_FT_CHECK(std::is_empty<DOverlineData<false>>::value);
    D_FT_CHECK(std::is_empty<DSmallCapsData<false>>::value);
    D_FT_CHECK(std::is_empty<DAllCapsData<false>>::value);
    D_FT_CHECK(std::is_empty<DSubscriptData<false>>::value);
    D_FT_CHECK(std::is_empty<DSuperscriptData<false>>::value);
    D_FT_CHECK(std::is_empty<DLetterSpacingData<false>>::value);
    D_FT_CHECK(std::is_empty<DLineHeightData<false>>::value);
    D_FT_CHECK(std::is_empty<DStretchData<false>>::value);
    D_FT_CHECK(std::is_empty<DSpacingData<false>>::value);
    D_FT_CHECK(std::is_empty<DOpentypeFeaturesData<false>>::value);
    D_FT_CHECK(std::is_empty<DVariableAxesData<false>>::value);
    D_FT_CHECK(std::is_empty<DScriptHintData<false>>::value);
    D_FT_CHECK(std::is_empty<DBackendHandlesData<false>>::value);

    // the color mixins carry the color type as a second parameter; the
    // disabled face must ignore it entirely
    D_FT_CHECK((std::is_empty<DColorData<false, probe_color>>::value));
    D_FT_CHECK((std::is_empty<DColorData<false, wide_color>>::value));
    D_FT_CHECK((std::is_empty<DBackgroundData<false, probe_color>>::value));
    D_FT_CHECK((std::is_empty<DBackgroundData<false, wide_color>>::value));

    return true;
}

/*
tests_mixin_enabled_are_not_empty
  The other half of the contract: an ENABLED mixin must actually carry data,
or the feature flag would be a no-op that silently drops writes.
  Tests the following:
  - every <true> face is non-empty
  - each one is at least as large as the member it introduces
*/
bool tests_mixin_enabled_are_not_empty()
{
    using namespace font_mixin;

    D_FT_CHECK(!std::is_empty<DUnderlineData<true>>::value);
    D_FT_CHECK(!std::is_empty<DStrikethroughData<true>>::value);
    D_FT_CHECK(!std::is_empty<DOverlineData<true>>::value);
    D_FT_CHECK(!std::is_empty<DSmallCapsData<true>>::value);
    D_FT_CHECK(!std::is_empty<DAllCapsData<true>>::value);
    D_FT_CHECK(!std::is_empty<DSubscriptData<true>>::value);
    D_FT_CHECK(!std::is_empty<DSuperscriptData<true>>::value);
    D_FT_CHECK(!std::is_empty<DLetterSpacingData<true>>::value);
    D_FT_CHECK(!std::is_empty<DLineHeightData<true>>::value);
    D_FT_CHECK(!std::is_empty<DStretchData<true>>::value);
    D_FT_CHECK(!std::is_empty<DSpacingData<true>>::value);
    D_FT_CHECK(!std::is_empty<DOpentypeFeaturesData<true>>::value);
    D_FT_CHECK(!std::is_empty<DVariableAxesData<true>>::value);
    D_FT_CHECK(!std::is_empty<DScriptHintData<true>>::value);
    D_FT_CHECK(!std::is_empty<DBackendHandlesData<true>>::value);

    D_FT_CHECK((!std::is_empty<DColorData<true, probe_color>>::value));
    D_FT_CHECK((!std::is_empty<DBackgroundData<true, probe_color>>::value));

    // the payloads are at least as big as what they hold
    D_FT_CHECK(sizeof(DLetterSpacingData<true>) >= sizeof(float));
    D_FT_CHECK(sizeof(DOpentypeFeaturesData<true>) >=
               sizeof(std::vector<DOpentypeFeature>));
    D_FT_CHECK((sizeof(DColorData<true, wide_color>) >= sizeof(wide_color)));

    return true;
}

/*
tests_mixin_decoration_defaults
  Tests the following:
  - underline, strikethrough, and overline all default to false
  - a font that enables them starts undecorated
*/
bool tests_mixin_decoration_defaults()
{
    const font_mixin::DUnderlineData<true>     u;
    const font_mixin::DStrikethroughData<true> s;
    const font_mixin::DOverlineData<true>      o;

    D_FT_CHECK(!u.underline);
    D_FT_CHECK(!s.strikethrough);
    D_FT_CHECK(!o.overline);

    // and through the font that inherits them
    const decorated_font_t f;

    D_FT_CHECK(!f.underline);
    D_FT_CHECK(!f.strikethrough);
    D_FT_CHECK(!f.overline);

    return true;
}

/*
tests_mixin_casing_defaults
  Tests the following:
  - small_caps, all_caps, subscript, and superscript all default to false
*/
bool tests_mixin_casing_defaults()
{
    const font_mixin::DSmallCapsData<true>  sc;
    const font_mixin::DAllCapsData<true>    ac;
    const font_mixin::DSubscriptData<true>   sub;
    const font_mixin::DSuperscriptData<true> sup;

    D_FT_CHECK(!sc.small_caps);
    D_FT_CHECK(!ac.all_caps);
    D_FT_CHECK(!sub.subscript);
    D_FT_CHECK(!sup.superscript);

    const casing_font_t f;

    D_FT_CHECK(!f.small_caps);
    D_FT_CHECK(!f.all_caps);
    D_FT_CHECK(!f.subscript);
    D_FT_CHECK(!f.superscript);

    return true;
}

/*
tests_mixin_metrics_defaults
  Both metric overrides default to 0.0f, and for line_height that zero is
LOAD-BEARING: the header defines 0 as "adapter default", not as "collapse
the leading to nothing".
  Tests the following:
  - letter_spacing defaults to 0.0f (no tracking adjustment)
  - line_height defaults to 0.0f, meaning "use the adapter's own metric"
  - both members are float
*/
bool tests_mixin_metrics_defaults()
{
    const font_mixin::DLetterSpacingData<true> ls;
    const font_mixin::DLineHeightData<true>    lh;

    D_FT_CHECK_NEAR(ls.letter_spacing, 0.0f);
    D_FT_CHECK_NEAR(lh.line_height,    0.0f);

    D_FT_CHECK(std::is_same<decltype(ls.letter_spacing), float>::value);
    D_FT_CHECK(std::is_same<decltype(lh.line_height),    float>::value);

    const metrics_font f;

    D_FT_CHECK_NEAR(f.letter_spacing, 0.0f);
    D_FT_CHECK_NEAR(f.line_height,    0.0f);

    return true;
}

/*
tests_mixin_axes_defaults
  Tests the following:
  - stretch defaults to DFontStretch::normal (5), a VALID width class -
    value-initialising it would have produced 0, which is not one
  - spacing defaults to DFontSpacing::any (0), i.e. "adapter should not
    filter"
*/
bool tests_mixin_axes_defaults()
{
    const font_mixin::DStretchData<true> st;
    const font_mixin::DSpacingData<true> sp;

    D_FT_CHECK(st.stretch == DFontStretch::normal);
    D_FT_CHECK(sp.spacing == DFontSpacing::any);

    // the distinction that matters: stretch is NOT zero-initialised
    D_FT_CHECK(static_cast<std::uint8_t>(st.stretch) != 0u);
    D_FT_CHECK(static_cast<std::uint8_t>(sp.spacing) == 0u);

    return true;
}

/*
tests_mixin_color_default_is_value_initialised
  The foreground is declared `_ColorType foreground {}`, so it is VALUE-
initialised - whatever that means for the color type in play.  For
probe_color that is all-zero channels.
  Tests the following:
  - the foreground of a fresh color mixin equals a value-initialised color
  - the same holds through the font that inherits it
*/
bool tests_mixin_color_default_is_value_initialised()
{
    const font_mixin::DColorData<true, probe_color> c;
    const probe_color                               fresh {};

    D_FT_CHECK(c.foreground == fresh);

    const color_font f;

    D_FT_CHECK(f.foreground == fresh);

    return true;
}

/*
tests_mixin_background_defaults
  The background mixin is the only one carrying TWO members - the color and
a separate enable flag - which is exactly why has_font_background is a
conjunction rather than a single detector.
  Tests the following:
  - background is value-initialised
  - background_enabled defaults to FALSE: a font with the feature compiled
    in still paints no background until someone asks for one
*/
bool tests_mixin_background_defaults()
{
    const font_mixin::DBackgroundData<true, probe_color> b;
    const probe_color                                    fresh {};

    D_FT_CHECK(b.background == fresh);
    D_FT_CHECK(!b.background_enabled);

    const background_font f;

    D_FT_CHECK(f.background == fresh);
    D_FT_CHECK(!f.background_enabled);

    return true;
}

/*
tests_mixin_opentype_features_default_empty
  Tests the following:
  - the feature vector starts empty
  - it is a std::vector<DOpentypeFeature>
*/
bool tests_mixin_opentype_features_default_empty()
{
    const font_mixin::DOpentypeFeaturesData<true> o;

    D_FT_CHECK(o.opentype_features.empty());
    D_FT_CHECK(o.opentype_features.size() == 0u);

    D_FT_CHECK((std::is_same<decltype(o.opentype_features),
                             std::vector<DOpentypeFeature>>::value));

    const opentype_font f;

    D_FT_CHECK(f.opentype_features.empty());

    return true;
}

/*
tests_mixin_variable_axes_default_empty
  Tests the following:
  - the axis vector starts empty
  - it is a std::vector<DVariableAxis>
*/
bool tests_mixin_variable_axes_default_empty()
{
    const font_mixin::DVariableAxesData<true> v;

    D_FT_CHECK(v.variable_axes.empty());

    D_FT_CHECK((std::is_same<decltype(v.variable_axes),
                             std::vector<DVariableAxis>>::value));

    const variable_font_t f;

    D_FT_CHECK(f.variable_axes.empty());

    return true;
}

/*
tests_mixin_script_hint_defaults_empty
  Tests the following:
  - script_tag and language_tag both start empty
  - both are std::string (a BCP-47 language tag will not fit a fourcc)
*/
bool tests_mixin_script_hint_defaults_empty()
{
    const font_mixin::DScriptHintData<true> s;

    D_FT_CHECK(s.script_tag.empty());
    D_FT_CHECK(s.language_tag.empty());

    D_FT_CHECK((std::is_same<decltype(s.script_tag),
                             std::string>::value));
    D_FT_CHECK((std::is_same<decltype(s.language_tag),
                             std::string>::value));

    const script_font f;

    D_FT_CHECK(f.script_tag.empty());
    D_FT_CHECK(f.language_tag.empty());

    return true;
}

/*
tests_mixin_backend_handles_defaults
  Tests the following:
  - postscript_name, full_name, and file_path start empty
  - face_index starts at 0 (the first face of a TTC / OTC)
  - native_handle starts at nullptr, and is an untyped void*
*/
bool tests_mixin_backend_handles_defaults()
{
    const font_mixin::DBackendHandlesData<true> b;

    D_FT_CHECK(b.postscript_name.empty());
    D_FT_CHECK(b.full_name.empty());
    D_FT_CHECK(b.file_path.empty());
    D_FT_CHECK(b.face_index == 0);
    D_FT_CHECK(b.native_handle == nullptr);

    D_FT_CHECK(std::is_same<decltype(b.face_index),    int>::value);
    D_FT_CHECK(std::is_same<decltype(b.native_handle), void*>::value);

    const backend_font f;

    D_FT_CHECK(f.file_path.empty());
    D_FT_CHECK(f.face_index == 0);
    D_FT_CHECK(f.native_handle == nullptr);

    return true;
}

/*
tests_mixin_color_type_is_parameterised
  The color mixins are the only ones templated on a second parameter.  The
member's type must BE that parameter, or DFont<F, rgba> would silently store
an rgb.
  Tests the following:
  - DColorData<true, C>::foreground has type C, for two different C
  - DBackgroundData<true, C>::background likewise
  - DFont<F, C>::color_type and the member type agree
*/
bool tests_mixin_color_type_is_parameterised()
{
    using probe_fg = decltype(
        font_mixin::DColorData<true, probe_color>{}.foreground);
    using wide_fg  = decltype(
        font_mixin::DColorData<true, wide_color>{}.foreground);

    D_FT_CHECK((std::is_same<probe_fg, probe_color>::value));
    D_FT_CHECK((std::is_same<wide_fg,  wide_color>::value));

    using probe_bg = decltype(
        font_mixin::DBackgroundData<true, probe_color>{}.background);

    D_FT_CHECK((std::is_same<probe_bg, probe_color>::value));

    // and the font agrees with its own alias
    using fw = DFont<ff_color | ff_background, wide_color>;

    D_FT_CHECK((std::is_same<fw::color_type, wide_color>::value));
    D_FT_CHECK((std::is_same<decltype(fw{}.foreground), wide_color>::value));
    D_FT_CHECK((std::is_same<decltype(fw{}.background), wide_color>::value));

    return true;
}

/*
tests_ebo_disabled_color_costs_nothing
  The central zero-cost claim, measured in the one way every ABI must
agree on.  probe_color and wide_color differ by tens of bytes.  With the
color features OFF, the two fonts must be byte-for-byte the same size: the
mixin arrangement is identical, so the ONLY thing that could differ is a
payload that is supposed not to exist.
  Tests the following:
  - sizeof(DFont<ff_none, probe_color>) == sizeof(DFont<ff_none, wide_color>)
  - the same holds for a font that enables a NON-color feature set
  - wide_color really is much larger, so the test has teeth
*/
bool tests_ebo_disabled_color_costs_nothing()
{
    // the test only means something if the two colors differ in size
    D_FT_CHECK(sizeof(wide_color) > sizeof(probe_color));
    D_FT_CHECK(sizeof(wide_color) >= (sizeof(probe_color) + 32u));

    D_FT_CHECK((sizeof(DFont<ff_none, probe_color>) ==
                sizeof(DFont<ff_none, wide_color>)));

    // a font with every NON-color feature must also be color-type agnostic
    constexpr unsigned no_color = ( ff_decorations | ff_casing | ff_metrics
                                  | ff_axes | ff_opentype_features
                                  | ff_variable_axes | ff_script_hint
                                  | ff_backend_handles );

    D_FT_CHECK((sizeof(DFont<no_color, probe_color>) ==
                sizeof(DFont<no_color, wide_color>)));

    return true;
}

/*
tests_ebo_disabled_color_costs_nothing_alongside_features
  The same probe, run across a spread of feature sets, so a layout bug that
only shows up once some mixin is non-empty cannot hide.
  Tests the following:
  - color-type independence holds for ff_underline, ff_decorations,
    ff_metrics, ff_gui_basic minus color, and ff_backend_handles
*/
bool tests_ebo_disabled_color_costs_nothing_alongside_features()
{
    D_FT_CHECK((sizeof(DFont<ff_underline, probe_color>) ==
                sizeof(DFont<ff_underline, wide_color>)));

    D_FT_CHECK((sizeof(DFont<ff_decorations, probe_color>) ==
                sizeof(DFont<ff_decorations, wide_color>)));

    D_FT_CHECK((sizeof(DFont<ff_metrics, probe_color>) ==
                sizeof(DFont<ff_metrics, wide_color>)));

    D_FT_CHECK((sizeof(DFont<ff_axes, probe_color>) ==
                sizeof(DFont<ff_axes, wide_color>)));

    D_FT_CHECK((sizeof(DFont<ff_backend_handles, probe_color>) ==
                sizeof(DFont<ff_backend_handles, wide_color>)));

    D_FT_CHECK((sizeof(DFont<ff_opentype_features, probe_color>) ==
                sizeof(DFont<ff_opentype_features, wide_color>)));

    return true;
}

/*
tests_ebo_enabled_color_costs_its_weight
  The converse, and the reason the previous test is not vacuous: once the
color feature IS on, the color type must be paid for.  A layout that folded
the enabled mixin away too would pass the zero-cost test for entirely the
wrong reason.

  Note the growth need not equal the raw sizeof difference between the two
color types.  A small color (probe_color is 3 bytes) can land partly in the
outer struct's trailing alignment padding, so replacing it with a 64-byte
color grows the object by somewhat LESS than 61 bytes - the small color was
partly free.  What must hold is the direction and a substantial magnitude,
not an exact byte count that would hard-code one ABI's padding.
  Tests the following:
  - with ff_color on, the wide-color font is strictly larger
  - the growth is large - most of the color-type difference shows up, even
    if a few bytes hide in padding
  - ff_background costs as well, and the two together cost more than either
*/
bool tests_ebo_enabled_color_costs_its_weight()
{
    const std::size_t narrow = sizeof(DFont<ff_color, probe_color>);
    const std::size_t wide   = sizeof(DFont<ff_color, wide_color>);

    // direction: the fat color must cost more
    D_FT_CHECK(wide > narrow);

    // magnitude: most of the difference is real payload, not padding.  We
    // do not demand the FULL 61-byte difference (a few bytes of the 3-byte
    // probe color may have sat in the core's trailing padding), but the
    // growth must be within one pointer's worth of it - far more than the
    // "zero" a folded-away mixin would show.
    const std::size_t raw_diff = (sizeof(wide_color) - sizeof(probe_color));

    D_FT_CHECK((wide - narrow) > 0u);
    D_FT_CHECK((wide - narrow) >= (raw_diff - sizeof(void*)));

    // background pays too
    D_FT_CHECK((sizeof(DFont<ff_background, wide_color>) >
                sizeof(DFont<ff_background, probe_color>)));

    // and both together cost at least as much as either alone
    const std::size_t both = sizeof(DFont<ff_color | ff_background,
                                         wide_color>);

    D_FT_CHECK(both >= wide);
    D_FT_CHECK(both >= sizeof(DFont<ff_background, wide_color>));

    return true;
}

/*
tests_ebo_features_grow_the_type
  Tests the following:
  - a font with every feature is strictly larger than a bare one
  - enabling a feature never SHRINKS the type
*/
bool tests_ebo_features_grow_the_type()
{
    D_FT_CHECK(sizeof(full_font) > sizeof(bare_font));

    D_FT_CHECK(sizeof(underline_font_t) >= sizeof(bare_font));
    D_FT_CHECK(sizeof(decorated_font_t) >= sizeof(bare_font));
    D_FT_CHECK(sizeof(metrics_font)     >= sizeof(bare_font));
    D_FT_CHECK(sizeof(opentype_font)    >  sizeof(bare_font));
    D_FT_CHECK(sizeof(script_font)      >  sizeof(bare_font));
    D_FT_CHECK(sizeof(backend_font)     >  sizeof(bare_font));

    return true;
}

/*
tests_ebo_profiles_are_ordered_by_payload
  The platform profiles nest as bit sets (section 1), so they must nest as
payloads too: a superset of features can never produce a smaller object.
  Tests the following:
  - bare <= terminal <= gui_basic is NOT assumed (the profiles are not
    nested that way); instead the genuinely nested chain is checked:
    gui_basic's bits are a subset of gui_standard's, and gui_standard's of
    gui_rich's, so the sizes are non-decreasing along that chain
  - gui_rich (== ff_all) is the largest
*/
bool tests_ebo_profiles_are_ordered_by_payload()
{
    // the bit-subset chain established in section 1
    D_FT_CHECK((static_cast<unsigned>(ff_gui_basic) &
                static_cast<unsigned>(ff_gui_standard)) ==
               static_cast<unsigned>(ff_gui_basic));
    D_FT_CHECK((static_cast<unsigned>(ff_gui_standard) &
                static_cast<unsigned>(ff_gui_rich)) ==
               static_cast<unsigned>(ff_gui_standard));

    // therefore the payloads are non-decreasing along it
    D_FT_CHECK(sizeof(gui_basic_font) <= sizeof(gui_std_font));
    D_FT_CHECK(sizeof(gui_std_font)   <= sizeof(gui_rich_font));

    D_FT_CHECK(sizeof(bare_font)     <= sizeof(gui_basic_font));
    D_FT_CHECK(sizeof(terminal_font) <= sizeof(gui_std_font));

    // ff_all is ff_gui_rich, so they are literally the same type
    D_FT_CHECK(sizeof(full_font) == sizeof(gui_rich_font));

    return true;
}

NS_END  // testing
NS_END  // djinterp
