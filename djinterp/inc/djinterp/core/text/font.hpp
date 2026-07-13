/******************************************************************************
* djinterp [text]                                                      font.hpp
*
* Abstract font model:
*   A framework-agnostic, OS-agnostic typography model.  Defines the
* symbolic identity, axes, decorations, metrics, and variable-axis
* state a renderer or dialog needs to describe a font, with zero
* assumption about the underlying technology (FreeType, DirectWrite,
* CoreText, fontconfig, bitmap atlases, terminal SGR, SVG, ...).
*
*   The core type `font<_Feat, _ColorType>` is a compile-time
* configurable data aggregate.  Capabilities that not every platform
* supports - underline, strikethrough, color, small caps, variable-
* font axes, OpenType feature tags - are gated behind feature-flag
* EBO mixins, matching the pattern used by button, input_control,
* and dialog elsewhere in uxoxo.  Disabled capabilities consume zero
* bytes (EBO folds the empty mixin into the outer struct).
*
*   Fundamental identity is always present:
*     - family name
*     - size (in a configurable unit)
*     - weight (symbolic + numeric)
*     - slant (upright / italic / oblique)
*
*   Everything else - underline, strikethrough, overline, small caps,
* stretch, letter spacing, line height, foreground color, background
* color, OpenType feature list, variable-axis values, language /
* script hint - is optional.
*
*   This module intentionally does NOT:
*     - open or enumerate files
*     - measure glyph advances or shape text
*     - touch the filesystem, GDI, FreeType, or any platform API
*     - prescribe a rendering pipeline
*
*   Backends map `font<>` onto their native representation; the
* font_traits module provides structural detection so adapters can
* branch with `if constexpr (has_underline_v<F>)`.
*
*
* path:      /inc/djinterp/core/text/font.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  feature flags (font_feat)
2.  core enums (font_weight,
                font_slant,
                font_stretch,
                font_spacing,
                font_size_unit)
3.  color type
4.  openType feature tags (fourcc-style)
5.  variable-font axis
6.  font family info (catalogue entry)
7.  EBO mixins
8.  font<_Feat, _ColorType> struct
9.  free functions (fn_set_*, 
                    fn_clear_*, 
                    fn_has_*, 
                    fn_convert_size)
10. font traits (consolidated from font_traits.hpp)
11. font concepts (consolidated from font_concepts.hpp)
*/

#ifndef DJINTERP_TEXT_FONT_
#define DJINTERP_TEXT_FONT_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"      // is_detected<> (used by the font traits)
#include "../util/color/color_rgb.hpp"  // native rgb (default font color type)


NS_DJINTERP

// =============================================================================
//  1.  FONT FEATURE FLAGS
// =============================================================================
//   Each flag gates a data member or mixin in `font<_Feat>`.  A flag
// costs nothing when disabled (empty base optimization folds the
// corresponding mixin into the outer type).
//
//   Rationale for the partitions:
//     - "core" (always present): family, size, weight, slant
//     - "decorations": underline, strikethrough, overline - commonly
//       absent on terminals and low-level glyph pipelines
//     - "casing": small caps, all caps, subscript, superscript
//     - "metrics overrides": letter_spacing, line_height
//     - "stretch + spacing axes": horizontal width, monospace hint
//     - "color": foreground RGBA (parameterised color type)
//     - "background": a distinct background-fill color
//     - "opentype_features": fourcc-tagged feature list (e.g. "liga")
//     - "variable": variable-font axis list (wght, wdth, slnt, ital, ...)
//     - "script_hint": language / script tag (for shaping selection)
//     - "backend_handles": PostScript name, file path, face index,
//       opaque native handle - resolution hints for adapters

enum font_feat : unsigned
{
    ff_none              = 0,
    ff_underline         = 1u << 0,
    ff_strikethrough     = 1u << 1,
    ff_overline          = 1u << 2,
    ff_small_caps        = 1u << 3,
    ff_all_caps          = 1u << 4,
    ff_subscript         = 1u << 5,
    ff_superscript       = 1u << 6,
    ff_letter_spacing    = 1u << 7,
    ff_line_height       = 1u << 8,
    ff_stretch           = 1u << 9,
    ff_spacing           = 1u << 10,
    ff_color             = 1u << 11,
    ff_background        = 1u << 12,
    ff_opentype_features = 1u << 13,
    ff_variable_axes     = 1u << 14,
    ff_script_hint       = 1u << 15,
    ff_backend_handles   = 1u << 16,

    // useful aggregates
    ff_decorations       = ff_underline | ff_strikethrough | ff_overline,
    ff_casing            = ff_small_caps | ff_all_caps
                         | ff_subscript | ff_superscript,
    ff_metrics           = ff_letter_spacing | ff_line_height,
    ff_axes              = ff_stretch | ff_spacing,

    // common platform profiles
    ff_terminal_basic    = ff_color | ff_background,
    ff_terminal_rich     = ff_color | ff_background
                         | ff_underline | ff_strikethrough,
    ff_gui_basic         = ff_decorations | ff_metrics | ff_color,
    ff_gui_standard      = ff_decorations | ff_casing | ff_metrics
                         | ff_axes | ff_color | ff_background
                         | ff_backend_handles,
    ff_gui_rich          = ff_decorations | ff_casing | ff_metrics
                         | ff_axes | ff_color | ff_background
                         | ff_opentype_features | ff_variable_axes
                         | ff_script_hint | ff_backend_handles,

    ff_all               = ff_decorations | ff_casing | ff_metrics
                         | ff_axes | ff_color | ff_background
                         | ff_opentype_features | ff_variable_axes
                         | ff_script_hint | ff_backend_handles
};

constexpr unsigned operator|(font_feat _a,
                             font_feat _b) noexcept
{
    return static_cast<unsigned>(_a) | static_cast<unsigned>(_b);
}

constexpr bool has_ff(unsigned   _f,
                      font_feat  _bit) noexcept
{
    return (_f & static_cast<unsigned>(_bit)) != 0;
}


// =============================================================================
//  2.  CORE ENUMS
// =============================================================================

// font_weight
//   enum: symbolic weight names with numeric values matching the OS/2
// usWeightClass scale and the CSS font-weight keywords.  A custom
// numeric weight is carried in `font::weight_numeric`, which adapters
// should prefer when nonzero.
enum class font_weight : std::uint16_t
{
    thin        = 100,
    extra_light = 200,
    light       = 300,
    normal      = 400,
    medium      = 500,
    semi_bold   = 600,
    bold        = 700,
    extra_bold  = 800,
    black       = 900,
    extra_black = 950
};

// font_slant
//   enum: italic / oblique axis.  `oblique` is algorithmically slanted
// while `italic` is typographically designed - adapters that cannot
// distinguish may treat them as equivalent.
enum class font_slant : std::uint8_t
{
    upright,
    italic,
    oblique
};

// font_stretch
//   enum: horizontal-scale axis, matching the OS/2 usWidthClass range.
// Only carried when ff_stretch is enabled.
enum class font_stretch : std::uint8_t
{
    ultra_condensed = 1,
    extra_condensed = 2,
    condensed       = 3,
    semi_condensed  = 4,
    normal          = 5,
    semi_expanded   = 6,
    expanded        = 7,
    extra_expanded  = 8,
    ultra_expanded  = 9
};

