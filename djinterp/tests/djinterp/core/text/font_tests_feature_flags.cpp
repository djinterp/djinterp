// djinterp [test]  font_tests_feature_flags.cpp
//   Section 1: font_feat, operator|, has_ff.  The bits gate every mixin in
// the header, so their values are load-bearing ABI, not decoration.

// djinterp
#include "font_tests.hpp"


NS_DJINTERP
NS_TESTING

/*
tests_ff_none_is_zero
  The empty feature set.
  Tests the following:
  - ff_none is exactly 0
  - a font<ff_none> therefore enables no mixin
*/
bool
tests_ff_none_is_zero()
{
    static_assert((static_cast<unsigned>(ff_none) == 0u),
                  "ff_none must be the zero bit set");

    D_FT_CHECK(static_cast<unsigned>(ff_none) == 0u);

    return true;
}

/*
tests_ff_individual_bit_values
  Each capability flag holds the exact bit the header assigns it.
  Tests the following:
  - all seventeen flags, one by one, against 1u << n
  - the numbering is contiguous from bit 0 to bit 16
*/
bool
tests_ff_individual_bit_values()
{
    D_FT_CHECK(static_cast<unsigned>(ff_underline)         == (1u <<  0));
    D_FT_CHECK(static_cast<unsigned>(ff_strikethrough)     == (1u <<  1));
    D_FT_CHECK(static_cast<unsigned>(ff_overline)          == (1u <<  2));
    D_FT_CHECK(static_cast<unsigned>(ff_small_caps)        == (1u <<  3));
    D_FT_CHECK(static_cast<unsigned>(ff_all_caps)          == (1u <<  4));
    D_FT_CHECK(static_cast<unsigned>(ff_subscript)         == (1u <<  5));
    D_FT_CHECK(static_cast<unsigned>(ff_superscript)       == (1u <<  6));
    D_FT_CHECK(static_cast<unsigned>(ff_letter_spacing)    == (1u <<  7));
    D_FT_CHECK(static_cast<unsigned>(ff_line_height)       == (1u <<  8));
    D_FT_CHECK(static_cast<unsigned>(ff_stretch)           == (1u <<  9));
    D_FT_CHECK(static_cast<unsigned>(ff_spacing)           == (1u << 10));
    D_FT_CHECK(static_cast<unsigned>(ff_color)             == (1u << 11));
    D_FT_CHECK(static_cast<unsigned>(ff_background)        == (1u << 12));
    D_FT_CHECK(static_cast<unsigned>(ff_opentype_features) == (1u << 13));
    D_FT_CHECK(static_cast<unsigned>(ff_variable_axes)     == (1u << 14));
    D_FT_CHECK(static_cast<unsigned>(ff_script_hint)       == (1u << 15));
    D_FT_CHECK(static_cast<unsigned>(ff_backend_handles)   == (1u << 16));

    return true;
}

/*
tests_ff_flags_are_distinct_powers_of_two
  The gating scheme only works if no two capabilities share a bit.
  Tests the following:
  - every flag is a power of two (exactly one bit set)
  - every pair of flags is disjoint (no aliasing)
*/
bool
tests_ff_flags_are_distinct_powers_of_two()
{
    const unsigned flags[] =
    {
        ff_underline,      ff_strikethrough,     ff_overline,
        ff_small_caps,     ff_all_caps,          ff_subscript,
        ff_superscript,    ff_letter_spacing,    ff_line_height,
        ff_stretch,        ff_spacing,           ff_color,
        ff_background,     ff_opentype_features, ff_variable_axes,
        ff_script_hint,    ff_backend_handles
    };

    const std::size_t count = sizeof(flags) / sizeof(flags[0]);

    D_FT_CHECK(count == 17u);

    // each flag must carry exactly one bit
    for (std::size_t i = 0; i < count; ++i)
    {
        D_FT_CHECK(flags[i] != 0u);
        D_FT_CHECK((flags[i] & (flags[i] - 1u)) == 0u);
    }

    // no two flags may overlap
    for (std::size_t i = 0; i < count; ++i)
    {
        for (std::size_t j = (i + 1u); j < count; ++j)
        {
            D_FT_CHECK((flags[i] & flags[j]) == 0u);
        }
    }

    return true;
}

