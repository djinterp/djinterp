/*******************************************************************************
* djinterp [text]                                             font_concepts.hpp
*
* Font concepts:
*   C++20 concepts layered over font_traits.hpp. These concepts provide
* readable constraints for font-like types without replacing the existing
* SFINAE trait surface.
*
*   The concepts mirror the public trait surface from font_traits.hpp:
*     1.  Core identity
*     2.  Decorations and casing
*     3.  Metrics, axes, and color
*     4.  OpenType, script, and backend handles
*     5.  Composite font profiles
*
*
* path:      /inc/djinterp/core/text/font_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                        created: 2026.05.15
*******************************************************************************/

#ifndef DJINTERP_TEXT_FONT_CONCEPTS_
#define DJINTERP_TEXT_FONT_CONCEPTS_ 1

#ifndef __cplusplus
    #error "font_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "font_traits.hpp"


NS_DJINTERP

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===============================================================================
//  1.  CORE IDENTITY CONCEPTS
// ===============================================================================

// font_family_font
//   concept: the type exposes a font family member.
template<typename _Type>
concept font_family_font =
    has_font_family<_Type>::value;

// font_style_named_font
//   concept: the type exposes a font style-name member.
template<typename _Type>
concept font_style_named_font =
    has_font_style_name<_Type>::value;

// font_sized_font
//   concept: the type exposes a font size member.
template<typename _Type>
concept font_sized_font =
    has_font_size<_Type>::value;

// font_size_unit_font
//   concept: the type exposes a font size-unit member.
template<typename _Type>
concept font_size_unit_font =
    has_font_size_unit<_Type>::value;

// font_weighted_font
//   concept: the type exposes a font weight member.
template<typename _Type>
concept font_weighted_font =
    has_font_weight<_Type>::value;

// font_numeric_weight_font
//   concept: the type exposes a numeric font weight member.
template<typename _Type>
concept font_numeric_weight_font =
    has_font_weight_numeric<_Type>::value;

// font_slanted_font
//   concept: the type exposes a slant member.
template<typename _Type>
concept font_slanted_font =
    has_font_slant<_Type>::value;

// font_like_type
//   concept: the type satisfies the minimal font-like profile.
template<typename _Type>
concept font_like_type =
    is_font_like<_Type>::value;


// ===============================================================================
//  2.  DECORATION AND CASING CONCEPTS
// ===============================================================================

// underline_font
//   concept: the type exposes underline support.
template<typename _Type>
concept underline_font =
    has_font_underline<_Type>::value;

// strikethrough_font
//   concept: the type exposes strikethrough support.
template<typename _Type>
concept strikethrough_font =
    has_font_strikethrough<_Type>::value;

// overline_font
//   concept: the type exposes overline support.
template<typename _Type>
concept overline_font =
    has_font_overline<_Type>::value;

// decorated_font
//   concept: the type exposes at least one decoration axis.
template<typename _Type>
concept decorated_font =
    has_font_decorations<_Type>::value;

// small_caps_font
//   concept: the type exposes small-caps support.
template<typename _Type>
concept small_caps_font =
    has_font_small_caps<_Type>::value;

// all_caps_font
//   concept: the type exposes all-caps support.
template<typename _Type>
concept all_caps_font =
    has_font_all_caps<_Type>::value;

// subscript_font
//   concept: the type exposes subscript support.
template<typename _Type>
concept subscript_font =
    has_font_subscript<_Type>::value;

// superscript_font
//   concept: the type exposes superscript support.
template<typename _Type>
concept superscript_font =
    has_font_superscript<_Type>::value;

// casing_font
//   concept: the type exposes at least one casing feature.
template<typename _Type>
concept casing_font =
    has_font_casing<_Type>::value;


// ===============================================================================
//  3.  METRICS, AXES, AND COLOR CONCEPTS
// ===============================================================================

// letter_spacing_font
//   concept: the type exposes letter-spacing overrides.
template<typename _Type>
concept letter_spacing_font =
    has_font_letter_spacing<_Type>::value;

// line_height_font
//   concept: the type exposes line-height overrides.
template<typename _Type>
concept line_height_font =
    has_font_line_height<_Type>::value;