// font_spacing
//   enum: how glyph advance widths are distributed.  Carried only
// when ff_spacing is enabled.
enum class font_spacing : std::uint8_t
{
    any,              // adapter should not filter or care
    proportional,
    monospace,
    dual_width,       // CJK half/full-width
    charcell          // terminal-grid cells only
};

// font_size_unit
//   enum: unit the numeric `size` field is expressed in.  Backends
// convert to device-native units at resolution time; `fn_convert_size`
// offers a no-op-agnostic helper for the common case.
enum class font_size_unit : std::uint8_t
{
    points,           // 1/72 inch
    pixels,
    em,
    percent,
    device_units      // already resolved
};


// =============================================================================
//  3.  COLOR TYPE
// =============================================================================

// font_color
//   struct: a legacy 8-bit RGBA color, retained so existing
// `font<F, font_color>` instantiations keep compiling.  The font's
// default color type is now the native djinterp::rgb; prefer that (or
// djinterp::rgba for alpha).  Kept here as a self-contained option.
struct font_color
{
    std::uint8_t  r = 0;
    std::uint8_t  g = 0;
    std::uint8_t  b = 0;
    std::uint8_t  a = 255;

    constexpr bool
    operator==(const font_color& _o) const noexcept
    {
        return ( (r == _o.r) && (g == _o.g) &&
                 (b == _o.b) && (a == _o.a) );
    }
};


// =============================================================================
//  4.  OPENTYPE FEATURE TAGS
// =============================================================================

// opentype_tag
//   type: 4-character OpenType feature tag packed into a uint32.
// Use `ot_tag("liga")` at compile time; adapters unpack for their
// native API (hb_tag_t, DWRITE_FONT_FEATURE_TAG, CTFontFeatureTag).
using opentype_tag = std::uint32_t;

// ot_tag
//   function: packs a 4-character string into an opentype_tag.
// `_s` must be exactly 4 ASCII bytes; shorter strings are padded
// with space (0x20) in OpenType convention.
constexpr opentype_tag
ot_tag(
    const char _s[5]
) noexcept
{
    // big-endian packing, matching the on-disk OpenType format
    return ( (static_cast<opentype_tag>(
                static_cast<unsigned char>(_s[0])) << 24)
           | (static_cast<opentype_tag>(
                static_cast<unsigned char>(_s[1])) << 16)
           | (static_cast<opentype_tag>(
                static_cast<unsigned char>(_s[2])) <<  8)
           | (static_cast<opentype_tag>(
                static_cast<unsigned char>(_s[3]))) );
}

// opentype_feature
//   struct: one OpenType feature entry.  Most features are boolean
// (value 0 or 1); some are integer-valued (e.g. `salt` salt variant
// 1..N, `ss01`..`ss20` stylistic sets).  `value = 0` disables the
// feature; nonzero enables or selects a variant.
struct opentype_feature
{
    opentype_tag   tag    = 0;
    std::uint32_t  value  = 1;
};


// =============================================================================
//  5.  VARIABLE-FONT AXIS
// =============================================================================

// variable_axis
//   struct: one variable-font axis value.  The tag identifies the
// axis in OpenType convention: "wght" weight, "wdth" width, "slnt"
// slant, "ital" italic, "opsz" optical size, plus any vendor-
// registered custom axes.
struct variable_axis
{
    opentype_tag  tag   = 0;
    float         value = 0.0f;
};


// =============================================================================
//  6.  FONT FAMILY INFO
// =============================================================================

// font_family_info
//   struct: one entry in the catalogue of available families.  Kept
// here (rather than in the dialog) so that non-dialog consumers -
// font managers, asset pipelines, debug overlays - share one type.
struct font_family_info
{
    std::string               family;
    std::vector<std::string>  styles;              // "Regular", "Bold", ...
    std::vector<float>        fixed_sizes;         // empty => scalable
    bool                      is_scalable   = true;
    bool                      is_monospace  = false;
    bool                      is_symbol     = false;
    bool                      is_variable   = false;   // OpenType variable
    std::vector<std::string>  writing_systems;     // "Latin", "Cyrillic", ...
    std::string               foundry;
    std::uint32_t             coverage_bits = 0;
};


// =============================================================================
//  7.  EBO MIXINS
// =============================================================================
//   Each mixin stores exactly the data its feature introduces.  The
// `_Enable == false` specialisation is empty and subject to EBO.

namespace font_mixin {

    // -- decorations --------------------------------------------------

    template <bool _Enable>
    struct underline_data
    {};

    template <>
    struct underline_data<true>
    {
        bool underline = false;
    };

    template <bool _Enable>
    struct strikethrough_data
    {};

    template <>
    struct strikethrough_data<true>
    {
        bool strikethrough = false;
    };

    template <bool _Enable>
    struct overline_data
    {};

    template <>
    struct overline_data<true>
    {
        bool overline = false;
    };

    // -- casing -------------------------------------------------------

    template <bool _Enable>
    struct small_caps_data
    {};

    template <>
    struct small_caps_data<true>
    {
        bool small_caps = false;
    };

    template <bool _Enable>
    struct all_caps_data
    {};

    template <>
    struct all_caps_data<true>
    {
        bool all_caps = false;
    };

    template <bool _Enable>
    struct subscript_data
    {};

    template <>
    struct subscript_data<true>
    {
        bool subscript = false;
    };

    template <bool _Enable>
    struct superscript_data
    {};

    template <>
    struct superscript_data<true>
    {
        bool superscript = false;
    };

    // -- metrics overrides --------------------------------------------

    template <bool _Enable>
    struct letter_spacing_data
    {};

    template <>
    struct letter_spacing_data<true>
    {
        float letter_spacing = 0.0f;    // em units
    };

    template <bool _Enable>
    struct line_height_data
    {};

    template <>
    struct line_height_data<true>
    {
        float line_height = 0.0f;       // multiplier; 0 => adapter default
    };

    // -- axes ---------------------------------------------------------

    template <bool _Enable>
    struct stretch_data
    {};

    template <>
    struct stretch_data<true>
    {
        font_stretch stretch = font_stretch::normal;
    };

    template <bool _Enable>
    struct spacing_data
    {};

    template <>
    struct spacing_data<true>
    {
        font_spacing spacing = font_spacing::any;
    };

    // -- color --------------------------------------------------------

    template <bool _Enable, typename _ColorType>
    struct color_data
    {};

    template <typename _ColorType>
    struct color_data<true, _ColorType>
    {
        _ColorType foreground {};
    };

    template <bool _Enable, typename _ColorType>
    struct background_data
    {};

    template <typename _ColorType>
    struct background_data<true, _ColorType>
    {
        _ColorType background {};
        bool       background_enabled = false;
    };

    // -- opentype features --------------------------------------------

    template <bool _Enable>
    struct opentype_features_data
    {};

    template <>
    struct opentype_features_data<true>
    {
        std::vector<opentype_feature> opentype_features;
    };

    // -- variable-font axes -------------------------------------------

    template <bool _Enable>
    struct variable_axes_data
    {};

    template <>
    struct variable_axes_data<true>
    {
        std::vector<variable_axis> variable_axes;
    };

    // -- script hint --------------------------------------------------

    template <bool _Enable>
    struct script_hint_data
    {};

