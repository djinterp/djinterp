/******************************************************************************
* djinterp [text]                                    text_template_traits.hpp
*
*   Structural SFINAE detection for the full text template protocol.
* Extends text_function_traits.hpp with detection for advanced binding
* types (conditional, section, list, transform), rendering variants,
* escape configuration, and section-block support.
*
*   All detection is purely structural — no tag types, no base-class
* checks.  Expose the right members and the trait system classifies
* you automatically.
*
*   DEPENDENCIES:
*   text_function_traits.hpp — base text function and simple template
*                              detection (is_text_template,
*                              has_bind_method, has_render_method,
*                              has_marker_config).  This header builds
*                              on those traits without duplicating them.
*
*   COMPAT:
*   C++11: all traits via struct::value
*   C++14: _v variable templates
*   C++20: concept wrappers behind feature gate
*
*
* TABLE OF CONTENTS
* =================
* I.    ADVANCED BINDING DETECTION
* II.   SECTION BLOCK DETECTION
* III.  RENDERING VARIANT DETECTION
* IV.   ESCAPE AND DEPTH DETECTION
* V.    BINDING MANAGEMENT DETECTION
* VI.   COMBINED CLASSIFICATION
* VII.  VARIABLE TEMPLATES
* VIII. C++20 CONCEPT WRAPPERS
*
*
* path:      /inc/text/meta/text_template_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEXT_TEMPLATE_TRAITS_
#define DJINTERP_TEXT_TEMPLATE_TRAITS_ 1

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./text_function_traits.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   ADVANCED BINDING DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_bind_conditional_method
//   trait: true if _T exposes bind_conditional(key, pred,
// true_val, false_val).
template<typename _T,
         typename = void>
struct has_bind_conditional_method : std::false_type
{};

template<typename _T>
struct has_bind_conditional_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind_conditional(
        std::declval<const std::string&>(),
        std::declval<std::function<bool()>>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_bind_section_method
//   trait: true if _T exposes bind_section(key, predicate).
template<typename _T,
         typename = void>
struct has_bind_section_method : std::false_type
{};

template<typename _T>
struct has_bind_section_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind_section(
        std::declval<const std::string&>(),
        std::declval<std::function<bool()>>()))
>> : std::true_type
{};


// has_bind_list_method
//   trait: true if _T exposes a bind_list method.  Detection
// is loose — any bind_list accepting at least (key, ...)
// qualifies.
template<typename _T,
         typename = void>
struct has_bind_list_method : std::false_type
{};

template<typename _T>
struct has_bind_list_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind_list(
        std::declval<const std::string&>(),
        std::declval<_T&>(),
        std::declval<std::size_t>(),
        std::declval<std::function<void(_T&,
            std::size_t, std::size_t)>>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};


// has_bind_transform_method
//   trait: true if _T exposes bind_transform(key, source,
// transform_fn).
template<typename _T,
         typename = void>
struct has_bind_transform_method : std::false_type
{};

template<typename _T>
struct has_bind_transform_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().bind_transform(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<std::function<
            std::string(const std::string&)>>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION BLOCK DETECTION                              ///
///////////////////////////////////////////////////////////////////////////////

// has_section_open_marker
//   trait: true if _T exposes section_open_marker() or
// section_prefix().
template<typename _T,
         typename = void>
struct has_section_open_marker : std::false_type
{};

template<typename _T>
struct has_section_open_marker<_T, D_VOID_T<
    decltype(std::declval<const _T&>().section_open_marker())
>> : std::true_type
{};

// has_section_close_marker
//   trait: true if _T exposes section_close_marker().
template<typename _T,
         typename = void>
struct has_section_close_marker : std::false_type
{};

template<typename _T>
struct has_section_close_marker<_T, D_VOID_T<
    decltype(std::declval<const _T&>().section_close_marker())
>> : std::true_type
{};

// has_section_invert_marker
//   trait: true if _T exposes section_invert_marker().
template<typename _T,
         typename = void>
struct has_section_invert_marker : std::false_type
{};

template<typename _T>
struct has_section_invert_marker<_T, D_VOID_T<
    decltype(std::declval<const _T&>().section_invert_marker())
>> : std::true_type
{};

// has_section_support
//   trait: true if _T exposes section block markers and
// bind_section.
template<typename _T>
struct has_section_support
{
    static constexpr bool value =
        ( has_section_open_marker<_T>::value  &&
          has_section_close_marker<_T>::value &&
          has_bind_section_method<_T>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III. RENDERING VARIANT DETECTION                          ///
///////////////////////////////////////////////////////////////////////////////

// has_render_to_method
//   trait: true if _T exposes render_to(target, format).
template<typename _T,
         typename = void>
struct has_render_to_method : std::false_type
{};

template<typename _T>
struct has_render_to_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().render_to(
        std::declval<std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};

// has_render_with_method
//   trait: true if _T exposes render_with(format, extra_bindings)
// for one-shot additional bindings.
template<typename _T,
         typename = void>
struct has_render_with_method : std::false_type
{};

template<typename _T>
struct has_render_with_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().render_with(
        std::declval<const std::string&>(),
        std::declval<const std::vector<
            std::pair<std::string, std::string>>&>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                IV.  ESCAPE AND DEPTH DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_escape_char_method
//   trait: true if _T exposes escape_char().
template<typename _T,
         typename = void>
struct has_escape_char_method : std::false_type
{};

template<typename _T>
struct has_escape_char_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().escape_char())
>> : std::true_type
{};

// has_set_escape_char_method
//   trait: true if _T exposes set_escape_char(char).
template<typename _T,
         typename = void>
struct has_set_escape_char_method : std::false_type
{};

template<typename _T>
struct has_set_escape_char_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().set_escape_char(
        std::declval<char>()))
>> : std::true_type
{};

// has_max_depth_method
//   trait: true if _T exposes max_depth().
template<typename _T,
         typename = void>
struct has_max_depth_method : std::false_type
{};

template<typename _T>
struct has_max_depth_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().max_depth())
>> : std::true_type
{};

// has_set_max_depth_method
//   trait: true if _T exposes set_max_depth(n).
template<typename _T,
         typename = void>
struct has_set_max_depth_method : std::false_type
{};

template<typename _T>
struct has_set_max_depth_method<_T, D_VOID_T<
    decltype(std::declval<_T&>().set_max_depth(
        std::declval<std::size_t>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                V.   BINDING MANAGEMENT DETECTION                         ///
///////////////////////////////////////////////////////////////////////////////

// has_has_binding_method
//   trait: true if _T exposes has_binding(key).
template<typename _T,
         typename = void>
struct has_has_binding_method : std::false_type
{};

template<typename _T>
struct has_has_binding_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().has_binding(
        std::declval<const std::string&>()))
>> : std::true_type
{};

// has_binding_count_method
//   trait: true if _T exposes binding_count().
template<typename _T,
         typename = void>
struct has_binding_count_method : std::false_type
{};

template<typename _T>
struct has_binding_count_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().binding_count())
>> : std::true_type
{};

// has_binding_keys_method
//   trait: true if _T exposes binding_keys() returning
// an iterable of key strings.
template<typename _T,
         typename = void>
struct has_binding_keys_method : std::false_type
{};

template<typename _T>
struct has_binding_keys_method<_T, D_VOID_T<
    decltype(std::declval<const _T&>().binding_keys())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMBINED CLASSIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

// text_template_class
//   struct: comprehensive classification of a text template
// type.  Extends text_function_class with advanced binding
// and section detection.
template<typename _T>
struct text_template_class
{
    // -----------------------------------------------------------------
    // Core Protocol (from text_function_traits.hpp)
    // -----------------------------------------------------------------
    static constexpr bool is_template =
        is_text_template<_T>::value;
    static constexpr bool is_full_template =
        is_full_text_template<_T>::value;
    static constexpr bool has_markers =
        has_marker_config<_T>::value;
    static constexpr bool has_render =
        has_render_method<_T>::value;
    static constexpr bool has_bind =
        has_bind_method<_T>::value;

    // -----------------------------------------------------------------
    // Advanced Binding Capabilities
    // -----------------------------------------------------------------
    static constexpr bool has_fn_bind =
        has_bind_function_method<_T>::value;
    static constexpr bool has_tmpl_bind =
        has_bind_template_method<_T>::value;
    static constexpr bool has_list_bind =
        has_bind_list_method<_T>::value;
    static constexpr bool has_conditional_bind =
        has_bind_conditional_method<_T>::value;
    static constexpr bool has_section_bind =
        has_bind_section_method<_T>::value;
    static constexpr bool has_transform_bind =
        has_bind_transform_method<_T>::value;

    // -----------------------------------------------------------------
    // Section Block Support
    // -----------------------------------------------------------------
    static constexpr bool has_sections =
        has_section_support<_T>::value;
    static constexpr bool has_invert_section =
        has_section_invert_marker<_T>::value;

    // -----------------------------------------------------------------
    // Rendering Variants
    // -----------------------------------------------------------------
    static constexpr bool has_render_to =
        has_render_to_method<_T>::value;
    static constexpr bool has_render_with =
        has_render_with_method<_T>::value;

    // -----------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------
    static constexpr bool has_escape =
        has_escape_char_method<_T>::value;
    static constexpr bool has_depth_config =
        ( has_max_depth_method<_T>::value &&
          has_set_max_depth_method<_T>::value );

    // -----------------------------------------------------------------
    // Binding Management
    // -----------------------------------------------------------------
    static constexpr bool has_unbind =
        has_unbind_method<_T>::value;
    static constexpr bool has_has_binding =
        has_has_binding_method<_T>::value;
    static constexpr bool has_count =
        has_binding_count_method<_T>::value;
    static constexpr bool has_keys =
        has_binding_keys_method<_T>::value;

    // -----------------------------------------------------------------
    // Composability
    // -----------------------------------------------------------------
    static constexpr bool is_text_fn =
        is_chainable_text_function<_T>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. VARIABLE TEMPLATES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _T>
    D_CONSTEXPR bool has_bind_conditional_method_v =
        has_bind_conditional_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_bind_section_method_v =
        has_bind_section_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_bind_list_method_v =
        has_bind_list_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_bind_transform_method_v =
        has_bind_transform_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_section_support_v =
        has_section_support<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_render_to_method_v =
        has_render_to_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_render_with_method_v =
        has_render_with_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_escape_char_method_v =
        has_escape_char_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_max_depth_method_v =
        has_max_depth_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_has_binding_method_v =
        has_has_binding_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_binding_count_method_v =
        has_binding_count_method<_T>::value;

    template<typename _T>
    D_CONSTEXPR bool has_binding_keys_method_v =
        has_binding_keys_method<_T>::value;

#endif  // variable templates


///////////////////////////////////////////////////////////////////////////////
///                VIII. C++20 CONCEPT WRAPPERS                             ///
///////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    // full_text_template_type
    //   concept: constrains types satisfying the full text
    // template protocol with advanced bindings.
    template<typename _T>
    concept full_text_template_type =
        ( is_text_template<_T>::value           &&
          has_bind_function_method<_T>::value    &&
          has_bind_list_method<_T>::value );

    // section_aware_template_type
    //   concept: constrains templates with section block
    // support.
    template<typename _T>
    concept section_aware_template_type =
        ( is_text_template<_T>::value    &&
          has_section_support<_T>::value );

    // configurable_template_type
    //   concept: constrains templates with full configuration
    // (markers, escape, depth).
    template<typename _T>
    concept configurable_template_type =
        ( is_text_template<_T>::value               &&
          has_marker_config<_T>::value               &&
          has_escape_char_method<_T>::value           &&
          has_max_depth_method<_T>::value );

#endif  // __cpp_concepts


NS_END  // djinterp


#endif  // DJINTERP_TEXT_TEMPLATE_TRAITS_