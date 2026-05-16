/******************************************************************************
* djinterp [text]                                   text_template_concepts.hpp
*
* Text template concepts:
*   C++20 concepts layered over text_template_traits.hpp. This header
* complements the small built-in concept block in text_template_traits.hpp
* by adding granular advanced-binding, section, rendering, configuration,
* and binding-management concepts.
*
*   Existing concepts in text_template_traits.hpp are intentionally not
* redefined here:
*     full_text_template_type, section_aware_template_type,
*     configurable_template_type.
*
* 
* path:      /inc/djinterp/core/text/text_template_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.15
******************************************************************************/

#ifndef DJINTERP_TEXT_TEMPLATE_CONCEPTS_
#define DJINTERP_TEXT_TEMPLATE_CONCEPTS_ 1

#ifndef __cplusplus
    #error "text_template_concepts.hpp requires C++ compilation"
#endif

#include "text_template_traits.hpp"


NS_DJINTERP

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   ADVANCED BINDING CONCEPTS
// ===========================================================================

// conditional_binding_template
//   concept: the type exposes bind_conditional(...).
template<typename _Type>
concept conditional_binding_template =
    has_bind_conditional_method<_Type>::value;

// section_binding_template
//   concept: the type exposes bind_section(...).
template<typename _Type>
concept section_binding_template =
    has_bind_section_method<_Type>::value;

// list_binding_template
//   concept: the type exposes bind_list(...).
template<typename _Type>
concept list_binding_template =
    has_bind_list_method<_Type>::value;

// transform_binding_template
//   concept: the type exposes bind_transform(...).
template<typename _Type>
concept transform_binding_template =
    has_bind_transform_method<_Type>::value;

// advanced_binding_template
//   concept: the type supports at least one advanced binding mode.
template<typename _Type>
concept advanced_binding_template =
    ( has_bind_conditional_method<_Type>::value ||
      has_bind_section_method<_Type>::value     ||
      has_bind_list_method<_Type>::value        ||
      has_bind_transform_method<_Type>::value );


// ===========================================================================
// II.  SECTION BLOCK CONCEPTS
// ===========================================================================

// section_open_marker_template
//   concept: the type exposes section_open_marker() or equivalent.
template<typename _Type>
concept section_open_marker_template =
    has_section_open_marker<_Type>::value;

// section_close_marker_template
//   concept: the type exposes section_close_marker().
template<typename _Type>
concept section_close_marker_template =
    has_section_close_marker<_Type>::value;

// inverted_section_marker_template
//   concept: the type exposes section inversion marker support.
template<typename _Type>
concept inverted_section_marker_template =
    has_section_invert_marker<_Type>::value;

// section_block_template
//   concept: the type exposes the full section block marker protocol.
template<typename _Type>
concept section_block_template =
    has_section_support<_Type>::value;


// ===========================================================================
// III. RENDERING VARIANT CONCEPTS
// ===========================================================================

// render_to_template
//   concept: the type exposes render_to(...).
template<typename _Type>
concept render_to_template =
    has_render_to_method<_Type>::value;

// render_with_template
//   concept: the type exposes render_with(...).
template<typename _Type>
concept render_with_template =
    has_render_with_method<_Type>::value;

// extended_rendering_template
//   concept: the type exposes at least one extended rendering variant.
template<typename _Type>
concept extended_rendering_template =
    ( has_render_to_method<_Type>::value ||
      has_render_with_method<_Type>::value );


// ===========================================================================
// IV.  ESCAPE AND DEPTH CONFIGURATION CONCEPTS
// ===========================================================================

// escape_query_template
//   concept: the type exposes escape_char().
template<typename _Type>
concept escape_query_template =
    has_escape_char_method<_Type>::value;

// escape_mutable_template
//   concept: the type exposes set_escape_char(char).
template<typename _Type>
concept escape_mutable_template =
    has_set_escape_char_method<_Type>::value;

// escape_configurable_template
//   concept: the type exposes query and mutation for escape configuration.
template<typename _Type>
concept escape_configurable_template =
    has_escape_char_method<_Type>::value &&
    has_set_escape_char_method<_Type>::value;

// max_depth_query_template
//   concept: the type exposes max_depth().
template<typename _Type>
concept max_depth_query_template =
    has_max_depth_method<_Type>::value;

// max_depth_mutable_template
//   concept: the type exposes set_max_depth(size_t).
template<typename _Type>
concept max_depth_mutable_template =
    has_set_max_depth_method<_Type>::value;

// depth_configurable_template
//   concept: the type exposes query and mutation for max-depth configuration.
template<typename _Type>
concept depth_configurable_template =
    has_max_depth_method<_Type>::value &&
    has_set_max_depth_method<_Type>::value;


// ===========================================================================
// V.   BINDING MANAGEMENT CONCEPTS
// ===========================================================================

// binding_query_template
//   concept: the type exposes has_binding(key).
template<typename _Type>
concept binding_query_template =
    has_has_binding_method<_Type>::value;

// binding_count_template
//   concept: the type exposes binding_count().
template<typename _Type>
concept binding_count_template =
    has_binding_count_method<_Type>::value;

// binding_keys_template
//   concept: the type exposes binding_keys().
template<typename _Type>
concept binding_keys_template =
    has_binding_keys_method<_Type>::value;

// binding_managed_template
//   concept: the type exposes at least one binding inspection path.
template<typename _Type>
concept binding_managed_template =
    ( has_has_binding_method<_Type>::value    ||
      has_binding_count_method<_Type>::value  ||
      has_binding_keys_method<_Type>::value );


// ===========================================================================
// VI.  CLASSIFICATION-BASED SHORTHAND CONCEPTS
// ===========================================================================

// classified_template_type
//   concept: shorthand for text_template_class<T>::is_template.
template<typename _Type>
concept classified_template_type =
    text_template_class<_Type>::is_template;

// classified_full_template_type
//   concept: shorthand for text_template_class<T>::is_full_template.
template<typename _Type>
concept classified_full_template_type =
    text_template_class<_Type>::is_full_template;

// classified_section_template
//   concept: shorthand for text_template_class<T>::has_sections.
template<typename _Type>
concept classified_section_template =
    text_template_class<_Type>::has_sections;

// classified_advanced_binding_template
//   concept: shorthand for any advanced binding capability in text_template_class.
template<typename _Type>
concept classified_advanced_binding_template =
    ( text_template_class<_Type>::has_fn_bind          ||
      text_template_class<_Type>::has_tmpl_bind        ||
      text_template_class<_Type>::has_list_bind        ||
      text_template_class<_Type>::has_conditional_bind ||
      text_template_class<_Type>::has_section_bind     ||
      text_template_class<_Type>::has_transform_bind );

// classified_extended_render_template
//   concept: shorthand for extended rendering variants in text_template_class.
template<typename _Type>
concept classified_extended_render_template =
    ( text_template_class<_Type>::has_render_to ||
      text_template_class<_Type>::has_render_with );

// classified_binding_managed_template
//   concept: shorthand for binding management in text_template_class.
template<typename _Type>
concept classified_binding_managed_template =
    ( text_template_class<_Type>::has_has_binding ||
      text_template_class<_Type>::has_count       ||
      text_template_class<_Type>::has_keys );

// full_configurable_template
//   concept: template with marker, escape, and depth configuration.
template<typename _Type>
concept full_configurable_template =
    text_template_class<_Type>::has_markers      &&
    text_template_class<_Type>::has_escape       &&
    text_template_class<_Type>::has_depth_config;

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_TEXT_TEMPLATE_CONCEPTS_