    template <>
    struct script_hint_data<true>
    {
        std::string  script_tag;         // "latn", "cyrl", "hani", ...
        std::string  language_tag;       // BCP-47: "en", "zh-Hans", ...
    };

    // -- backend handles ----------------------------------------------

    template <bool _Enable>
    struct backend_handles_data
    {};

    template <>
    struct backend_handles_data<true>
    {
        std::string  postscript_name;
        std::string  full_name;          // e.g. "Inter Regular"
        std::string  file_path;
        int          face_index    = 0;  // TTC / OTC face index
        void*        native_handle = nullptr;
    };

}   // namespace font_mixin


// =============================================================================
//  8.  FONT
// =============================================================================

// font
//   struct: framework-agnostic font descriptor.  Carries symbolic
// identity plus any compile-time-enabled axes, decorations, or
// backend hints.
//
//   _Feat       bitwise OR of font_feat values.  Gates mixins.
//   _ColorType  color type used by the foreground/background mixins.
//               Defaults to the native djinterp::rgb.  Substitute any
//               type that is default-constructible, copyable, and
//               equality-comparable (e.g. djinterp::rgba for alpha, or
//               the legacy font_color).
//
//   Always-present members:
//     family            std::string         display name
//     style_name        std::string         e.g. "Bold Italic"
//     size              float               numeric size
//     size_unit         font_size_unit      size interpretation
//     weight            font_weight         symbolic weight
//     weight_numeric    std::uint16_t       0 => use `weight` enum;
//                                             nonzero wins
//     slant             font_slant          upright / italic / oblique

template <unsigned _Feat      = ff_none,
          typename _ColorType = rgb>
struct font
    : font_mixin::underline_data         <has_ff(_Feat, ff_underline)>
    , font_mixin::strikethrough_data     <has_ff(_Feat, ff_strikethrough)>
    , font_mixin::overline_data          <has_ff(_Feat, ff_overline)>
    , font_mixin::small_caps_data        <has_ff(_Feat, ff_small_caps)>
    , font_mixin::all_caps_data          <has_ff(_Feat, ff_all_caps)>
    , font_mixin::subscript_data         <has_ff(_Feat, ff_subscript)>
    , font_mixin::superscript_data       <has_ff(_Feat, ff_superscript)>
    , font_mixin::letter_spacing_data    <has_ff(_Feat, ff_letter_spacing)>
    , font_mixin::line_height_data       <has_ff(_Feat, ff_line_height)>
    , font_mixin::stretch_data           <has_ff(_Feat, ff_stretch)>
    , font_mixin::spacing_data           <has_ff(_Feat, ff_spacing)>
    , font_mixin::color_data             <has_ff(_Feat, ff_color),
                                          _ColorType>
    , font_mixin::background_data        <has_ff(_Feat, ff_background),
                                          _ColorType>
    , font_mixin::opentype_features_data <has_ff(_Feat, ff_opentype_features)>
    , font_mixin::variable_axes_data     <has_ff(_Feat, ff_variable_axes)>
    , font_mixin::script_hint_data       <has_ff(_Feat, ff_script_hint)>
    , font_mixin::backend_handles_data   <has_ff(_Feat, ff_backend_handles)>
{
    using color_type = _ColorType;

    static constexpr unsigned features            = _Feat;
    static constexpr bool has_underline           = has_ff(_Feat, ff_underline);
    static constexpr bool has_strikethrough       = has_ff(_Feat, ff_strikethrough);
    static constexpr bool has_overline            = has_ff(_Feat, ff_overline);
    static constexpr bool has_small_caps          = has_ff(_Feat, ff_small_caps);
    static constexpr bool has_all_caps            = has_ff(_Feat, ff_all_caps);
    static constexpr bool has_subscript           = has_ff(_Feat, ff_subscript);
    static constexpr bool has_superscript         = has_ff(_Feat, ff_superscript);
    static constexpr bool has_letter_spacing      = has_ff(_Feat, ff_letter_spacing);
    static constexpr bool has_line_height         = has_ff(_Feat, ff_line_height);
    static constexpr bool has_stretch             = has_ff(_Feat, ff_stretch);
    static constexpr bool has_spacing             = has_ff(_Feat, ff_spacing);
    static constexpr bool has_color               = has_ff(_Feat, ff_color);
    static constexpr bool has_background          = has_ff(_Feat, ff_background);
    static constexpr bool has_opentype_features   = has_ff(_Feat, ff_opentype_features);
    static constexpr bool has_variable_axes       = has_ff(_Feat, ff_variable_axes);
    static constexpr bool has_script_hint         = has_ff(_Feat, ff_script_hint);
    static constexpr bool has_backend_handles     = has_ff(_Feat, ff_backend_handles);

    // -- core (always present) ----------------------------------------
    std::string       family;
    std::string       style_name;
    float             size            = 10.0f;
    font_size_unit    size_unit       = font_size_unit::points;
    font_weight       weight          = font_weight::normal;
    std::uint16_t     weight_numeric  = 0;
    font_slant        slant           = font_slant::upright;

    // -- introspection ------------------------------------------------
    [[nodiscard]] bool
    empty() const noexcept
    {
        return family.empty();
    }

    // -- construction -------------------------------------------------
    font() = default;

    explicit font(
            std::string _family
        )
            : family(std::move(_family))
        {}

    font(
            std::string  _family,
            float        _size
        )
            : family(std::move(_family)),
              size(_size)
        {}

    font(
            std::string  _family,
            float        _size,
            font_weight  _weight,
            font_slant   _slant = font_slant::upright
        )
            : family(std::move(_family)),
              size(_size),
              weight(_weight),
              slant(_slant)
        {}
};


// =============================================================================
//  9.  FREE FUNCTIONS
// =============================================================================

// ---------------------------------------------------------------------
// core setters
// ---------------------------------------------------------------------

// fn_set_family
template <unsigned _F, typename _C>
void
fn_set_family(
    font<_F, _C>&  _fn,
    std::string    _family
)
{
    _fn.family = std::move(_family);

    return;
}

// fn_set_style_name
template <unsigned _F, typename _C>
void
fn_set_style_name(
    font<_F, _C>&  _fn,
    std::string    _style
)
{
    _fn.style_name = std::move(_style);

    return;
}

// fn_set_size
template <unsigned _F, typename _C>
void
fn_set_size(
    font<_F, _C>&  _fn,
    float          _size
)
{
    _fn.size = _size;

    return;
}

// fn_set_size_unit
template <unsigned _F, typename _C>
void
fn_set_size_unit(
    font<_F, _C>&   _fn,
    font_size_unit  _unit
)
{
    _fn.size_unit = _unit;

    return;
}

// fn_set_weight
//   sets the symbolic weight and clears any numeric override.
template <unsigned _F, typename _C>
void
fn_set_weight(
    font<_F, _C>&  _fn,
    font_weight    _w
)
{
    _fn.weight         = _w;
    _fn.weight_numeric = 0;

    return;
}

// fn_set_weight_numeric
//   sets the numeric weight override.  `0` clears the override so
// the symbolic `weight` field takes effect again.
template <unsigned _F, typename _C>
void
fn_set_weight_numeric(
    font<_F, _C>&  _fn,
    std::uint16_t  _w
)
{
    _fn.weight_numeric = _w;

    return;
}

