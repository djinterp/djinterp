/******************************************************************************
* djinterp [test]                                                font_tests.hpp
*
*   Declarations, probe types, and helpers for the font.hpp unit suite.
* The section TUs (font_tests_*.cpp) define the tests_* functions, flat in
* djinterp::testing; the runner (font_tests_runner.cpp, in the config tree)
* registers them with report_builder.
*
*   The suite is deliberately built on TWO kinds of subject:
*
*     - font<_Feat, _ColorType> itself, instantiated across the whole
*       feature lattice (ff_none, singletons, aggregates, the platform
*       profiles, ff_all), which exercises the mixins, the free functions,
*       and the static has_* constants.
*
*     - DUCK-TYPED probe structs that never derive from font<> at all.
*       The traits in section 10 are STRUCTURAL - they detect members, not
*       inheritance - and the only way to prove that is to feed them types
*       that font.hpp has never heard of.  The probes are also how the
*       conjunctions (has_font_background, has_font_script_hint) and the
*       disjunctions (has_font_decorations, has_font_backend_handles) are
*       pinned down: a font<> can never present HALF of a mixin, so a
*       hand-built half-mixin is the only witness.
*
*   The color mixins are exercised through probe_color / wide_color rather
* than the default djinterp::rgb, so the suite states only what font.hpp
* promises of a color type (default-constructible, copyable, equality-
* comparable) and never leans on rgb's channel layout.
*
*
* path:      /tests/djinterp/core/text/font_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.12
******************************************************************************/

#ifndef DJINTERP_TEXT_FONT_TESTS_
#define DJINTERP_TEXT_FONT_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "font.hpp"


NS_DJINTERP
NS_TESTING

// =============================================================================
//  1.  SUITE MACROS
// =============================================================================

// D_FT_CHECK
//   macro: fails the enclosing test (returns false) when the condition is
// false.  The suite-unique letters (FT = FonT) keep the name from colliding
// with a co-compiled suite's check macro.
//
//   Variadic on purpose: a great many of this suite's assertions are of the
// form `std::is_same<A, B>::value`, and the preprocessor would otherwise
// read the comma inside the template argument list as an argument separator.
// __VA_ARGS__ rejoins them.  (MSVC needs /Zc:preprocessor for the conformant
// behaviour; djinterp_module.cmake already passes it.)
#define D_FT_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)

// D_FT_CHECK_NEAR
//   macro: fails the enclosing test unless _a and _b agree to within the
// suite's float tolerance.
#define D_FT_CHECK_NEAR(_a, _b)                                               \
    D_FT_CHECK(::djinterp::testing::approx_eq((_a), (_b)))


// =============================================================================
//  2.  HELPERS
// =============================================================================

// k_epsilon
//   constant: absolute tolerance for the suite's float comparisons.  Sizes
// live in the 0..400 range, so an absolute bound is both meaningful and
// immune to the rounding that `percent` conversions pick up from 0.01f.
constexpr float k_epsilon = 1.0e-4f;

// approx_eq
//   function: absolute-tolerance float comparison.
constexpr bool
approx_eq(
    float  _a,
    float  _b,
    float  _eps = k_epsilon
) noexcept
{
    return ( ( (_a > _b) ? (_a - _b) : (_b - _a) ) <= _eps );
}

// weight_of
//   function: the numeric value behind a symbolic font_weight.
constexpr std::uint16_t
weight_of(
    font_weight _w
) noexcept
{
    return static_cast<std::uint16_t>(_w);
}


// =============================================================================
//  3.  PROBE COLOR TYPES
// =============================================================================

// probe_color
//   struct: the suite's stand-in color.  Satisfies exactly what font.hpp
// asks of a _ColorType - default-constructible, copyable, equality-
// comparable - and nothing more, so a passing color test cannot be leaning
// on any property of djinterp::rgb.
struct probe_color
{
    std::uint8_t  r = 0;
    std::uint8_t  g = 0;
    std::uint8_t  b = 0;

    constexpr bool
    operator==(const probe_color& _o) const noexcept
    {
        return ( (r == _o.r) &&
                 (g == _o.g) &&
                 (b == _o.b) );
    }
};