/*
tests_ff_underlying_type_is_unsigned
  font_feat is declared `: unsigned`, which is what lets a raw OR of the
enumerators be handed straight to font<>'s `unsigned _Feat` parameter.
  Tests the following:
  - std::underlying_type_t<font_feat> is unsigned
  - font_feat is an enum, and an UNSCOPED one (it converts implicitly)
*/
bool
tests_ff_underlying_type_is_unsigned()
{
    static_assert(std::is_same<std::underlying_type<font_feat>::type,
                               unsigned>::value,
                  "font_feat must be backed by unsigned");
    static_assert(std::is_enum<font_feat>::value, "font_feat must be an enum");
    static_assert(std::is_convertible<font_feat, unsigned>::value,
                  "font_feat must be unscoped so it feeds font<>'s _Feat");

    D_FT_CHECK(std::is_same<std::underlying_type<font_feat>::type,
                            unsigned>::value);
    D_FT_CHECK(std::is_convertible<font_feat, unsigned>::value);

    return true;
}

/*
tests_ff_bits_fit_in_unsigned
  The highest flag is 1u << 16; the header would silently break on a
platform where unsigned could not hold it.
  Tests the following:
  - ff_all fits in unsigned with room to spare
  - the top flag has not wrapped to zero
*/
bool
tests_ff_bits_fit_in_unsigned()
{
    static_assert((sizeof(unsigned) * 8u) > 16u,
                  "unsigned must be wide enough for bit 16");

    D_FT_CHECK(static_cast<unsigned>(ff_backend_handles) != 0u);
    D_FT_CHECK(static_cast<unsigned>(ff_all) >=
               static_cast<unsigned>(ff_backend_handles));

    return true;
}

/*
tests_ff_decorations_aggregate
  Tests the following:
  - ff_decorations is exactly underline | strikethrough | overline
  - it carries no other bit
*/
bool
tests_ff_decorations_aggregate()
{
    const unsigned want = ( static_cast<unsigned>(ff_underline)     |
                            static_cast<unsigned>(ff_strikethrough) |
                            static_cast<unsigned>(ff_overline) );

    D_FT_CHECK(static_cast<unsigned>(ff_decorations) == want);
    D_FT_CHECK(has_ff(ff_decorations, ff_underline));
    D_FT_CHECK(has_ff(ff_decorations, ff_strikethrough));
    D_FT_CHECK(has_ff(ff_decorations, ff_overline));
    D_FT_CHECK(!has_ff(ff_decorations, ff_small_caps));
    D_FT_CHECK(!has_ff(ff_decorations, ff_color));

    return true;
}

/*
tests_ff_casing_aggregate
  Tests the following:
  - ff_casing is exactly small_caps | all_caps | subscript | superscript
*/
bool
tests_ff_casing_aggregate()
{
    const unsigned want = ( static_cast<unsigned>(ff_small_caps) |
                            static_cast<unsigned>(ff_all_caps)   |
                            static_cast<unsigned>(ff_subscript)  |
                            static_cast<unsigned>(ff_superscript) );

    D_FT_CHECK(static_cast<unsigned>(ff_casing) == want);
    D_FT_CHECK(has_ff(ff_casing, ff_small_caps));
    D_FT_CHECK(has_ff(ff_casing, ff_all_caps));
    D_FT_CHECK(has_ff(ff_casing, ff_subscript));
    D_FT_CHECK(has_ff(ff_casing, ff_superscript));
    D_FT_CHECK(!has_ff(ff_casing, ff_underline));

    return true;
}

/*
tests_ff_metrics_aggregate
  Tests the following:
  - ff_metrics is exactly letter_spacing | line_height
*/
bool
tests_ff_metrics_aggregate()
{
    const unsigned want = ( static_cast<unsigned>(ff_letter_spacing) |
                            static_cast<unsigned>(ff_line_height) );

    D_FT_CHECK(static_cast<unsigned>(ff_metrics) == want);
    D_FT_CHECK(has_ff(ff_metrics, ff_letter_spacing));
    D_FT_CHECK(has_ff(ff_metrics, ff_line_height));
    D_FT_CHECK(!has_ff(ff_metrics, ff_stretch));

    return true;
}