// fn_effective_weight
//   returns the weight that should be applied by a renderer:
// weight_numeric when nonzero, else the numeric of `weight`.
template <unsigned _F, typename _C>
std::uint16_t
fn_effective_weight(
    const font<_F, _C>& _fn
) noexcept
{
    if (_fn.weight_numeric != 0)
    {
        return _fn.weight_numeric;
    }

    return static_cast<std::uint16_t>(_fn.weight);
}

// fn_set_slant
template <unsigned _F, typename _C>
void
fn_set_slant(
    font<_F, _C>&  _fn,
    font_slant     _s
)
{
    _fn.slant = _s;

    return;
}

// fn_set_bold
//   convenience: sets weight to bold (700) when `_on`, normal (400)
// when off.  Clears any numeric override so the change takes effect.
template <unsigned _F, typename _C>
void
fn_set_bold(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    _fn.weight         = _on ? font_weight::bold : font_weight::normal;
    _fn.weight_numeric = 0;

    return;
}

// fn_is_bold
template <unsigned _F, typename _C>
bool
fn_is_bold(
    const font<_F, _C>& _fn
) noexcept
{
    return (fn_effective_weight(_fn) >= static_cast<std::uint16_t>(
        font_weight::semi_bold));
}

// fn_set_italic
//   convenience: sets slant to italic when `_on`, upright when off.
template <unsigned _F, typename _C>
void
fn_set_italic(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    _fn.slant = _on ? font_slant::italic : font_slant::upright;

    return;
}

// fn_is_italic
template <unsigned _F, typename _C>
bool
fn_is_italic(
    const font<_F, _C>& _fn
) noexcept
{
    return ( (_fn.slant == font_slant::italic) ||
             (_fn.slant == font_slant::oblique) );
}


// ---------------------------------------------------------------------
// decorations
// ---------------------------------------------------------------------
//   Each decoration setter is a static_assert-guarded function that
// fails cleanly at compile time if the feature flag wasn't enabled.

template <unsigned _F, typename _C>
void
fn_set_underline(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_underline),
                  "requires ff_underline");

    _fn.underline = _on;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_strikethrough(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_strikethrough),
                  "requires ff_strikethrough");

    _fn.strikethrough = _on;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_overline(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_overline),
                  "requires ff_overline");

    _fn.overline = _on;

    return;
}


// ---------------------------------------------------------------------
// casing
// ---------------------------------------------------------------------

template <unsigned _F, typename _C>
void
fn_set_small_caps(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_small_caps),
                  "requires ff_small_caps");

    _fn.small_caps = _on;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_all_caps(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_all_caps),
                  "requires ff_all_caps");

    _fn.all_caps = _on;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_subscript(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_subscript),
                  "requires ff_subscript");

    _fn.subscript = _on;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_superscript(
    font<_F, _C>&  _fn,
    bool           _on
)
{
    static_assert(has_ff(_F, ff_superscript),
                  "requires ff_superscript");

    _fn.superscript = _on;

    return;
}


// ---------------------------------------------------------------------
// metrics overrides
// ---------------------------------------------------------------------