// wide_color
//   struct: a deliberately fat color type.  Used to prove the EBO claim
// from the other side: when the color features are OFF the color type must
// cost nothing, and when they are ON it must cost what it weighs.
struct wide_color
{
    double  channel[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

    constexpr bool
    operator==(const wide_color& _o) const noexcept
    {
        return (channel[0] == _o.channel[0]);
    }
};


// =============================================================================
//  4.  FONT ALIASES  (the feature lattice under test)
// =============================================================================

using bare_font        = font<ff_none,              probe_color>;
using underline_font_t = font<ff_underline,         probe_color>;
using strike_font      = font<ff_strikethrough,     probe_color>;
using overline_font_t  = font<ff_overline,          probe_color>;
using decorated_font_t = font<ff_decorations,       probe_color>;
using small_caps_font_t= font<ff_small_caps,        probe_color>;
using casing_font_t    = font<ff_casing,            probe_color>;
using letter_font      = font<ff_letter_spacing,    probe_color>;
using line_font        = font<ff_line_height,       probe_color>;
using metrics_font     = font<ff_metrics,           probe_color>;
using stretch_font_t   = font<ff_stretch,           probe_color>;
using spacing_font_t   = font<ff_spacing,           probe_color>;
using axes_font        = font<ff_axes,              probe_color>;
using color_font       = font<ff_color,             probe_color>;
using background_font  = font<ff_background,        probe_color>;
using opentype_font    = font<ff_opentype_features, probe_color>;
using variable_font_t  = font<ff_variable_axes,     probe_color>;
using script_font      = font<ff_script_hint,       probe_color>;
using backend_font     = font<ff_backend_handles,   probe_color>;
using terminal_font    = font<ff_terminal_basic,    probe_color>;
using gui_basic_font   = font<ff_gui_basic,         probe_color>;
using gui_std_font     = font<ff_gui_standard,      probe_color>;
using gui_rich_font    = font<ff_gui_rich,          probe_color>;
using full_font        = font<ff_all,               probe_color>;

// minimal_rich_font
//   type: the SMALLEST feature set that still satisfies is_font_rich - one
// decoration, one casing transform, one metric override, color, and
// OpenType features.  It carries no background, no axes, no variable axes,
// no script hint, and no backend handles, which is the point: is_font_rich
// is built from disjunctions, so the rich gate does NOT imply ff_gui_rich.
using minimal_rich_font =
    font<ff_underline | ff_small_caps | ff_line_height
       | ff_color | ff_opentype_features, probe_color>;


// =============================================================================
//  5.  DUCK-TYPED PROBES  (structural detection, no inheritance)
// =============================================================================

// duck_font
//   struct: the minimal font-like profile (family + size + weight + slant)
// with no relationship to font<> whatsoever.
struct duck_font
{
    std::string  family;
    float        size   = 12.0f;
    font_weight  weight = font_weight::normal;
    font_slant   slant  = font_slant::upright;
};

// duck_no_family / duck_no_size / duck_no_weight / duck_no_slant
//   struct: duck_font with exactly ONE of the four required members
// removed.  Four witnesses, one per conjunct of is_font_like.
struct duck_no_family
{
    float        size   = 12.0f;
    font_weight  weight = font_weight::normal;
    font_slant   slant  = font_slant::upright;
};

struct duck_no_size
{
    std::string  family;
    font_weight  weight = font_weight::normal;
    font_slant   slant  = font_slant::upright;
};

struct duck_no_weight
{
    std::string  family;
    float        size  = 12.0f;
    font_slant   slant = font_slant::upright;
};

struct duck_no_slant
{
    std::string  family;
    float        size   = 12.0f;
    font_weight  weight = font_weight::normal;
};

// duck_terminal
//   struct: font-like PLUS a foreground - the structural shape a terminal
// adapter accepts, reached without touching font<>.
struct duck_terminal
{
    std::string  family;
    float        size   = 12.0f;
    font_weight  weight = font_weight::normal;
    font_slant   slant  = font_slant::upright;
    probe_color  foreground {};
};

// duck_underline_only
//   struct: carries a decoration but is NOT font-like.  Separates
// has_font_underline (a member check) from is_font_with_decorations
// (a member check AND the font-like floor).
struct duck_underline_only
{
    bool  underline = false;
};

// duck_foreground_only
//   struct: carries a foreground but is NOT font-like.
struct duck_foreground_only
{
    probe_color  foreground {};
};

// duck_background_only
//   struct: a NOMINAL background - the field with no enable flag.  This is
// the false positive has_font_background's conjunction exists to reject.
struct duck_background_only
{
    probe_color  background {};
};

// duck_background_flag_only
//   struct: the enable flag with no color behind it - the other half.
struct duck_background_flag_only
{
    bool  background_enabled = false;
};

// duck_background_paired
//   struct: both halves; the only shape has_font_background accepts.
struct duck_background_paired
{
    probe_color  background {};
    bool         background_enabled = false;
};

// duck_script_only / duck_language_only / duck_script_paired
//   struct: the three halves-and-whole of has_font_script_hint's
// conjunction.
struct duck_script_only
{
    std::string  script_tag;
};

struct duck_language_only
{
    std::string  language_tag;
};

struct duck_script_paired
{
    std::string  script_tag;
    std::string  language_tag;
};

// duck_file_path_only
//   struct: ONE of the three backend handles.  has_font_backend_handles is
// a disjunction, so this must satisfy it while has_font_postscript_name and
// has_font_native_handle both stay false - something no font<> can witness,
// since its handles all arrive together in one mixin.
struct duck_file_path_only
{
    std::string  file_path;
};

// duck_native_handle_only
//   struct: a different single backend handle.
struct duck_native_handle_only
{
    void*  native_handle = nullptr;
};

// duck_not_a_font
//   struct: no font members at all.
struct duck_not_a_font
{
    int  unrelated = 0;
};


// =============================================================================
//  6.  COMPILE-TIME SWEEP HELPERS
// =============================================================================

// static_flags_agree
//   function: true when every font<_F>::has_* constant agrees with
// has_ff(_F, ...).  One expression covering all seventeen gates, so a new
// feature that forgets its constant is caught by every config in the sweep.
template<unsigned _Feat>
constexpr bool
static_flags_agree() noexcept
{
    using f = font<_Feat, probe_color>;

    return ( (f::features              == _Feat)                              &&
             (f::has_underline         == has_ff(_Feat, ff_underline))        &&
             (f::has_strikethrough     == has_ff(_Feat, ff_strikethrough))    &&
             (f::has_overline          == has_ff(_Feat, ff_overline))         &&
             (f::has_small_caps        == has_ff(_Feat, ff_small_caps))       &&
             (f::has_all_caps          == has_ff(_Feat, ff_all_caps))         &&
             (f::has_subscript         == has_ff(_Feat, ff_subscript))        &&
             (f::has_superscript       == has_ff(_Feat, ff_superscript))      &&
             (f::has_letter_spacing    == has_ff(_Feat, ff_letter_spacing))   &&
             (f::has_line_height       == has_ff(_Feat, ff_line_height))      &&
             (f::has_stretch           == has_ff(_Feat, ff_stretch))          &&
             (f::has_spacing           == has_ff(_Feat, ff_spacing))          &&
             (f::has_color             == has_ff(_Feat, ff_color))            &&
             (f::has_background        == has_ff(_Feat, ff_background))       &&
             (f::has_opentype_features == has_ff(_Feat, ff_opentype_features))&&
             (f::has_variable_axes     == has_ff(_Feat, ff_variable_axes))    &&
             (f::has_script_hint       == has_ff(_Feat, ff_script_hint))      &&
             (f::has_backend_handles   == has_ff(_Feat, ff_backend_handles)) );
}

// traits_track_the_flags
//   function: true when every structural has_font_* trait agrees with the
// feature flag that gates its member.  Ties section 10's detection back to
// section 1's bits for whatever _Feat the caller sweeps.
template<unsigned _Feat>
constexpr bool
traits_track_the_flags() noexcept
{
    using f = font<_Feat, probe_color>;

    return ( (has_font_underline_v<f>         == has_ff(_Feat, ff_underline))     &&
             (has_font_strikethrough_v<f>     == has_ff(_Feat, ff_strikethrough)) &&
             (has_font_overline_v<f>          == has_ff(_Feat, ff_overline))      &&
             (has_font_small_caps_v<f>        == has_ff(_Feat, ff_small_caps))    &&
             (has_font_all_caps_v<f>          == has_ff(_Feat, ff_all_caps))      &&
             (has_font_subscript_v<f>         == has_ff(_Feat, ff_subscript))     &&
             (has_font_superscript_v<f>       == has_ff(_Feat, ff_superscript))   &&
             (has_font_letter_spacing_v<f>    == has_ff(_Feat, ff_letter_spacing))&&
             (has_font_line_height_v<f>       == has_ff(_Feat, ff_line_height))   &&
             (has_font_stretch_v<f>           == has_ff(_Feat, ff_stretch))       &&
             (has_font_spacing_v<f>           == has_ff(_Feat, ff_spacing))       &&
             (has_font_color_v<f>             == has_ff(_Feat, ff_color))         &&
             (has_font_background_v<f>        == has_ff(_Feat, ff_background))    &&
             (has_font_opentype_features_v<f> ==
                                          has_ff(_Feat, ff_opentype_features))    &&
             (has_font_variable_axes_v<f>     == has_ff(_Feat, ff_variable_axes)) &&
             (has_font_script_hint_v<f>       == has_ff(_Feat, ff_script_hint))   &&
             (has_font_backend_handles_v<f>   ==
                                          has_ff(_Feat, ff_backend_handles)) );
}


// =============================================================================
//  7.  SFINAE DISPATCH PROBES  (section 10's enable_if_* helpers)
// =============================================================================
//   Each pair is a real overload set: the constrained overload takes an
// extra `int` and the fallback a `long`, so a literal 0 prefers the
// constrained one whenever its enable_if_* alias is well-formed.  This is
// the only way to show the helpers actually STEER overload resolution
// rather than merely evaluating to void.

// probe_font_like
//   function: 1 when enable_if_font_like<_Type> is well-formed, else 0.
template<typename _Type>
constexpr int
probe_font_like_impl(int, enable_if_font_like<_Type>* = nullptr) noexcept
{
    return 1;
}

template<typename>
constexpr int
probe_font_like_impl(long) noexcept
{
    return 0;
}

template<typename _Type>
constexpr int
probe_font_like() noexcept
{
    return probe_font_like_impl<_Type>(0);
}

// probe_underline
//   function: 1 when enable_if_has_font_underline<_Type> is well-formed.
template<typename _Type>
constexpr int
probe_underline_impl(int,
                     enable_if_has_font_underline<_Type>* = nullptr) noexcept
{
    return 1;
}

template<typename>
constexpr int
probe_underline_impl(long) noexcept
{
    return 0;
}

template<typename _Type>
constexpr int
probe_underline() noexcept
{
    return probe_underline_impl<_Type>(0);
}

// probe_strikethrough
//   function: 1 when enable_if_has_font_strikethrough<_Type> is well-formed.
template<typename _Type>
constexpr int
probe_strikethrough_impl(
    int,
    enable_if_has_font_strikethrough<_Type>* = nullptr
) noexcept
{
    return 1;
}

template<typename>
constexpr int
probe_strikethrough_impl(long) noexcept
{
    return 0;
}

template<typename _Type>
constexpr int
probe_strikethrough() noexcept
{
    return probe_strikethrough_impl<_Type>(0);
}

// probe_color_capable
//   function: 1 when enable_if_has_font_color<_Type> is well-formed.
template<typename _Type>
constexpr int
probe_color_capable_impl(int,
                         enable_if_has_font_color<_Type>* = nullptr) noexcept
{
    return 1;
}

template<typename>
constexpr int
probe_color_capable_impl(long) noexcept
{
    return 0;
}

template<typename _Type>
constexpr int
probe_color_capable() noexcept
{
    return probe_color_capable_impl<_Type>(0);
}

// probe_variable
//   function: 1 when enable_if_is_font_variable<_Type> is well-formed.
template<typename _Type>
constexpr int
probe_variable_impl(int,
                    enable_if_is_font_variable<_Type>* = nullptr) noexcept
{
    return 1;
}

template<typename>
constexpr int
probe_variable_impl(long) noexcept
{
    return 0;
}

template<typename _Type>
constexpr int
probe_variable() noexcept
{
    return probe_variable_impl<_Type>(0);
}

// probe_rich
//   function: 1 when enable_if_is_font_rich<_Type> is well-formed.
template<typename _Type>
constexpr int
probe_rich_impl(int, enable_if_is_font_rich<_Type>* = nullptr) noexcept
{
    return 1;
}

template<typename>
constexpr int
probe_rich_impl(long) noexcept
{
    return 0;
}

template<typename _Type>
constexpr int
probe_rich() noexcept
{
    return probe_rich_impl<_Type>(0);
}


// =============================================================================
//  8.  CONCEPT DISPATCH PROBES  (section 11; C++20 face only)
// =============================================================================

#if ( (defined(__cpp_concepts)) &&                                            \
      (__cpp_concepts >= 201907L) )