/*
tests_ff_axes_aggregate
  Tests the following:
  - ff_axes is exactly stretch | spacing
  - it does NOT include the variable-font axes (a different concept)
*/
bool
tests_ff_axes_aggregate()
{
    const unsigned want = ( static_cast<unsigned>(ff_stretch) |
                            static_cast<unsigned>(ff_spacing) );

    D_FT_CHECK(static_cast<unsigned>(ff_axes) == want);
    D_FT_CHECK(has_ff(ff_axes, ff_stretch));
    D_FT_CHECK(has_ff(ff_axes, ff_spacing));
    D_FT_CHECK(!has_ff(ff_axes, ff_variable_axes));

    return true;
}

/*
tests_ff_terminal_profiles
  The two terminal profiles are what a TUI backend actually supports.
  Tests the following:
  - ff_terminal_basic is color | background, and nothing else
  - ff_terminal_rich adds underline and strikethrough
  - neither profile carries an overline (terminals cannot draw one)
  - neither carries metrics, casing, or backend handles
*/
bool
tests_ff_terminal_profiles()
{
    const unsigned basic = ( static_cast<unsigned>(ff_color) |
                             static_cast<unsigned>(ff_background) );

    D_FT_CHECK(static_cast<unsigned>(ff_terminal_basic) == basic);

    const unsigned rich = ( basic                                   |
                            static_cast<unsigned>(ff_underline)     |
                            static_cast<unsigned>(ff_strikethrough) );

    D_FT_CHECK(static_cast<unsigned>(ff_terminal_rich) == rich);

    D_FT_CHECK(!has_ff(ff_terminal_basic, ff_underline));
    D_FT_CHECK(!has_ff(ff_terminal_rich,  ff_overline));
    D_FT_CHECK(!has_ff(ff_terminal_rich,  ff_letter_spacing));
    D_FT_CHECK(!has_ff(ff_terminal_rich,  ff_small_caps));
    D_FT_CHECK(!has_ff(ff_terminal_rich,  ff_backend_handles));

    return true;
}

/*
tests_ff_gui_profiles
  Tests the following:
  - ff_gui_basic is decorations | metrics | color
  - ff_gui_standard is decorations | casing | metrics | axes | color |
    background | backend_handles
  - ff_gui_rich adds opentype features, variable axes, and the script hint
  - the profiles nest: basic's bits are a subset of standard's, and
    standard's a subset of rich's
*/
bool
tests_ff_gui_profiles()
{
    const unsigned basic = ( static_cast<unsigned>(ff_decorations) |
                             static_cast<unsigned>(ff_metrics)     |
                             static_cast<unsigned>(ff_color) );

    D_FT_CHECK(static_cast<unsigned>(ff_gui_basic) == basic);

    const unsigned standard = ( static_cast<unsigned>(ff_decorations)     |
                                static_cast<unsigned>(ff_casing)          |
                                static_cast<unsigned>(ff_metrics)         |
                                static_cast<unsigned>(ff_axes)            |
                                static_cast<unsigned>(ff_color)           |
                                static_cast<unsigned>(ff_background)      |
                                static_cast<unsigned>(ff_backend_handles) );

    D_FT_CHECK(static_cast<unsigned>(ff_gui_standard) == standard);

    const unsigned rich = ( standard                                        |
                            static_cast<unsigned>(ff_opentype_features)     |
                            static_cast<unsigned>(ff_variable_axes)         |
                            static_cast<unsigned>(ff_script_hint) );

    D_FT_CHECK(static_cast<unsigned>(ff_gui_rich) == rich);

    // the profiles nest
    D_FT_CHECK((basic    & standard) == basic);
    D_FT_CHECK((standard & rich)     == standard);

    return true;
}

/*
tests_ff_gui_standard_omits_opentype_and_variable
  The sharp edge of the profile ladder: ff_gui_standard is NOT a rich font.
It has decorations, casing, metrics, color - but no OpenType feature list,
so is_font_rich (section 10) rejects it.  Pinning the bits here is what
makes that later result a deliberate design fact rather than an accident.
  Tests the following:
  - ff_gui_standard lacks ff_opentype_features
  - ff_gui_standard lacks ff_variable_axes and ff_script_hint
  - ff_gui_rich has all three
*/
bool
tests_ff_gui_standard_omits_opentype_and_variable()
{
    D_FT_CHECK(!has_ff(ff_gui_standard, ff_opentype_features));
    D_FT_CHECK(!has_ff(ff_gui_standard, ff_variable_axes));
    D_FT_CHECK(!has_ff(ff_gui_standard, ff_script_hint));

    D_FT_CHECK(has_ff(ff_gui_rich, ff_opentype_features));
    D_FT_CHECK(has_ff(ff_gui_rich, ff_variable_axes));
    D_FT_CHECK(has_ff(ff_gui_rich, ff_script_hint));

    // everything gui_standard DOES have, gui_rich has too
    D_FT_CHECK((static_cast<unsigned>(ff_gui_standard) &
                static_cast<unsigned>(ff_gui_rich)) ==
               static_cast<unsigned>(ff_gui_standard));

    return true;
}