// metrics_override_font
//   concept: the type exposes any metrics override.
template<typename _Type>
concept metrics_override_font =
    has_font_metrics_overrides<_Type>::value;

// stretch_font
//   concept: the type exposes a stretch axis.
template<typename _Type>
concept stretch_font =
    has_font_stretch<_Type>::value;

// spacing_axis_font
//   concept: the type exposes a spacing axis.
template<typename _Type>
concept spacing_axis_font =
    has_font_spacing<_Type>::value;

// foreground_color_font
//   concept: the type exposes foreground color.
template<typename _Type>
concept foreground_color_font =
    has_font_color<_Type>::value;

// background_color_font
//   concept: the type exposes background color.
template<typename _Type>
concept background_color_font =
    has_font_background<_Type>::value;

// color_typed_font
//   concept: the type exposes a color_type alias.
template<typename _Type>
concept color_typed_font =
    has_font_color_type_alias<_Type>::value;


// ===============================================================================
//  4.  OPENTYPE, SCRIPT, AND BACKEND CONCEPTS
// ===============================================================================

// opentype_feature_font
//   concept: the type exposes OpenType feature support.
template<typename _Type>
concept opentype_feature_font =
    has_font_opentype_features<_Type>::value;

// variable_axis_font
//   concept: the type exposes variable-font axes.
template<typename _Type>
concept variable_axis_font =
    has_font_variable_axes<_Type>::value;

// script_hint_font
//   concept: the type exposes script and language hint members.
template<typename _Type>
concept script_hint_font =
    has_font_script_hint<_Type>::value;

// file_backed_font
//   concept: the type exposes a font file path.
template<typename _Type>
concept file_backed_font =
    has_font_file_path<_Type>::value;

// postscript_named_font
//   concept: the type exposes a PostScript name.
template<typename _Type>
concept postscript_named_font =
    has_font_postscript_name<_Type>::value;

// native_handle_font
//   concept: the type exposes a native backend handle.
template<typename _Type>
concept native_handle_font =
    has_font_native_handle<_Type>::value;

// backend_resolvable_font
//   concept: the type exposes at least one backend-resolution handle.
template<typename _Type>
concept backend_resolvable_font =
    has_font_backend_handles<_Type>::value;


// ===============================================================================
//  5.  COMPOSITE PROFILE CONCEPTS
// ===============================================================================

// bold_like_font
//   concept: font-like type with weight support.
template<typename _Type>
concept bold_like_font =
    is_font_bold_like<_Type>::value;

// italic_like_font
//   concept: font-like type with slant support.
template<typename _Type>
concept italic_like_font =
    is_font_italic_like<_Type>::value;

// decoration_capable_font
//   concept: font-like type with decoration support.
template<typename _Type>
concept decoration_capable_font =
    is_font_with_decorations<_Type>::value;

// casing_capable_font
//   concept: font-like type with casing support.
template<typename _Type>
concept casing_capable_font =
    is_font_with_casing<_Type>::value;

// color_capable_font
//   concept: font-like type with foreground color support.
template<typename _Type>
concept color_capable_font =
    is_font_with_color<_Type>::value;

// background_capable_font
//   concept: font-like type with background color support.
template<typename _Type>
concept background_capable_font =
    is_font_with_background<_Type>::value;

// metrics_capable_font
//   concept: font-like type with metric override support.
template<typename _Type>
concept metrics_capable_font =
    is_font_with_metrics<_Type>::value;

// opentype_capable_font
//   concept: font-like type with OpenType feature support.
template<typename _Type>
concept opentype_capable_font =
    is_font_with_opentype<_Type>::value;

// variable_font_type
//   concept: font-like type with variable-font axes.
template<typename _Type>
concept variable_font_type =
    is_font_variable<_Type>::value;

// terminal_font_type
//   concept: font-like type suitable for terminal or TUI rendering.
template<typename _Type>
concept terminal_font_type =
    is_font_terminal<_Type>::value;

// rich_font_type
//   concept: font-like type satisfying the rich modern renderer profile.
template<typename _Type>
concept rich_font_type =
    is_font_rich<_Type>::value;

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_TEXT_FONT_CONCEPTS_