// concept_probe
//   function: an unconstrained overload and an underline_font-constrained
// one.  The constrained overload is more specialised, so it must win for
// any type that satisfies the concept - which is what "the concept face
// steers resolution" actually means.
template<typename _Type>
constexpr int
concept_probe() noexcept
{
    return 0;
}

template<underline_font _Type>
constexpr int
concept_probe() noexcept
{
    return 1;
}

// concept_conjunction_probe
//   function: two concepts composed in one requires-clause.  These concepts
// are ATOMIC (each wraps a distinct trait::value), so they never subsume one
// another - composing them with && is the supported way to demand both.
template<typename _Type>
constexpr bool
concept_conjunction_probe() noexcept
    requires ( underline_font<_Type> && casing_font<_Type> )
{
    return true;
}

template<typename _Type>
constexpr bool
concept_conjunction_probe() noexcept
{
    return false;
}

#endif  // __cpp_concepts >= 201907L


// =============================================================================
//  9.  TEST DECLARATIONS
// =============================================================================

// 1. feature flags  (font_feat, operator|, has_ff)
bool tests_ff_none_is_zero();
bool tests_ff_individual_bit_values();
bool tests_ff_flags_are_distinct_powers_of_two();
bool tests_ff_underlying_type_is_unsigned();
bool tests_ff_bits_fit_in_unsigned();
bool tests_ff_decorations_aggregate();
bool tests_ff_casing_aggregate();
bool tests_ff_metrics_aggregate();
bool tests_ff_axes_aggregate();
bool tests_ff_terminal_profiles();
bool tests_ff_gui_profiles();
bool tests_ff_gui_standard_omits_opentype_and_variable();
bool tests_ff_all_contains_every_flag();
bool tests_ff_all_equals_gui_rich();
bool tests_operator_or_combines_bits();
bool tests_operator_or_yields_unsigned();
bool tests_operator_or_is_constexpr_and_noexcept();
bool tests_operator_or_is_commutative_and_idempotent();
bool tests_has_ff_detects_set_bits();
bool tests_has_ff_rejects_unset_bits();
bool tests_has_ff_on_ff_none_is_always_false();
bool tests_has_ff_with_the_ff_none_bit_is_always_false();
bool tests_has_ff_on_aggregates();
bool tests_has_ff_is_constexpr_and_noexcept();