/*
tests_ff_all_contains_every_flag
  Tests the following:
  - has_ff(ff_all, x) is true for every one of the seventeen flags
  - ff_all sets no bit above bit 16
*/
bool
tests_ff_all_contains_every_flag()
{
    D_FT_CHECK(has_ff(ff_all, ff_underline));
    D_FT_CHECK(has_ff(ff_all, ff_strikethrough));
    D_FT_CHECK(has_ff(ff_all, ff_overline));
    D_FT_CHECK(has_ff(ff_all, ff_small_caps));
    D_FT_CHECK(has_ff(ff_all, ff_all_caps));
    D_FT_CHECK(has_ff(ff_all, ff_subscript));
    D_FT_CHECK(has_ff(ff_all, ff_superscript));
    D_FT_CHECK(has_ff(ff_all, ff_letter_spacing));
    D_FT_CHECK(has_ff(ff_all, ff_line_height));
    D_FT_CHECK(has_ff(ff_all, ff_stretch));
    D_FT_CHECK(has_ff(ff_all, ff_spacing));
    D_FT_CHECK(has_ff(ff_all, ff_color));
    D_FT_CHECK(has_ff(ff_all, ff_background));
    D_FT_CHECK(has_ff(ff_all, ff_opentype_features));
    D_FT_CHECK(has_ff(ff_all, ff_variable_axes));
    D_FT_CHECK(has_ff(ff_all, ff_script_hint));
    D_FT_CHECK(has_ff(ff_all, ff_backend_handles));

    // 17 contiguous bits, nothing above them
    const unsigned mask = ((1u << 17) - 1u);

    D_FT_CHECK((static_cast<unsigned>(ff_all) & ~mask) == 0u);
    D_FT_CHECK(static_cast<unsigned>(ff_all) == mask);

    return true;
}

/*
tests_ff_all_equals_gui_rich
  A property worth recording rather than discovering by surprise: as the
header stands today ff_all and ff_gui_rich enumerate the SAME bits, so the
two names are interchangeable.  If a future flag is added to ff_all but not
to ff_gui_rich, this test is the one that notices.
  Tests the following:
  - ff_all and ff_gui_rich are bit-identical
  - font<ff_all> and font<ff_gui_rich> are therefore the same type
*/
bool
tests_ff_all_equals_gui_rich()
{
    D_FT_CHECK(static_cast<unsigned>(ff_all) ==
               static_cast<unsigned>(ff_gui_rich));

    D_FT_CHECK((std::is_same<font<ff_all,      probe_color>,
                             font<ff_gui_rich, probe_color>>::value));

    return true;
}

/*
tests_operator_or_combines_bits
  Tests the following:
  - a | b sets both bits
  - chaining a third flag keeps the first two
  - OR-ing with ff_none is the identity
*/
bool
tests_operator_or_combines_bits()
{
    const unsigned two = (ff_underline | ff_color);

    D_FT_CHECK(has_ff(two, ff_underline));
    D_FT_CHECK(has_ff(two, ff_color));
    D_FT_CHECK(!has_ff(two, ff_overline));

    const unsigned three = (two | static_cast<unsigned>(ff_overline));

    D_FT_CHECK(has_ff(three, ff_underline));
    D_FT_CHECK(has_ff(three, ff_color));
    D_FT_CHECK(has_ff(three, ff_overline));

    // identity
    D_FT_CHECK((ff_underline | ff_none) ==
               static_cast<unsigned>(ff_underline));

    return true;
}