template <unsigned _F, typename _C>
void
fn_set_letter_spacing(
    font<_F, _C>&  _fn,
    float          _ems
)
{
    static_assert(has_ff(_F, ff_letter_spacing),
                  "requires ff_letter_spacing");

    _fn.letter_spacing = _ems;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_line_height(
    font<_F, _C>&  _fn,
    float          _multiplier
)
{
    static_assert(has_ff(_F, ff_line_height),
                  "requires ff_line_height");

    _fn.line_height = _multiplier;

    return;
}


// ---------------------------------------------------------------------
// axes
// ---------------------------------------------------------------------

template <unsigned _F, typename _C>
void
fn_set_stretch(
    font<_F, _C>&  _fn,
    font_stretch   _s
)
{
    static_assert(has_ff(_F, ff_stretch),
                  "requires ff_stretch");

    _fn.stretch = _s;

    return;
}

template <unsigned _F, typename _C>
void
fn_set_spacing(
    font<_F, _C>&  _fn,
    font_spacing   _s
)
{
    static_assert(has_ff(_F, ff_spacing),
                  "requires ff_spacing");

    _fn.spacing = _s;

    return;
}


// ---------------------------------------------------------------------
// color / background
// ---------------------------------------------------------------------

template <unsigned _F, typename _C>
void
fn_set_foreground(
    font<_F, _C>&  _fn,
    _C             _color
)
{
    static_assert(has_ff(_F, ff_color),
                  "requires ff_color");

    _fn.foreground = std::move(_color);

    return;
}

template <unsigned _F, typename _C>
void
fn_set_background(
    font<_F, _C>&  _fn,
    _C             _color
)
{
    static_assert(has_ff(_F, ff_background),
                  "requires ff_background");

    _fn.background         = std::move(_color);
    _fn.background_enabled = true;

    return;
}

template <unsigned _F, typename _C>
void
fn_clear_background(
    font<_F, _C>& _fn
)
{
    static_assert(has_ff(_F, ff_background),
                  "requires ff_background");

    _fn.background_enabled = false;

    return;
}


// ---------------------------------------------------------------------
// opentype features
// ---------------------------------------------------------------------

// fn_set_opentype_feature
//   sets or replaces a feature tag's value.  Passing `0` as the value
// disables the feature (idiomatic for booleans like `liga`).
template <unsigned _F, typename _C>
void
fn_set_opentype_feature(
    font<_F, _C>&   _fn,
    opentype_tag    _tag,
    std::uint32_t   _value = 1
)
{
    static_assert(has_ff(_F, ff_opentype_features),
                  "requires ff_opentype_features");

    for (auto& e : _fn.opentype_features)
    {
        if (e.tag == _tag)
        {
            e.value = _value;

            return;
        }
    }

    _fn.opentype_features.push_back({_tag, _value});

    return;
}

// fn_remove_opentype_feature
template <unsigned _F, typename _C>
bool
fn_remove_opentype_feature(
    font<_F, _C>&   _fn,
    opentype_tag    _tag
)
{
    static_assert(has_ff(_F, ff_opentype_features),
                  "requires ff_opentype_features");

    for (auto it = _fn.opentype_features.begin();
         it != _fn.opentype_features.end();
         ++it)
    {
        if (it->tag == _tag)
        {
            _fn.opentype_features.erase(it);

            return true;
        }
    }

    return false;
}

// fn_get_opentype_feature
//   returns the current value for `_tag`, or 0 if unset.
template <unsigned _F, typename _C>
std::uint32_t
fn_get_opentype_feature(
    const font<_F, _C>&  _fn,
    opentype_tag         _tag
)
{
    static_assert(has_ff(_F, ff_opentype_features),
                  "requires ff_opentype_features");

    for (const auto& e : _fn.opentype_features)
    {
        if (e.tag == _tag)
        {
            return e.value;
        }
    }

    return 0;
}


// ---------------------------------------------------------------------
// variable-font axes
// ---------------------------------------------------------------------

// fn_set_variable_axis
template <unsigned _F, typename _C>
void
fn_set_variable_axis(
    font<_F, _C>&  _fn,
    opentype_tag   _tag,
    float          _value
)
{
    static_assert(has_ff(_F, ff_variable_axes),
                  "requires ff_variable_axes");

    for (auto& a : _fn.variable_axes)
    {
        if (a.tag == _tag)
        {
            a.value = _value;

            return;
        }
    }

    _fn.variable_axes.push_back({_tag, _value});

    return;
}

// fn_get_variable_axis
//   returns the current value for `_tag`, or the supplied `_default`
// when the axis is not set.
template <unsigned _F, typename _C>
float
fn_get_variable_axis(
    const font<_F, _C>&  _fn,
    opentype_tag         _tag,
    float                _default = 0.0f
)
{
    static_assert(has_ff(_F, ff_variable_axes),
                  "requires ff_variable_axes");

    for (const auto& a : _fn.variable_axes)
    {
        if (a.tag == _tag)
        {
            return a.value;
        }
    }

    return _default;
}


// ---------------------------------------------------------------------
// script hint
// ---------------------------------------------------------------------

template <unsigned _F, typename _C>
void
fn_set_script(
    font<_F, _C>&  _fn,
    std::string    _script,
    std::string    _language = std::string()
)
{
    static_assert(has_ff(_F, ff_script_hint),
                  "requires ff_script_hint");

    _fn.script_tag   = std::move(_script);
    _fn.language_tag = std::move(_language);

    return;
}


// ---------------------------------------------------------------------
// backend handles
// ---------------------------------------------------------------------

template <unsigned _F, typename _C>
void
fn_set_file_path(
    font<_F, _C>&  _fn,
    std::string    _path,
    int            _face_index = 0
)
{
    static_assert(has_ff(_F, ff_backend_handles),
                  "requires ff_backend_handles");

    _fn.file_path  = std::move(_path);
    _fn.face_index = _face_index;

    return;
}


// ---------------------------------------------------------------------
// size conversion
// ---------------------------------------------------------------------

// fn_convert_size
//   converts a size value from `_from` units to `_to` units, using
// the supplied DPI for point<->pixel conversions and the supplied
// em-size (in points) for em<->absolute conversions.
//
//   Returns `_value` unchanged if `_from == _to`, or if the conversion
// would require data that isn't available (e.g. em->device_units
// without an em anchor).  Designed to be safe: unsupported conversions
// are no-ops rather than hard errors, so the function can sit on the
// hot path without branching traps.
//
//   Arguments:
//     _value      the numeric size to convert
//     _from       the source unit
//     _to         the target unit
//     _dpi        dots-per-inch of the target surface (default 96)
//     _em_pts    the em size in points (default 10, for percent-of-em
//                 and percent conversions)
constexpr float
fn_convert_size(
    float           _value,
    font_size_unit  _from,
    font_size_unit  _to,
    float           _dpi     = 96.0f,
    float           _em_pts  = 10.0f
) noexcept
{
    if (_from == _to)
    {
        return _value;
    }

    // normalise `_value` to points as an intermediate
    float pts = _value;

    switch (_from)
    {
        case font_size_unit::points:         pts = _value;                        break;
        case font_size_unit::pixels:         pts = _value * 72.0f / _dpi;         break;
        case font_size_unit::em:             pts = _value * _em_pts;              break;
        case font_size_unit::percent:        pts = _value * 0.01f * _em_pts;      break;
        case font_size_unit::device_units:   return _value;  // opaque; no-op
    }

    switch (_to)
    {
        case font_size_unit::points:         return pts;
        case font_size_unit::pixels:         return pts * _dpi / 72.0f;
        case font_size_unit::em:
            return (_em_pts != 0.0f) ? (pts / _em_pts) : _value;
        case font_size_unit::percent:
            return (_em_pts != 0.0f) ? (pts / _em_pts * 100.0f) : _value;
        case font_size_unit::device_units:   return _value;  // opaque; no-op
    }

    return _value;
}



///////////////////////////////////////////////////////////////////////////////
///       10.   FONT TRAITS   (consolidated from font_traits.hpp)           ///
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// I.   EXPRESSION DETECTORS
// ===========================================================================
//   Each `using` alias probes for a specific member.  In SFINAE
// context, a well-formed detector names the member's type; an
// ill-formed one silently falls out.  The detection idiom from
// djinterp::type_traits (is_detected<>) then classifies the result.

// -- core identity --------------------------------------------------

// font_family_t
//   detector: `.family` member (expected: std::string or string-like).
template<typename _Type>
using font_family_t = decltype(std::declval<_Type&>().family);

// font_style_name_t
//   detector: `.style_name` member.
template<typename _Type>
using font_style_name_t = decltype(std::declval<_Type&>().style_name);

// font_size_t
//   detector: `.size` member (expected: float or arithmetic).
template<typename _Type>
using font_size_t = decltype(std::declval<_Type&>().size);

// font_size_unit_t
//   detector: `.size_unit` member.
template<typename _Type>
using font_size_unit_t = decltype(std::declval<_Type&>().size_unit);

// font_weight_t
//   detector: `.weight` member (symbolic weight).
template<typename _Type>
using font_weight_t = decltype(std::declval<_Type&>().weight);

// font_weight_numeric_t
//   detector: `.weight_numeric` member.
template<typename _Type>
using font_weight_numeric_t = decltype(std::declval<_Type&>().weight_numeric);

// font_slant_t
//   detector: `.slant` member.
template<typename _Type>
using font_slant_t = decltype(std::declval<_Type&>().slant);

// font_empty_t
//   detector: `.empty()` method returning bool.
template<typename _Type>
using font_empty_t = decltype(std::declval<const _Type&>().empty());


// -- decorations ----------------------------------------------------

template<typename _Type>
using font_underline_t =
    decltype(std::declval<_Type&>().underline);

template<typename _Type>
using font_strikethrough_t =
    decltype(std::declval<_Type&>().strikethrough);

template<typename _Type>
using font_overline_t =
    decltype(std::declval<_Type&>().overline);


// -- casing ---------------------------------------------------------

template<typename _Type>
using font_small_caps_t =
    decltype(std::declval<_Type&>().small_caps);

template<typename _Type>
using font_all_caps_t =
    decltype(std::declval<_Type&>().all_caps);

template<typename _Type>
using font_subscript_t =
    decltype(std::declval<_Type&>().subscript);

template<typename _Type>
using font_superscript_t =
    decltype(std::declval<_Type&>().superscript);


// -- metrics overrides ----------------------------------------------

template<typename _Type>
using font_letter_spacing_t =
    decltype(std::declval<_Type&>().letter_spacing);

template<typename _Type>
using font_line_height_t =
    decltype(std::declval<_Type&>().line_height);


// -- axes -----------------------------------------------------------

template<typename _Type>
using font_stretch_t =
    decltype(std::declval<_Type&>().stretch);

template<typename _Type>
using font_spacing_t =
    decltype(std::declval<_Type&>().spacing);


// -- color / background --------------------------------------------

template<typename _Type>
using font_foreground_t =
    decltype(std::declval<_Type&>().foreground);

template<typename _Type>
using font_background_t =
    decltype(std::declval<_Type&>().background);

template<typename _Type>
using font_background_enabled_t =
    decltype(std::declval<_Type&>().background_enabled);


// -- opentype features ---------------------------------------------

template<typename _Type>
using font_opentype_features_t =
    decltype(std::declval<_Type&>().opentype_features);


// -- variable-font axes --------------------------------------------

template<typename _Type>
using font_variable_axes_t =
    decltype(std::declval<_Type&>().variable_axes);


// -- script hint ---------------------------------------------------

template<typename _Type>
using font_script_tag_t =
    decltype(std::declval<_Type&>().script_tag);

template<typename _Type>
using font_language_tag_t =
    decltype(std::declval<_Type&>().language_tag);


// -- backend handles -----------------------------------------------

template<typename _Type>
using font_postscript_name_t =
    decltype(std::declval<_Type&>().postscript_name);

template<typename _Type>
using font_full_name_t =
    decltype(std::declval<_Type&>().full_name);

template<typename _Type>
using font_file_path_t =
    decltype(std::declval<_Type&>().file_path);

template<typename _Type>
using font_face_index_t =
    decltype(std::declval<_Type&>().face_index);

template<typename _Type>
using font_native_handle_t =
    decltype(std::declval<_Type&>().native_handle);


// -- color-type alias ----------------------------------------------

template<typename _Type>
using font_color_type_t =
    typename clean_t<_Type>::color_type;


// ===========================================================================
// II.  INDIVIDUAL CAPABILITY TRAITS
// ===========================================================================

// -- core identity --------------------------------------------------

// has_font_family
//   trait: checks for a `.family` member.
template<typename _Type>
struct has_font_family
    : djinterp::is_detected<font_family_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_family_v =
    has_font_family<_Type>::value;

// has_font_style_name
template<typename _Type>
struct has_font_style_name
    : djinterp::is_detected<font_style_name_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_style_name_v =
    has_font_style_name<_Type>::value;

// has_font_size
template<typename _Type>
struct has_font_size
    : djinterp::is_detected<font_size_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_size_v =
    has_font_size<_Type>::value;

// has_font_size_unit
template<typename _Type>
struct has_font_size_unit
    : djinterp::is_detected<font_size_unit_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_size_unit_v =
    has_font_size_unit<_Type>::value;

// has_font_weight
template<typename _Type>
struct has_font_weight
    : djinterp::is_detected<font_weight_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_weight_v =
    has_font_weight<_Type>::value;

// has_font_weight_numeric
template<typename _Type>
struct has_font_weight_numeric
    : djinterp::is_detected<font_weight_numeric_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_weight_numeric_v =
    has_font_weight_numeric<_Type>::value;

// has_font_slant
template<typename _Type>
struct has_font_slant
    : djinterp::is_detected<font_slant_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_slant_v =
    has_font_slant<_Type>::value;


// -- decorations ----------------------------------------------------

// has_font_underline
template<typename _Type>
struct has_font_underline
    : djinterp::is_detected<font_underline_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_underline_v =
    has_font_underline<_Type>::value;

// has_font_strikethrough
template<typename _Type>
struct has_font_strikethrough
    : djinterp::is_detected<font_strikethrough_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_strikethrough_v =
    has_font_strikethrough<_Type>::value;

// has_font_overline
template<typename _Type>
struct has_font_overline
    : djinterp::is_detected<font_overline_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_overline_v =
    has_font_overline<_Type>::value;

// has_font_decorations
//   trait: true when at least one decoration (underline, strikethrough,
// or overline) is exposed.  Useful for adapters that render any
// decoration via a single code path.
template<typename _Type>
struct has_font_decorations : djinterp::disjunction<
    has_font_underline<_Type>,
    has_font_strikethrough<_Type>,
    has_font_overline<_Type>>
{};

template<typename _Type>
constexpr bool has_font_decorations_v =
    has_font_decorations<_Type>::value;


// -- casing ---------------------------------------------------------

// has_font_small_caps
template<typename _Type>
struct has_font_small_caps
    : djinterp::is_detected<font_small_caps_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_small_caps_v =
    has_font_small_caps<_Type>::value;

// has_font_all_caps
template<typename _Type>
struct has_font_all_caps
    : djinterp::is_detected<font_all_caps_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_all_caps_v =
    has_font_all_caps<_Type>::value;

// has_font_subscript
template<typename _Type>
struct has_font_subscript
    : djinterp::is_detected<font_subscript_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_subscript_v =
    has_font_subscript<_Type>::value;

// has_font_superscript
template<typename _Type>
struct has_font_superscript
    : djinterp::is_detected<font_superscript_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_superscript_v =
    has_font_superscript<_Type>::value;

// has_font_casing
//   trait: true if any casing transform is exposed.
template<typename _Type>
struct has_font_casing : djinterp::disjunction<
    has_font_small_caps<_Type>,
    has_font_all_caps<_Type>,
    has_font_subscript<_Type>,
    has_font_superscript<_Type>>
{};

template<typename _Type>
constexpr bool has_font_casing_v =
    has_font_casing<_Type>::value;


// -- metrics overrides ----------------------------------------------

// has_font_letter_spacing
template<typename _Type>
struct has_font_letter_spacing
    : djinterp::is_detected<font_letter_spacing_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_letter_spacing_v =
    has_font_letter_spacing<_Type>::value;

// has_font_line_height
template<typename _Type>
struct has_font_line_height
    : djinterp::is_detected<font_line_height_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_line_height_v =
    has_font_line_height<_Type>::value;

// has_font_metrics_overrides
//   trait: true if either metrics override is exposed.
template<typename _Type>
struct has_font_metrics_overrides : djinterp::disjunction<
    has_font_letter_spacing<_Type>,
    has_font_line_height<_Type>>
{};

template<typename _Type>
constexpr bool has_font_metrics_overrides_v =
    has_font_metrics_overrides<_Type>::value;


// -- axes -----------------------------------------------------------

// has_font_stretch
template<typename _Type>
struct has_font_stretch
    : djinterp::is_detected<font_stretch_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_stretch_v =
    has_font_stretch<_Type>::value;

// has_font_spacing
template<typename _Type>
struct has_font_spacing
    : djinterp::is_detected<font_spacing_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_spacing_v =
    has_font_spacing<_Type>::value;


// -- color / background --------------------------------------------

// has_font_color
//   trait: true if the type exposes a `foreground` member.
template<typename _Type>
struct has_font_color
    : djinterp::is_detected<font_foreground_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_color_v =
    has_font_color<_Type>::value;

// has_font_background
//   trait: true when both `background` and `background_enabled` are
// exposed.  The paired check avoids false positives on types that
// carry only a nominal background field.
template<typename _Type>
struct has_font_background : djinterp::conjunction<
    djinterp::is_detected<font_background_t,         _Type>,
    djinterp::is_detected<font_background_enabled_t, _Type>>
{};

template<typename _Type>
constexpr bool has_font_background_v =
    has_font_background<_Type>::value;


// -- opentype features ---------------------------------------------

// has_font_opentype_features
template<typename _Type>
struct has_font_opentype_features
    : djinterp::is_detected<font_opentype_features_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_opentype_features_v =
    has_font_opentype_features<_Type>::value;


// -- variable-font axes --------------------------------------------

// has_font_variable_axes
template<typename _Type>
struct has_font_variable_axes
    : djinterp::is_detected<font_variable_axes_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_variable_axes_v =
    has_font_variable_axes<_Type>::value;


// -- script hint ---------------------------------------------------

// has_font_script_hint
//   trait: true when both script_tag and language_tag are exposed.
template<typename _Type>
struct has_font_script_hint : djinterp::conjunction<
    djinterp::is_detected<font_script_tag_t,   _Type>,
    djinterp::is_detected<font_language_tag_t, _Type>>
{};

template<typename _Type>
constexpr bool has_font_script_hint_v =
    has_font_script_hint<_Type>::value;


// -- backend handles -----------------------------------------------

// has_font_file_path
template<typename _Type>
struct has_font_file_path
    : djinterp::is_detected<font_file_path_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_file_path_v =
    has_font_file_path<_Type>::value;

// has_font_postscript_name
template<typename _Type>
struct has_font_postscript_name
    : djinterp::is_detected<font_postscript_name_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_postscript_name_v =
    has_font_postscript_name<_Type>::value;

// has_font_native_handle
template<typename _Type>
struct has_font_native_handle
    : djinterp::is_detected<font_native_handle_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_native_handle_v =
    has_font_native_handle<_Type>::value;

// has_font_backend_handles
//   trait: true if ANY backend-resolution handle is exposed (file
// path, PostScript name, or opaque native handle).  Adapters use
// this to decide whether symbolic-only resolution is sufficient.
template<typename _Type>
struct has_font_backend_handles : djinterp::disjunction<
    has_font_file_path<_Type>,
    has_font_postscript_name<_Type>,
    has_font_native_handle<_Type>>
{};

template<typename _Type>
constexpr bool has_font_backend_handles_v =
    has_font_backend_handles<_Type>::value;


// -- color type alias ----------------------------------------------

// has_font_color_type_alias
//   trait: true if the type exposes a `color_type` nested alias.
// Used by templates that need to forward the color type without
// assuming `font_color`.
template<typename _Type>
struct has_font_color_type_alias
    : djinterp::is_detected<font_color_type_t, _Type>
{};

template<typename _Type>
constexpr bool has_font_color_type_alias_v =
    has_font_color_type_alias<_Type>::value;


// ===========================================================================
// III. COMPOSITE TRAITS
// ===========================================================================

// is_font_like
//   trait: compound check for the minimal font interface - family +
// size + weight + slant.  This is the least-common-denominator a
// backend can rely on across Win32 LOGFONT, Cocoa NSFont, fontconfig
// patterns, FreeType face descriptors, and bitmap atlas refs.
//
//   Note: `style_name`, `size_unit`, `weight_numeric`, and `empty()`
// are conveniences rather than requirements - many backend types
// lack them, and adapters provide defaults when absent.
template<typename _Type>
struct is_font_like : djinterp::conjunction<
    has_font_family<_Type>,
    has_font_size<_Type>,
    has_font_weight<_Type>,
    has_font_slant<_Type>>
{};

template<typename _Type>
constexpr bool is_font_like_v =
    is_font_like<_Type>::value;

// is_font_bold_like
//   trait: true if `_Type` is font-like AND exposes a weight axis.
// Equivalent to is_font_like here (weight is in the minimal set),
// provided as an explicit name to mirror is_font_italic_like and
// document intent at the call site.
template<typename _Type>
struct is_font_bold_like : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_weight<_Type>>
{};

template<typename _Type>
constexpr bool is_font_bold_like_v =
    is_font_bold_like<_Type>::value;

// is_font_italic_like
//   trait: true if `_Type` is font-like AND exposes slant.
template<typename _Type>
struct is_font_italic_like : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_slant<_Type>>
{};

template<typename _Type>
constexpr bool is_font_italic_like_v =
    is_font_italic_like<_Type>::value;

// is_font_with_decorations
template<typename _Type>
struct is_font_with_decorations : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_decorations<_Type>>
{};

template<typename _Type>
constexpr bool is_font_with_decorations_v =
    is_font_with_decorations<_Type>::value;

// is_font_with_casing
template<typename _Type>
struct is_font_with_casing : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_casing<_Type>>
{};

template<typename _Type>
constexpr bool is_font_with_casing_v =
    is_font_with_casing<_Type>::value;

// is_font_with_color
template<typename _Type>
struct is_font_with_color : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_color<_Type>>
{};

template<typename _Type>
constexpr bool is_font_with_color_v =
    is_font_with_color<_Type>::value;

// is_font_with_background
template<typename _Type>
struct is_font_with_background : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_background<_Type>>
{};

template<typename _Type>
constexpr bool is_font_with_background_v =
    is_font_with_background<_Type>::value;

// is_font_with_metrics
template<typename _Type>
struct is_font_with_metrics : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_metrics_overrides<_Type>>
{};

template<typename _Type>
constexpr bool is_font_with_metrics_v =
    is_font_with_metrics<_Type>::value;

// is_font_with_opentype
template<typename _Type>
struct is_font_with_opentype : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_opentype_features<_Type>>
{};

template<typename _Type>
constexpr bool is_font_with_opentype_v =
    is_font_with_opentype<_Type>::value;

// is_font_variable
//   trait: true when variable-font axes are exposed.  Used by
// adapters that want to fall back to static-font resolution when
// the input doesn't carry axes.
template<typename _Type>
struct is_font_variable : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_variable_axes<_Type>>
{};

template<typename _Type>
constexpr bool is_font_variable_v =
    is_font_variable<_Type>::value;

// is_font_terminal
//   trait: appropriate for a terminal / TUI backend - family + size +
// weight + slant + (color or background).  Terminals rarely carry
// decorations or metric overrides; this gate reflects that.
template<typename _Type>
struct is_font_terminal : djinterp::conjunction<
    is_font_like<_Type>,
    djinterp::disjunction<
        has_font_color<_Type>,
        has_font_background<_Type>>>
{};

template<typename _Type>
constexpr bool is_font_terminal_v =
    is_font_terminal<_Type>::value;

// is_font_rich
//   trait: the "full" gate - decorations + casing + metrics + axes
// + color + opentype features.  A font that satisfies this is
// suitable input for any modern GUI text renderer.
template<typename _Type>
struct is_font_rich : djinterp::conjunction<
    is_font_like<_Type>,
    has_font_decorations<_Type>,
    has_font_casing<_Type>,
    has_font_metrics_overrides<_Type>,
    has_font_color<_Type>,
    has_font_opentype_features<_Type>>
{};

template<typename _Type>
constexpr bool is_font_rich_v =
    is_font_rich<_Type>::value;


// ===========================================================================
// IV.  SFINAE HELPERS
// ===========================================================================

// enable_if_font_like
//   type: SFINAE helper selecting overloads that accept any font-like type.
template<typename _Type>
using enable_if_font_like =
    typename std::enable_if<is_font_like<_Type>::value>::type;

// enable_if_has_font_underline
template<typename _Type>
using enable_if_has_font_underline =
    typename std::enable_if<has_font_underline<_Type>::value>::type;

// enable_if_has_font_strikethrough
template<typename _Type>
using enable_if_has_font_strikethrough =
    typename std::enable_if<has_font_strikethrough<_Type>::value>::type;

// enable_if_has_font_color
template<typename _Type>
using enable_if_has_font_color =
    typename std::enable_if<has_font_color<_Type>::value>::type;

// enable_if_is_font_variable
template<typename _Type>
using enable_if_is_font_variable =
    typename std::enable_if<is_font_variable<_Type>::value>::type;

// enable_if_is_font_rich
template<typename _Type>
using enable_if_is_font_rich =
    typename std::enable_if<is_font_rich<_Type>::value>::type;

///////////////////////////////////////////////////////////////////////////////
///       11.   FONT CONCEPTS   (consolidated from font_concepts.hpp)       ///
///////////////////////////////////////////////////////////////////////////////

#if ( (defined(__cpp_concepts)) &&                                            \
      (__cpp_concepts >= 201907L) )