// 2. core enums  (font_weight, font_slant, font_stretch, font_spacing,
//                 font_size_unit)
bool tests_font_weight_underlying_type();
bool tests_font_weight_values();
bool tests_font_weight_is_monotonic();
bool tests_font_slant_underlying_type();
bool tests_font_slant_values();
bool tests_font_stretch_underlying_type();
bool tests_font_stretch_values();
bool tests_font_stretch_normal_is_the_midpoint();
bool tests_font_spacing_underlying_type();
bool tests_font_spacing_values();
bool tests_font_size_unit_underlying_type();
bool tests_font_size_unit_values();
bool tests_enums_are_scoped();
bool tests_enum_defaults_match_the_model();

// 3-6. value types  (font_color, ot_tag, opentype_feature, variable_axis,
//                    font_family_info)
bool tests_font_color_defaults();
bool tests_font_color_aggregate_init();
bool tests_font_color_equality();
bool tests_font_color_inequality_per_channel();
bool tests_font_color_equality_is_constexpr_and_noexcept();
bool tests_opentype_tag_is_uint32();
bool tests_ot_tag_packs_big_endian();
bool tests_ot_tag_known_tags();
bool tests_ot_tag_is_constexpr_and_noexcept();
bool tests_ot_tag_handles_high_bit_bytes();
bool tests_ot_tag_space_padding_is_the_callers_job();
bool tests_ot_tag_distinct_tags_are_distinct_values();
bool tests_opentype_feature_defaults();
bool tests_opentype_feature_aggregate_init();
bool tests_variable_axis_defaults();
bool tests_variable_axis_aggregate_init();
bool tests_font_family_info_defaults();
bool tests_font_family_info_population();
bool tests_font_family_info_scalable_is_the_odd_default();