/*
tests_operator_or_yields_unsigned
  The overload returns `unsigned`, not font_feat - which is exactly why the
result can be handed to font<>'s `unsigned _Feat` parameter with no cast at
the call site.
  Tests the following:
  - decltype(ff_a | ff_b) is unsigned
  - the result is usable directly as a template argument
*/
bool
tests_operator_or_yields_unsigned()
{
    static_assert(std::is_same<decltype(ff_underline | ff_overline),
                               unsigned>::value,
                  "operator| must yield unsigned for font<>'s _Feat slot");

    D_FT_CHECK((std::is_same<decltype(ff_underline | ff_overline),
                             unsigned>::value));

    // the whole point: it drops straight into the template parameter
    using combined = font<ff_underline | ff_overline, probe_color>;

    D_FT_CHECK(combined::has_underline);
    D_FT_CHECK(combined::has_overline);
    D_FT_CHECK(!combined::has_strikethrough);

    return true;
}

/*
tests_operator_or_is_constexpr_and_noexcept
  Tests the following:
  - operator| folds in a constant expression
  - it is declared noexcept
*/
bool
tests_operator_or_is_constexpr_and_noexcept()
{
    constexpr unsigned combined = (ff_color | ff_background);

    static_assert((combined == static_cast<unsigned>(ff_terminal_basic)),
                  "color | background must fold to ff_terminal_basic");
    static_assert(noexcept(ff_color | ff_background),
                  "operator| must be noexcept");

    D_FT_CHECK(combined == static_cast<unsigned>(ff_terminal_basic));
    D_FT_CHECK(noexcept(ff_color | ff_background));

    return true;
}

/*
tests_operator_or_is_commutative_and_idempotent
  Tests the following:
  - a | b == b | a
  - a | a == a
  - the operation is associative across three flags
*/
bool
tests_operator_or_is_commutative_and_idempotent()
{
    D_FT_CHECK((ff_underline | ff_color) == (ff_color | ff_underline));
    D_FT_CHECK((ff_underline | ff_underline) ==
               static_cast<unsigned>(ff_underline));

    const unsigned left  = ((ff_underline | ff_color) |
                            static_cast<unsigned>(ff_overline));
    const unsigned right = (static_cast<unsigned>(ff_underline) |
                            (ff_color | ff_overline));

    D_FT_CHECK(left == right);

    return true;
}

/*
tests_has_ff_detects_set_bits
  Tests the following:
  - has_ff is true for every bit present in the set
  - it works on a hand-rolled combination as well as a named profile
*/
bool
tests_has_ff_detects_set_bits()
{
    const unsigned f = (ff_underline | ff_color);

    D_FT_CHECK(has_ff(f, ff_underline));
    D_FT_CHECK(has_ff(f, ff_color));

    D_FT_CHECK(has_ff(ff_gui_basic, ff_underline));
    D_FT_CHECK(has_ff(ff_gui_basic, ff_line_height));
    D_FT_CHECK(has_ff(ff_gui_basic, ff_color));

    return true;
}

/*
tests_has_ff_rejects_unset_bits
  Tests the following:
  - has_ff is false for bits absent from the set
  - a neighbouring bit does not leak in
*/
bool
tests_has_ff_rejects_unset_bits()
{
    const unsigned f = static_cast<unsigned>(ff_underline);

    D_FT_CHECK(!has_ff(f, ff_strikethrough));
    D_FT_CHECK(!has_ff(f, ff_overline));
    D_FT_CHECK(!has_ff(f, ff_color));

    D_FT_CHECK(!has_ff(ff_gui_basic, ff_small_caps));
    D_FT_CHECK(!has_ff(ff_gui_basic, ff_background));
    D_FT_CHECK(!has_ff(ff_gui_basic, ff_opentype_features));

    return true;
}

