/******************************************************************************
* djinterp [text]                                     text_function_traits.hpp
*
*   Structural SFINAE detection traits for text function and text template
* types.  All detection is purely structural — no tag types, no base-class
* checks, no registration.  Expose the right members and the trait system
* classifies you automatically.
*
*   DETECTED PROTOCOLS:
*
*   text_function protocol:
*     A callable F that accepts input I and returns a string-like output O.
*     Optionally accepts additional arguments.  Detected by checking that
*     F(I) is well-formed and that the result is string-like.
*
*   text_specifier protocol:
*     A key/value mapping entry.  Must expose `key` (string-like) and
*     either `value` (string-like) or `resolve()` (callable returning
*     string).  Detected structurally.
*
*   text_template protocol:
*     A type exposing bind(key, value), render(format_string), and
*     optionally prefix()/suffix() for marker configuration.
*
*   text_destination protocol:
*     Any type that can receive text output.  Delegates to the printer
*     traits (is_output_target) when available, but provides its own
*     independent detection for standalone use.
*
*   COMPAT:
*   C++11: all traits via struct::value
*   C++14: _v variable templates
*   C++17: if constexpr in consumer code
*   C++20: concept wrappers behind feature gate
*
*
* TABLE OF CONTENTS
* =================
* I.    TEXT FUNCTION DETECTION
* II.   TEXT SPECIFIER DETECTION
* III.  TEXT TEMPLATE DETECTION
* IV.   TEXT DESTINATION DETECTION
* V.    CHAINABILITY DETECTION
* VI.   COMBINED CLASSIFICATION
* VII.  VARIABLE TEMPLATES
* VIII. C++20 CONCEPT WRAPPERS
*
*
* path:      /inc/text/meta/text_function_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEXT_FUNCTION_TRAITS_
#define DJINTERP_TEXT_FUNCTION_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"


NS_DJINTERP
NS_TEXT
NS_TRAITS


///////////////////////////////////////////////////////////////////////////////
///                I.   TEXT FUNCTION DETECTION                              ///
///////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------
//  has_string_call_with
// ---------------------------------------------------------------------

// has_string_call_with
//   trait: true if _Fn(_Input) is well-formed and returns a
// type convertible to std::string.
template<typename _Fn,
         typename _Input,
         typename = void>
struct has_string_call_with : std::false_type
{};

template<typename _Fn,
         typename _Input>
struct has_string_call_with<_Fn, _Input, D_VOID_T<
    decltype(std::declval<const _Fn&>()(
        std::declval<_Input>()))
>> : std::is_convertible<
    decltype(std::declval<const _Fn&>()(
        std::declval<_Input>())),
    std::string
>
{};


// ---------------------------------------------------------------------
//  has_void_string_call
// ---------------------------------------------------------------------

// has_void_string_call
//   trait: true if _Fn() (nullary) is well-formed and returns
// a type convertible to std::string.
template<typename _Fn,
         typename = void>
struct has_void_string_call : std::false_type
{};

template<typename _Fn>
struct has_void_string_call<_Fn, D_VOID_T<
    decltype(std::declval<const _Fn&>()())
>> : std::is_convertible<
    decltype(std::declval<const _Fn&>()()),
    std::string
>
{};


// ---------------------------------------------------------------------
//  is_text_function
// ---------------------------------------------------------------------

// is_text_function
//   trait: true if _Fn is a callable that accepts _Input and
// returns string-like output.  This is the primary text
// function protocol detector.
template<typename _Fn,
         typename _Input = const std::string&>
struct is_text_function
{
    static constexpr bool value =
        has_string_call_with<_Fn, _Input>::value;
};


// is_nullary_text_function
//   trait: true if _Fn is a callable that takes no arguments
// and returns string-like output.  Used for computed
// specifier values.
template<typename _Fn>
struct is_nullary_text_function
{
    static constexpr bool value =
        has_void_string_call<_Fn>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEXT SPECIFIER DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------
//  has_key_field
// ---------------------------------------------------------------------

// has_key_field
//   trait: true if _T exposes a `key` member (field or
// method) returning something string-like.
template<typename _T,
         typename = void>
struct has_key_field : std::false_type
{};

template<typename _T>
struct has_key_field<_T, D_VOID_T<
    decltype(std::declval<const _T&>().key)
>> : std::true_type
{};

template<typename _T,
         typename = void>
struct has_key_method : std::false_type
{};

template<typename _T>
struct has_key_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().key())
>> : std::true_type
{};

// has_key_access
//   trait: true if _T exposes key via field or method.
template<typename _T>
struct has_key_access
{
    static constexpr bool value =
        ( has_key_field<_T>::value ||
          has_key_method<_T>::value );
};


// ---------------------------------------------------------------------
//  has_value_field
// ---------------------------------------------------------------------

// has_value_field
//   trait: true if _T exposes a `value` member field.
template<typename _T,
         typename = void>
struct has_value_field : std::false_type
{};

template<typename _T>
struct has_value_field<_T, D_VOID_T<
    decltype(std::declval<const _T&>().value)
>> : std::true_type
{};

// has_value_method
//   trait: true if _T exposes a value() method.
template<typename _T,
         typename = void>
struct has_value_method : std::false_type
{};

template<typename _T>
struct has_value_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().value())
>> : std::true_type
{};

// has_value_access
//   trait: true if _T exposes value via field or method.
template<typename _T>
struct has_value_access
{
    static constexpr bool value =
        ( has_value_field<_T>::value ||
          has_value_method<_T>::value );
};


// ---------------------------------------------------------------------
//  has_resolve_method
// ---------------------------------------------------------------------

// has_resolve_method
//   trait: true if _T exposes a resolve() method returning
// a string-like type.  Used for computed/dynamic specifier
// bindings.
template<typename _T,
         typename = void>
struct has_resolve_method : std::false_type
{};

template<typename _T>
struct has_resolve_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().resolve())
>> : std::is_convertible<
    decltype(std::declval<const _T&>().resolve()),
    std::string
>
{};


// ---------------------------------------------------------------------
//  is_text_specifier
// ---------------------------------------------------------------------

// is_text_specifier
//   trait: true if _T has key access and either value access
// or a resolve method.
template<typename _T>
struct is_text_specifier
{
    static constexpr bool value =
        ( has_key_access<_T>::value &&
          ( has_value_access<_T>::value ||
            has_resolve_method<_T>::value ) );
};


///////////////////////////////////////////////////////////////////////////////
///                III. TEXT TEMPLATE DETECTION                              ///
///////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------
//  has_bind_method
// ---------------------------------------------------------------------

// has_bind_method
//   trait: true if _T exposes bind(string, string).
template<typename _T,
         typename = void>
struct has_bind_method : std::false_type
{};

template<typename _T>
struct has_bind_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};


// ---------------------------------------------------------------------
//  has_render_method
// ---------------------------------------------------------------------

// has_render_method
//   trait: true if _T exposes render(string) returning
// string.
template<typename _T,
         typename = void>
struct has_render_method : std::false_type
{};

template<typename _T>
struct has_render_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().render(
        std::declval<const std::string&>()))
>> : std::is_convertible<
    decltype(std::declval<const _T&>().render(
        std::declval<const std::string&>())),
    std::string
>
{};


// ---------------------------------------------------------------------
//  has_prefix_method / has_suffix_method
// ---------------------------------------------------------------------

// has_prefix_method
//   trait: true if _T exposes prefix().
template<typename _T,
         typename = void>
struct has_prefix_method : std::false_type
{};

template<typename _T>
struct has_prefix_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().prefix())
>> : std::true_type
{};

// has_suffix_method
//   trait: true if _T exposes suffix().
template<typename _T,
         typename = void>
struct has_suffix_method : std::false_type
{};

template<typename _T>
struct has_suffix_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().suffix())
>> : std::true_type
{};

// has_marker_config
//   trait: true if _T exposes both prefix() and suffix().
template<typename _T>
struct has_marker_config
{
    static constexpr bool value =
        ( has_prefix_method<_T>::value &&
          has_suffix_method<_T>::value );
};


// ---------------------------------------------------------------------
//  has_unbind_method
// ---------------------------------------------------------------------

// has_unbind_method
//   trait: true if _T exposes unbind(string).
template<typename _T,
         typename = void>
struct has_unbind_method : std::false_type
{};

template<typename _T>
struct has_unbind_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().unbind(
        std::declval<const std::string&>()))
>> : std::true_type
{};


// ---------------------------------------------------------------------
//  has_clear_bindings_method
// ---------------------------------------------------------------------

// has_clear_bindings_method
//   trait: true if _T exposes clear_bindings() or clear().
template<typename _T,
         typename = void>
struct has_clear_bindings_method : std::false_type
{};

template<typename _T>
struct has_clear_bindings_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().clear_bindings())
>> : std::true_type
{};

template<typename _T,
         typename = void>
struct has_clear_method : std::false_type
{};

template<typename _T>
struct has_clear_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().clear())
>> : std::true_type
{};


// ---------------------------------------------------------------------
//  has_bind_function_method
// ---------------------------------------------------------------------

// has_bind_function_method
//   trait: true if _T exposes bind_function(string, callable).
template<typename _T,
         typename = void>
struct has_bind_function_method : std::false_type
{};

template<typename _T>
struct has_bind_function_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind_function(
        std::declval<const std::string&>(),
        std::declval<std::function<std::string()>>()))
>> : std::true_type
{};


// ---------------------------------------------------------------------
//  has_bind_template_method
// ---------------------------------------------------------------------

// has_bind_template_method
//   trait: true if _T exposes bind_template(string, template&).
template<typename _T,
         typename = void>
struct has_bind_template_method : std::false_type
{};

template<typename _T>
struct has_bind_template_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind_template(
        std::declval<const std::string&>(),
        std::declval<const _T&>()))
>> : std::true_type
{};


// ---------------------------------------------------------------------
//  is_text_template
// ---------------------------------------------------------------------

// is_text_template
//   trait: true if _T satisfies the minimum text template
// protocol: bind(key, value) and render(format) returning
// string.
template<typename _T>
struct is_text_template
{
    static constexpr bool value =
        ( has_bind_method<_T>::value &&
          has_render_method<_T>::value );
};

// is_full_text_template
//   trait: true if _T satisfies the full text template
// protocol including marker configuration and function
// bindings.
template<typename _T>
struct is_full_text_template
{
    static constexpr bool value =
        ( is_text_template<_T>::value       &&
          has_marker_config<_T>::value      &&
          has_bind_function_method<_T>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  TEXT DESTINATION DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_append_method
//   trait: true if _T exposes append(const char*, size_t).
template<typename _T,
         typename = void>
struct has_append_method : std::false_type
{};

template<typename _T>
struct has_append_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().append(
        std::declval<const char*>(),
        std::declval<std::size_t>()))
>> : std::true_type
{};

// is_text_sink_callable
//   trait: true if _T is callable with (const char*, size_t).
template<typename _T,
         typename = void>
struct is_text_sink_callable : std::false_type
{};

template<typename _T>
struct is_text_sink_callable<_T, D_VOID_T<
    decltype(std::declval<_T&>()(
        std::declval<const char*>(),
        std::declval<std::size_t>()))
>> : std::true_type
{};

// is_text_destination
//   trait: composite — true if _T can receive text output
// via append, callable sink, or is a std::string.
template<typename _T>
struct is_text_destination
{
    using clean_type = typename std::remove_cv<
        typename std::remove_reference<_T>::type>::type;

    static constexpr bool value =
        ( has_append_method<clean_type>::value     ||
          is_text_sink_callable<clean_type>::value  ||
          std::is_same<clean_type, std::string>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   CHAINABILITY DETECTION                              ///
///////////////////////////////////////////////////////////////////////////////

// is_chainable_text_function
//   trait: true if _Fn accepts string input and produces
// string output, making it composable in a text function
// chain.
template<typename _Fn>
struct is_chainable_text_function
{
    static constexpr bool value =
        has_string_call_with<_Fn, const std::string&>::value;
};

// is_text_function_pair
//   trait: true if _F1 and _F2 are both chainable text
// functions, enabling composition: _F2(_F1(input)).
template<typename _F1,
         typename _F2>
struct is_text_function_pair
{
    static constexpr bool value =
        ( is_chainable_text_function<_F1>::value &&
          is_chainable_text_function<_F2>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMBINED CLASSIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

// text_function_class
//   struct: comprehensive classification of a text function
// type.
template<typename _T>
struct text_function_class
{
    // identity
    static constexpr bool is_text_fn       =
        is_chainable_text_function<_T>::value;
    static constexpr bool is_nullary       =
        is_nullary_text_function<_T>::value;
    static constexpr bool is_template      =
        is_text_template<_T>::value;
    static constexpr bool is_full_template =
        is_full_text_template<_T>::value;
    static constexpr bool is_specifier     =
        is_text_specifier<_T>::value;
    static constexpr bool is_destination   =
        is_text_destination<_T>::value;

    // template capabilities
    static constexpr bool has_markers      =
        has_marker_config<_T>::value;
    static constexpr bool has_fn_bind      =
        has_bind_function_method<_T>::value;
    static constexpr bool has_tmpl_bind    =
        has_bind_template_method<_T>::value;
    static constexpr bool has_unbind       =
        has_unbind_method<_T>::value;

    // chainability
    static constexpr bool is_chainable     =
        is_chainable_text_function<_T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. VARIABLE TEMPLATES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Fn, typename _Input = const std::string&>
    D_CONSTEXPR bool is_text_function_v =
        is_text_function<_Fn, _Input>::value;

    template<typename _Fn>
    D_CONSTEXPR bool is_nullary_text_function_v =
        is_nullary_text_function<_Fn>::value;

    template<typename _T>
    D_CONSTEXPR bool is_text_specifier_v =
        is_text_specifier<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool is_text_template_v =
        is_text_template<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool is_full_text_template_v =
        is_full_text_template<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool is_text_destination_v =
        is_text_destination<_T>::value;

    template<typename _Fn>
    D_CONSTEXPR bool is_chainable_text_function_v =
        is_chainable_text_function<_Fn>::value;

    template<typename _T>
    D_CONSTEXPR bool has_bind_method_v =
        has_bind_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_render_method_v =
        has_render_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_marker_config_v =
        has_marker_config<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_resolve_method_v =
        has_resolve_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_key_access_v =
        has_key_access<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_value_access_v =
        has_value_access<_T>::value;

#endif  // variable templates


///////////////////////////////////////////////////////////////////////////////
///                VIII. C++20 CONCEPT WRAPPERS                             ///
///////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    // text_function_type
    //   concept: constrains callables that accept string input
    // and produce string output.
    template<typename _Fn>
    concept text_function_type =
        is_chainable_text_function<_Fn>::value;

    // text_template_type
    //   concept: constrains types satisfying the text template
    // protocol.
    template<typename _T>
    concept text_template_type =
        is_text_template<_T>::value;

    // text_specifier_type
    //   concept: constrains types satisfying the text specifier
    // protocol.
    template<typename _T>
    concept text_specifier_type =
        is_text_specifier<_T>::value;

    // text_destination_type
    //   concept: constrains types that can receive text output.
    template<typename _T>
    concept text_destination_type =
        is_text_destination<_T>::value;

    // composable_text_functions
    //   concept: constrains a pair of chainable text functions.
    template<typename _F1, typename _F2>
    concept composable_text_functions =
        is_text_function_pair<_F1, _F2>::value;

#endif  // __cpp_concepts


NS_END  // traits
NS_END  // text
NS_END  // djinterp


#endif  // DJINTERP_TEXT_FUNCTION_TRAITS_