// 7. EBO mixins
bool tests_mixin_disabled_are_empty();
bool tests_mixin_enabled_are_not_empty();
bool tests_mixin_decoration_defaults();
bool tests_mixin_casing_defaults();
bool tests_mixin_metrics_defaults();
bool tests_mixin_axes_defaults();
bool tests_mixin_color_default_is_value_initialised();
bool tests_mixin_background_defaults();
bool tests_mixin_opentype_features_default_empty();
bool tests_mixin_variable_axes_default_empty();
bool tests_mixin_script_hint_defaults_empty();
bool tests_mixin_backend_handles_defaults();
bool tests_mixin_color_type_is_parameterised();
bool tests_ebo_disabled_color_costs_nothing();
bool tests_ebo_disabled_color_costs_nothing_alongside_features();
bool tests_ebo_enabled_color_costs_its_weight();
bool tests_ebo_features_grow_the_type();
bool tests_ebo_profiles_are_ordered_by_payload();

// 8. the font struct
bool tests_font_default_member_values();
bool tests_font_default_color_type_is_rgb();
bool tests_font_color_type_alias_follows_the_parameter();
bool tests_font_features_constant_echoes_feat();
bool tests_font_static_flags_agree_with_has_ff();
bool tests_font_static_flags_are_constant_expressions();
bool tests_font_static_flags_all_false_on_ff_none();
bool tests_font_static_flags_all_true_on_ff_all();
bool tests_font_empty_on_default();
bool tests_font_empty_tracks_the_family_only();
bool tests_font_empty_is_const_and_noexcept();
bool tests_font_empty_after_clearing_the_family();
bool tests_font_family_ctor_is_explicit();
bool tests_font_family_size_ctor();
bool tests_font_family_size_weight_ctor();
bool tests_font_family_size_weight_slant_ctor();
bool tests_font_ctors_leave_the_other_members_default();
bool tests_font_is_copyable_and_movable();
bool tests_font_copy_is_deep();
bool tests_font_move_preserves_the_value();
bool tests_font_mixin_members_are_reachable();
bool tests_font_distinct_feature_sets_are_distinct_types();

