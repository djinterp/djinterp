/******************************************************************************
* djinterp [text]                               text_function_concepts.hpp
*
* Text function concepts:
*   C++20 concepts layered over text_function_traits.hpp. This header
* complements the small built-in concept block in text_function_traits.hpp
* by adding more granular protocol and aggregate-profile concepts.
*
*   Existing concepts in text_function_traits.hpp are intentionally not
* redefined here:
*     text_function_type, text_template_type, text_specifier_type,
*     text_destination_type, composable_text_functions.
*
*
* path:      /inc/djinterp/core/text/text_function_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.15
******************************************************************************/

#ifndef DJINTERP_TEXT_FUNCTION_CONCEPTS_
#define DJINTERP_TEXT_FUNCTION_CONCEPTS_ 1

#ifndef __cplusplus
    #error "text_function_concepts.hpp requires C++ compilation"
#endif

#include "text_function_traits.hpp"


NS_DJINTERP

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   TEXT FUNCTION PROTOCOL CONCEPTS
// ===========================================================================

// string_callable_text_function
//   concept: the callable accepts _Input and returns string-convertible output.
template<typename _Fn,
         typename _Input = const std::string&>
concept string_callable_text_function =
    has_string_call_with<_Fn, _Input>::value;

// nullary_text_function
//   concept: the callable accepts no arguments and returns string-convertible output.
template<typename _Fn>
concept nullary_text_function =
    is_nullary_text_function<_Fn>::value;

// chainable_text_function
//   concept: the callable accepts std::string input and returns string output.
template<typename _Fn>
concept chainable_text_function =
    is_chainable_text_function<_Fn>::value;


// ===========================================================================
// II.  TEXT SPECIFIER CONCEPTS
// ===========================================================================

// key_field_text_specifier
//   concept: the type exposes a key field.
template<typename _Type>
concept key_field_text_specifier =
    has_key_field<_Type>::value;

// key_method_text_specifier
//   concept: the type exposes key().
template<typename _Type>
concept key_method_text_specifier =
    has_key_method<_Type>::value;

// key_access_text_specifier
//   concept: the type exposes either key field or key().
template<typename _Type>
concept key_access_text_specifier =
    has_key_access<_Type>::value;

// value_field_text_specifier
//   concept: the type exposes a value field.
template<typename _Type>
concept value_field_text_specifier =
    has_value_field<_Type>::value;

// value_method_text_specifier
//   concept: the type exposes value().
template<typename _Type>
concept value_method_text_specifier =
    has_value_method<_Type>::value;

// value_access_text_specifier
//   concept: the type exposes either value field or value().
template<typename _Type>
concept value_access_text_specifier =
    has_value_access<_Type>::value;

// resolvable_text_specifier
//   concept: the type exposes resolve().
template<typename _Type>
concept resolvable_text_specifier =
    has_resolve_method<_Type>::value;

// simple_text_specifier
//   concept: a structural text specifier with key and value/resolve access.
template<typename _Type>
concept simple_text_specifier =
    is_text_specifier<_Type>::value;


// ===========================================================================
// III. TEXT TEMPLATE SURFACE CONCEPTS
// ===========================================================================

// bindable_text_template
//   concept: the type exposes bind(key, value).
template<typename _Type>
concept bindable_text_template =
    has_bind_method<_Type>::value;

// renderable_text_template
//   concept: the type exposes render(format_string).
template<typename _Type>
concept renderable_text_template =
    has_render_method<_Type>::value;

// prefix_marker_text_template
//   concept: the type exposes prefix().
template<typename _Type>
concept prefix_marker_text_template =
    has_prefix_method<_Type>::value;

// suffix_marker_text_template
//   concept: the type exposes suffix().
template<typename _Type>
concept suffix_marker_text_template =
    has_suffix_method<_Type>::value;

// marker_configured_text_template
//   concept: the type exposes both prefix and suffix marker configuration.
template<typename _Type>
concept marker_configured_text_template =
    has_marker_config<_Type>::value;

// unbindable_text_template
//   concept: the type exposes unbind(key).
template<typename _Type>
concept unbindable_text_template =
    has_unbind_method<_Type>::value;

// clear_bindings_text_template
//   concept: the type exposes clear_bindings().
template<typename _Type>
concept clear_bindings_text_template =
    has_clear_bindings_method<_Type>::value;

// clearable_text_template
//   concept: the type exposes clear().
template<typename _Type>
concept clearable_text_template =
    has_clear_method<_Type>::value;

// function_binding_text_template
//   concept: the type exposes bind_function(...).
template<typename _Type>
concept function_binding_text_template =
    has_bind_function_method<_Type>::value;

// template_binding_text_template
//   concept: the type exposes bind_template(...).
template<typename _Type>
concept template_binding_text_template =
    has_bind_template_method<_Type>::value;

// full_template_surface
//   concept: the type satisfies the full base text-template profile.
template<typename _Type>
concept full_template_surface =
    is_full_text_template<_Type>::value;


// ===========================================================================
// IV.  TEXT DESTINATION CONCEPTS
// ===========================================================================

// append_text_destination
//   concept: the destination exposes append(const char*, size_t).
template<typename _Type>
concept append_text_destination =
    has_append_method<_Type>::value;

// callable_text_sink
//   concept: the destination is callable with (const char*, size_t).
template<typename _Type>
concept callable_text_sink =
    is_text_sink_callable<_Type>::value;

// string_text_destination
//   concept: the destination is std::string.
template<typename _Type>
concept string_text_destination =
    std::is_same<typename std::remove_cv<
        typename std::remove_reference<_Type>::type>::type,
        std::string>::value;


// ===========================================================================
// V.   AGGREGATE CLASSIFICATION CONCEPTS
// ===========================================================================

// classified_text_function
//   concept: shorthand for text_function_class<T>::is_text_fn.
template<typename _Type>
concept classified_text_function =
    text_function_class<_Type>::is_text_fn;

// classified_nullary_text_function
//   concept: shorthand for text_function_class<T>::is_nullary.
template<typename _Type>
concept classified_nullary_text_function =
    text_function_class<_Type>::is_nullary;

// classified_text_template
//   concept: shorthand for text_function_class<T>::is_template.
template<typename _Type>
concept classified_text_template =
    text_function_class<_Type>::is_template;

// classified_full_text_template
//   concept: shorthand for text_function_class<T>::is_full_template.
template<typename _Type>
concept classified_full_text_template =
    text_function_class<_Type>::is_full_template;

// classified_text_specifier
//   concept: shorthand for text_function_class<T>::is_specifier.
template<typename _Type>
concept classified_text_specifier =
    text_function_class<_Type>::is_specifier;

// classified_text_destination
//   concept: shorthand for text_function_class<T>::is_destination.
template<typename _Type>
concept classified_text_destination =
    text_function_class<_Type>::is_destination;

// mutable_text_template
//   concept: a text template with bind and at least one removal/clear path.
template<typename _Type>
concept mutable_text_template =
    bindable_text_template<_Type> &&
    ( unbindable_text_template<_Type>       ||
      clear_bindings_text_template<_Type>   ||
      clearable_text_template<_Type> );

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_TEXT_FUNCTION_CONCEPTS_
