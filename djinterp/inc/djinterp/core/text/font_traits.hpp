/*******************************************************************************
* uxoxo [font]                                                   font_traits.hpp
*
* Font trait module:
*   Compile-time structural detection for font-like types.  Every
* trait is SFINAE-based — no tags, no registration, no base class
* required.  A type that exposes the right members is classified
* automatically, whether it's the provided `font<_Feat, _C>`
* template, a backend-specific struct (LOGFONT, NSFont descriptor,
* FcPattern wrapper, FT_Face + index), or a project-specific
* game-engine font_ref.
*
*   NAMING CONVENTION (matches the rest of the djinterp / uxoxo
* trait layers):
*     Expression detectors:   font_<member>_t
*     Struct-based traits:    has_font_<capability>
*     Variable template _v:   has_font_<capability>_v
*     Composite traits:       is_font_like, is_font_with_<group>,
*                              is_font_bold_like, is_font_italic_like
*
*   This header provides ONLY tagged (struct-based) traits plus their
* `_v` variable-template shortcuts.  Tagless constexpr-bool traits
* are not provided because every detection here is cleanly expressible
* tagged — adding a tagless parallel track would duplicate surface
* area with no new capability.  If a downstream module needs tagless
* forms (e.g. to keep parameter lists uniform across trait families),
* they can be added in a follow-up without reshaping this one.
*
* Contents:
*   1.  Expression detectors
*   2.  Individual capability traits (has_font_*)
*   3.  Composite traits (is_font_like, is_font_*)
*   4.  SFINAE helpers (enable_if_font_*)
*
*
* path:      /inc/uxoxo/templates/font/font_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.04.18
*******************************************************************************/

#ifndef UXOXO_FONT_TRAITS_
#define UXOXO_FONT_TRAITS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
// djinterp
#include <djinterp/core/djinterp.hpp>
#include <djinterp/core/type_traits.hpp>
// uxoxo
#include "../../uxoxo.hpp"


NS_UXOXO

namespace font {
namespace traits {


// =============================================================================
// I.   EXPRESSION DETECTORS
// =============================================================================
//   Each `using` alias probes for a specific member.  In SFINAE
// context, a well-formed detector names the member's type; an
// ill-formed one silently falls out.  The detection idiom from
// djinterp::type_traits (is_detected<>) then classifies the result.

// -- core identity --------------------------------------------------

// font_family_t
//   detector: `.family` member (expected: std::string or string-like).
template<typename _Type>
using font_family_t =
    decltype(std::declval<_Type&>().family);

// font_style_name_t
//   detector: `.style_name` member.
template<typename _Type>
using font_style_name_t =
    decltype(std::declval<_Type&>().style_name);

// font_size_t
//   detector: `.size` member (expected: float or arithmetic).
template<typename _Type>
using font_size_t =
    decltype(std::declval<_Type&>().size);

// font_size_unit_t
//   detector: `.size_unit` member.
template<typename _Type>
using font_size_unit_t =
    decltype(std::declval<_Type&>().size_unit);

// font_weight_t
//   detector: `.weight` member (symbolic weight).
template<typename _Type>
using font_weight_t =
    decltype(std::declval<_Type&>().weight);

// font_weight_numeric_t
//   detector: `.weight_numeric` member.
template<typename _Type>
using font_weight_numeric_t =
    decltype(std::declval<_Type&>().weight_numeric);

// font_slant_t
//   detector: `.slant` member.
template<typename _Type>
using font_slant_t =
    decltype(std::declval<_Type&>().slant);

// font_empty_t
//   detector: `.empty()` method returning bool.
template<typename _Type>
using font_empty_t =
    decltype(std::declval<const _Type&>().empty());


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
    typename _Type::color_type;


// =============================================================================
// II.  INDIVIDUAL CAPABILITY TRAITS
// =============================================================================

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


// =============================================================================
// III. COMPOSITE TRAITS
// =============================================================================

// is_font_like
//   trait: compound check for the minimal font interface — family +
// size + weight + slant.  This is the least-common-denominator a
// backend can rely on across Win32 LOGFONT, Cocoa NSFont, fontconfig
// patterns, FreeType face descriptors, and bitmap atlas refs.
//
//   Note: `style_name`, `size_unit`, `weight_numeric`, and `empty()`
// are conveniences rather than requirements — many backend types
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
//   trait: appropriate for a terminal / TUI backend — family + size +
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
//   trait: the "full" gate — decorations + casing + metrics + axes
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


// =============================================================================
// IV.  SFINAE HELPERS
// =============================================================================

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


}   // namespace traits
}   // namespace font

NS_END  // uxoxo


#endif  // UXOXO_FONT_TRAITS_