// 9a. core free functions
bool tests_fn_set_family();
bool tests_fn_set_style_name();
bool tests_fn_set_size();
bool tests_fn_set_size_does_not_validate();
bool tests_fn_set_size_unit();
bool tests_fn_set_weight();
bool tests_fn_set_weight_clears_the_numeric_override();
bool tests_fn_set_weight_numeric();
bool tests_fn_set_weight_numeric_zero_restores_the_symbolic_weight();
bool tests_fn_effective_weight_prefers_the_numeric();
bool tests_fn_effective_weight_falls_back_to_the_enum();
bool tests_fn_effective_weight_honours_any_nonzero_numeric();
bool tests_fn_effective_weight_is_noexcept();
bool tests_fn_set_slant();
bool tests_fn_set_bold_true();
bool tests_fn_set_bold_false_forces_normal();
bool tests_fn_set_bold_clears_the_numeric_override();
bool tests_fn_is_bold_threshold_is_semi_bold();
bool tests_fn_is_bold_below_the_threshold();
bool tests_fn_is_bold_reads_the_effective_weight();
bool tests_fn_is_bold_numeric_boundary();
bool tests_fn_set_italic();
bool tests_fn_set_italic_false_from_oblique();
bool tests_fn_set_italic_true_overwrites_oblique();
bool tests_fn_is_italic_covers_italic_and_oblique();
bool tests_fn_bold_italic_round_trip();
bool tests_fn_core_ops_are_generic_over_the_feature_set();