// =============================================================================
//  1.  CORE IDENTITY CONCEPTS
// =============================================================================

// font_family_font
//   concept: the type exposes a font family member.
template<typename _Type>
concept font_family_font = has_font_family<_Type>::value;

// font_style_named_font
//   concept: the type exposes a font style-name member.
template<typename _Type>
concept font_style_named_font = has_font_style_name<_Type>::value;

// font_sized_font
//   concept: the type exposes a font size member.
template<typename _Type>
concept font_sized_font = has_font_size<_Type>::value;

// font_size_unit_font
//   concept: the type exposes a font size-unit member.
template<typename _Type>
concept font_size_unit_font = has_font_size_unit<_Type>::value;

// font_weighted_font
//   concept: the type exposes a font weight member.
template<typename _Type>
concept font_weighted_font = has_font_weight<_Type>::value;

// font_numeric_weight_font
//   concept: the type exposes a numeric font weight member.
template<typename _Type>
concept font_numeric_weight_font = has_font_weight_numeric<_Type>::value;

// font_slanted_font
//   concept: the type exposes a slant member.
template<typename _Type>
concept font_slanted_font = has_font_slant<_Type>::value;

// font_like_type
//   concept: the type satisfies the minimal font-like profile.
template<typename _Type>
concept font_like_type = is_font_like<_Type>::value;