/*
tests_has_ff_on_ff_none_is_always_false
  Tests the following:
  - querying the empty set returns false for every flag
*/
bool
tests_has_ff_on_ff_none_is_always_false()
{
    D_FT_CHECK(!has_ff(ff_none, ff_underline));
    D_FT_CHECK(!has_ff(ff_none, ff_strikethrough));
    D_FT_CHECK(!has_ff(ff_none, ff_overline));
    D_FT_CHECK(!has_ff(ff_none, ff_small_caps));
    D_FT_CHECK(!has_ff(ff_none, ff_all_caps));
    D_FT_CHECK(!has_ff(ff_none, ff_subscript));
    D_FT_CHECK(!has_ff(ff_none, ff_superscript));
    D_FT_CHECK(!has_ff(ff_none, ff_letter_spacing));
    D_FT_CHECK(!has_ff(ff_none, ff_line_height));
    D_FT_CHECK(!has_ff(ff_none, ff_stretch));
    D_FT_CHECK(!has_ff(ff_none, ff_spacing));
    D_FT_CHECK(!has_ff(ff_none, ff_color));
    D_FT_CHECK(!has_ff(ff_none, ff_background));
    D_FT_CHECK(!has_ff(ff_none, ff_opentype_features));
    D_FT_CHECK(!has_ff(ff_none, ff_variable_axes));
    D_FT_CHECK(!has_ff(ff_none, ff_script_hint));
    D_FT_CHECK(!has_ff(ff_none, ff_backend_handles));

    return true;
}

/*
tests_has_ff_with_the_ff_none_bit_is_always_false
  The degenerate query, and a trap worth naming: has_ff(f, ff_none) masks
with zero, so it is false even for ff_all.  "Does this font have no
features?" is NOT the question has_ff answers - compare against ff_none
directly instead.
  Tests the following:
  - has_ff(x, ff_none) is false for the empty set, a singleton, a profile,
    and the full set alike
*/
bool
tests_has_ff_with_the_ff_none_bit_is_always_false()
{
    D_FT_CHECK(!has_ff(ff_none,          ff_none));
    D_FT_CHECK(!has_ff(ff_underline,     ff_none));
    D_FT_CHECK(!has_ff(ff_gui_standard,  ff_none));
    D_FT_CHECK(!has_ff(ff_all,           ff_none));

    // the question you actually meant
    D_FT_CHECK(static_cast<unsigned>(ff_none) == 0u);

    return true;
}

/*
tests_has_ff_on_aggregates
  has_ff's second argument is typed font_feat, so an AGGREGATE may be passed
as the probe.  Because the test is `(f & bits) != 0`, that asks "any", not
"all" - a font with only underline reports true against ff_decorations.
  Tests the following:
  - probing with an aggregate is an ANY test, not an ALL test
  - a set holding one member of an aggregate matches the aggregate
  - a set holding none of them does not
*/
bool
tests_has_ff_on_aggregates()
{
    const unsigned one = static_cast<unsigned>(ff_overline);

    // any-semantics: one decoration is enough to answer to ff_decorations
    D_FT_CHECK(has_ff(one, ff_decorations));
    D_FT_CHECK(!has_ff(one, ff_casing));

    D_FT_CHECK(has_ff(ff_gui_basic, ff_decorations));
    D_FT_CHECK(has_ff(ff_gui_basic, ff_metrics));
    D_FT_CHECK(!has_ff(ff_gui_basic, ff_casing));
    D_FT_CHECK(!has_ff(ff_gui_basic, ff_axes));

    D_FT_CHECK(has_ff(ff_all, ff_decorations));
    D_FT_CHECK(has_ff(ff_all, ff_casing));
    D_FT_CHECK(has_ff(ff_all, ff_metrics));
    D_FT_CHECK(has_ff(ff_all, ff_axes));

    return true;
}

/*
tests_has_ff_is_constexpr_and_noexcept
  has_ff must fold at compile time - font<> calls it seventeen times in its
own base-class list and again for every static has_* constant.
  Tests the following:
  - has_ff is usable in a static_assert
  - has_ff is usable as a template argument
  - has_ff is noexcept
*/
bool
tests_has_ff_is_constexpr_and_noexcept()
{
    static_assert(has_ff(ff_gui_basic, ff_color),
                  "has_ff must fold in a constant expression");
    static_assert(!has_ff(ff_gui_basic, ff_background),
                  "has_ff must fold in a constant expression");
    static_assert(noexcept(has_ff(ff_all, ff_color)),
                  "has_ff must be noexcept");

    // used as a non-type template argument
    using probe = std::integral_constant<bool,
                                         has_ff(ff_terminal_rich,
                                                ff_strikethrough)>;

    D_FT_CHECK(probe::value);
    D_FT_CHECK(noexcept(has_ff(ff_all, ff_color)));

    return true;
}

NS_END  // testing
NS_END  // djinterp