// 9b. decorations / casing / metrics / axes / color
bool tests_fn_set_underline();
bool tests_fn_set_strikethrough();
bool tests_fn_set_overline();
bool tests_fn_decorations_are_independent();
bool tests_fn_set_small_caps();
bool tests_fn_set_all_caps();
bool tests_fn_set_subscript();
bool tests_fn_set_superscript();
bool tests_fn_casing_are_independent();
bool tests_fn_casing_permits_contradictory_combinations();
bool tests_fn_set_letter_spacing();
bool tests_fn_set_letter_spacing_accepts_negative();
bool tests_fn_set_line_height();
bool tests_fn_line_height_zero_means_adapter_default();
bool tests_fn_set_stretch();
bool tests_fn_set_spacing();
bool tests_fn_set_foreground();
bool tests_fn_set_foreground_with_a_custom_color_type();
bool tests_fn_set_background_enables_it();
bool tests_fn_clear_background_preserves_the_color();
bool tests_fn_clear_background_then_set_again();
bool tests_fn_clear_background_on_a_never_set_background();
bool tests_fn_foreground_and_background_are_independent();
bool tests_fn_color_ops_leave_the_core_alone();

// 9c. opentype features / variable axes / script hint / backend handles
bool tests_fn_set_opentype_feature_appends();
bool tests_fn_set_opentype_feature_defaults_the_value_to_one();
bool tests_fn_set_opentype_feature_upserts();
bool tests_fn_set_opentype_feature_preserves_insertion_order();
bool tests_fn_set_opentype_feature_zero_stores_a_disabled_entry();
bool tests_fn_get_opentype_feature_returns_zero_when_absent();
bool tests_fn_get_opentype_feature_conflates_absent_and_disabled();
bool tests_fn_remove_opentype_feature_reports_the_hit();
bool tests_fn_remove_opentype_feature_reports_the_miss();
bool tests_fn_remove_opentype_feature_removes_only_the_match();
bool tests_fn_remove_opentype_feature_keeps_the_order();
bool tests_fn_remove_opentype_feature_on_an_empty_list();
bool tests_fn_opentype_features_scale();
bool tests_fn_set_variable_axis_appends();
bool tests_fn_set_variable_axis_upserts();
bool tests_fn_get_variable_axis_returns_the_default_when_absent();
bool tests_fn_get_variable_axis_takes_a_caller_default();
bool tests_fn_get_variable_axis_stored_zero_beats_the_default();
bool tests_fn_variable_axis_accepts_the_whole_float_range();
bool tests_fn_variable_axes_preserve_insertion_order();
bool tests_fn_set_script_sets_both_tags();
bool tests_fn_set_script_defaults_the_language_to_empty();
bool tests_fn_set_script_wipes_a_previously_set_language();
bool tests_fn_set_file_path_sets_the_path_and_the_index();
bool tests_fn_set_file_path_defaults_the_face_index_to_zero();
bool tests_fn_set_file_path_resets_a_previously_set_face_index();
bool tests_fn_backend_handles_without_setters_are_plain_members();

// 9d. fn_convert_size
bool tests_convert_identity_for_every_unit();
bool tests_convert_points_to_pixels();
bool tests_convert_pixels_to_points();
bool tests_convert_points_to_pixels_honours_the_dpi();
bool tests_convert_pixels_to_points_honours_the_dpi();
bool tests_convert_em_to_points();
bool tests_convert_points_to_em();
bool tests_convert_percent_to_points();
bool tests_convert_points_to_percent();
bool tests_convert_em_to_percent();
bool tests_convert_percent_to_em();
bool tests_convert_em_to_pixels();
bool tests_convert_pixels_to_em();
bool tests_convert_percent_to_pixels();
bool tests_convert_pixels_to_percent();
bool tests_convert_device_units_as_source_is_opaque();
bool tests_convert_device_units_as_target_is_opaque();
bool tests_convert_zero_em_pts_guards_the_em_target();
bool tests_convert_zero_em_pts_guards_the_percent_target();
bool tests_convert_zero_em_pts_is_unguarded_on_the_source_side();
bool tests_convert_round_trips();
bool tests_convert_the_whole_unit_matrix();
bool tests_convert_zero_and_negative_values();
bool tests_convert_default_arguments();
bool tests_convert_is_constexpr_and_noexcept();