// =============================================================================
//  2.  DECORATION AND CASING CONCEPTS
// =============================================================================

// underline_font
//   concept: the type exposes underline support.
template<typename _Type>
concept underline_font = has_font_underline<_Type>::value;

// strikethrough_font
//   concept: the type exposes strikethrough support.
template<typename _Type>
concept strikethrough_font = has_font_strikethrough<_Type>::value;

// overline_font
//   concept: the type exposes overline support.
template<typename _Type>
concept overline_font = has_font_overline<_Type>::value;

// decorated_font
//   concept: the type exposes at least one decoration axis.
template<typename _Type>
concept decorated_font = has_font_decorations<_Type>::value;

// small_caps_font
//   concept: the type exposes small-caps support.
template<typename _Type>
concept small_caps_font = has_font_small_caps<_Type>::value;

// all_caps_font
//   concept: the type exposes all-caps support.
template<typename _Type>
concept all_caps_font = has_font_all_caps<_Type>::value;

// subscript_font
//   concept: the type exposes subscript support.
template<typename _Type>
concept subscript_font = has_font_subscript<_Type>::value;

// superscript_font
//   concept: the type exposes superscript support.
template<typename _Type>
concept superscript_font = has_font_superscript<_Type>::value;

// casing_font
//   concept: the type exposes at least one casing feature.
template<typename _Type>
concept casing_font = has_font_casing<_Type>::value;