// 10a. traits: detectors + capability traits
bool tests_detector_core_member_types();
bool tests_detector_yields_the_declared_type();
bool tests_detector_feature_member_types();
bool tests_detector_empty_returns_bool();
bool tests_detector_color_type_alias();
bool tests_detectors_are_const_and_reference_tolerant();
bool tests_has_font_core_traits_on_font();
bool tests_has_font_core_traits_reject_non_fonts();
bool tests_has_font_decoration_traits_track_the_flags();
bool tests_has_font_decorations_is_a_disjunction();
bool tests_has_font_casing_traits_track_the_flags();
bool tests_has_font_casing_is_a_disjunction();
bool tests_has_font_metrics_traits_track_the_flags();
bool tests_has_font_metrics_overrides_is_a_disjunction();
bool tests_has_font_axis_traits_track_the_flags();
bool tests_has_font_color_tracks_the_flag();
bool tests_has_font_background_requires_both_members();
bool tests_has_font_opentype_and_variable_track_the_flags();
bool tests_has_font_script_hint_requires_both_tags();
bool tests_has_font_backend_traits_track_the_flags();
bool tests_has_font_backend_handles_is_a_disjunction();
bool tests_has_font_color_type_alias_is_independent_of_ff_color();
bool tests_traits_track_the_flags_across_the_lattice();
bool tests_trait_v_aliases_agree_with_value();
bool tests_traits_derive_from_integral_constant();

// 10b. traits: composite profiles + SFINAE helpers
bool tests_is_font_like_across_the_lattice();
bool tests_is_font_like_on_a_duck_typed_struct();
bool tests_is_font_like_needs_the_family();
bool tests_is_font_like_needs_the_size();
bool tests_is_font_like_needs_the_weight();
bool tests_is_font_like_needs_the_slant();
bool tests_is_font_like_rejects_non_fonts();
bool tests_is_font_bold_like_is_is_font_like();
bool tests_is_font_italic_like_is_is_font_like();
bool tests_is_font_with_decorations();
bool tests_is_font_with_decorations_needs_the_font_like_floor();
bool tests_is_font_with_casing();
bool tests_is_font_with_color();
bool tests_is_font_with_background();
bool tests_is_font_with_metrics();
bool tests_is_font_with_opentype();
bool tests_is_font_variable();
bool tests_is_font_terminal_accepts_color_or_background();
bool tests_is_font_terminal_needs_the_font_like_floor();
bool tests_is_font_rich_on_the_rich_profile();
bool tests_is_font_rich_rejects_gui_standard();
bool tests_is_font_rich_rejects_gui_basic();
bool tests_is_font_rich_has_a_minimal_satisfying_set();
bool tests_composite_v_aliases_agree_with_value();
bool tests_enable_if_helpers_steer_overload_resolution();
bool tests_enable_if_helpers_are_void_on_success();
bool tests_enable_if_helpers_are_ill_formed_on_failure();

// 11. concepts  (C++20 face)
bool tests_concept_gate_matches_the_compiler();
bool tests_concept_core_identity();
bool tests_concept_font_like_type();
bool tests_concept_decorations();
bool tests_concept_decorated_font_is_a_disjunction();
bool tests_concept_casing();
bool tests_concept_casing_font_is_a_disjunction();
bool tests_concept_metrics();
bool tests_concept_axes();
bool tests_concept_colors();
bool tests_concept_color_typed_font_is_independent_of_ff_color();
bool tests_concept_opentype_and_variable();
bool tests_concept_script_hint();
bool tests_concept_backend();
bool tests_concept_backend_resolvable_is_a_disjunction();
bool tests_concept_composite_profiles();
bool tests_concept_terminal_and_rich();
bool tests_concepts_agree_with_their_traits();
bool tests_concept_constrained_overload_is_preferred();
bool tests_concept_conjunction_in_a_requires_clause();
bool tests_concepts_reject_non_fonts();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEXT_FONT_TESTS_