// =============================================================================
//  3.  METRICS, AXES, AND COLOR CONCEPTS
// =============================================================================

// letter_spacing_font
//   concept: the type exposes letter-spacing overrides.
template<typename _Type>
concept letter_spacing_font = has_font_letter_spacing<_Type>::value;

// line_height_font
//   concept: the type exposes line-height overrides.
template<typename _Type>
concept line_height_font = has_font_line_height<_Type>::value;

// metrics_override_font
//   concept: the type exposes any metrics override.
template<typename _Type>
concept metrics_override_font = has_font_metrics_overrides<_Type>::value;

// stretch_font
//   concept: the type exposes a stretch axis.
template<typename _Type>
concept stretch_font = has_font_stretch<_Type>::value;

// spacing_axis_font
//   concept: the type exposes a spacing axis.
template<typename _Type>
concept spacing_axis_font = has_font_spacing<_Type>::value;

// foreground_color_font
//   concept: the type exposes foreground color.
template<typename _Type>
concept foreground_color_font = has_font_color<_Type>::value;

// background_color_font
//   concept: the type exposes background color.
template<typename _Type>
concept background_color_font = has_font_background<_Type>::value;

// color_typed_font
//   concept: the type exposes a color_type alias.
template<typename _Type>
concept color_typed_font = has_font_color_type_alias<_Type>::value;


// =============================================================================
//  4.  OPENTYPE, SCRIPT, AND BACKEND CONCEPTS
// =============================================================================

// opentype_feature_font
//   concept: the type exposes OpenType feature support.
template<typename _Type>
concept opentype_feature_font = has_font_opentype_features<_Type>::value;

// variable_axis_font
//   concept: the type exposes variable-font axes.
template<typename _Type>
concept variable_axis_font = has_font_variable_axes<_Type>::value;

// script_hint_font
//   concept: the type exposes script and language hint members.
template<typename _Type>
concept script_hint_font = has_font_script_hint<_Type>::value;

// file_backed_font
//   concept: the type exposes a font file path.
template<typename _Type>
concept file_backed_font = has_font_file_path<_Type>::value;

// postscript_named_font
//   concept: the type exposes a PostScript name.
template<typename _Type>
concept postscript_named_font = has_font_postscript_name<_Type>::value;

// native_handle_font
//   concept: the type exposes a native backend handle.
template<typename _Type>
concept native_handle_font = has_font_native_handle<_Type>::value;

// backend_resolvable_font
//   concept: the type exposes at least one backend-resolution handle.
template<typename _Type>
concept backend_resolvable_font = has_font_backend_handles<_Type>::value;


// =============================================================================
//  5.  COMPOSITE PROFILE CONCEPTS
// =============================================================================

// bold_like_font
//   concept: font-like type with weight support.
template<typename _Type>
concept bold_like_font = is_font_bold_like<_Type>::value;

// italic_like_font
//   concept: font-like type with slant support.
template<typename _Type>
concept italic_like_font = is_font_italic_like<_Type>::value;

// decoration_capable_font
//   concept: font-like type with decoration support.
template<typename _Type>
concept decoration_capable_font = is_font_with_decorations<_Type>::value;

// casing_capable_font
//   concept: font-like type with casing support.
template<typename _Type>
concept casing_capable_font = is_font_with_casing<_Type>::value;

// color_capable_font
//   concept: font-like type with foreground color support.
template<typename _Type>
concept color_capable_font = is_font_with_color<_Type>::value;

// background_capable_font
//   concept: font-like type with background color support.
template<typename _Type>
concept background_capable_font = is_font_with_background<_Type>::value;

// metrics_capable_font
//   concept: font-like type with metric override support.
template<typename _Type>
concept metrics_capable_font = is_font_with_metrics<_Type>::value;

// opentype_capable_font
//   concept: font-like type with OpenType feature support.
template<typename _Type>
concept opentype_capable_font = is_font_with_opentype<_Type>::value;

// variable_font_type
//   concept: font-like type with variable-font axes.
template<typename _Type>
concept variable_font_type = is_font_variable<_Type>::value;

// terminal_font_type
//   concept: font-like type suitable for terminal or TUI rendering.
template<typename _Type>
concept terminal_font_type = is_font_terminal<_Type>::value;

// rich_font_type
//   concept: font-like type satisfying the rich modern renderer profile.
template<typename _Type>
concept rich_font_type = is_font_rich<_Type>::value;

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_TEXT_FONT_