/******************************************************************************
* djinterp [pdf]                                       pdf_template_traits.hpp
*
*   Structural SFINAE detection for the PDF subsystem.  Classifies three
* families of types without tag types or base-class checks - expose the
* right members and the trait system recognizes you automatically:
*
*     1.  PDF backends      - the common-subset drawing protocol from
*                             pdf.hpp (begin/end document and page, draw
*                             text/line/rect, metadata, capabilities,
*                             serialize/save).  Detection is duck-typed, so
*                             an adapter need not derive from pdf_backend.
*     2.  PDF documents     - the pdf_document façade surface (open/close,
*                             add_page, text, save/to_bytes).
*     3.  PDF templates     - the renderable-to-document protocol introduced
*                             by pdf_template.hpp (render_pdf / render_to_pdf).
*
*   This mirrors the layering of text_template_traits.hpp over
* text_function_traits.hpp: granular per-method traits, composite protocol
* traits, an aggregate classification struct, _v variable templates on
* C++14+, and concept wrappers behind a feature gate.
*
*   PORTABILITY:
*   C++11: all traits via struct::value
*   C++14: _v variable templates
*   C++20: concept wrappers behind __cpp_concepts gate
*
*
* TABLE OF CONTENTS
* =================
* I.    BACKEND METHOD DETECTION
* II.   BACKEND COMPOSITE
* III.  DOCUMENT METHOD DETECTION
* IV.   DOCUMENT COMPOSITE
* V.    TEMPLATE RENDER DETECTION
* VI.   COMBINED CLASSIFICATION
* VII.  VARIABLE TEMPLATES
* VIII. C++20 CONCEPT WRAPPERS
*
*
* path:      /inc/djinterp/core/pdf/pdf_template_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_PDF_TEMPLATE_TRAITS_
#define DJINTERP_PDF_TEMPLATE_TRAITS_ 1

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./pdf.hpp"


NS_DJINTERP
NS_PDF


///////////////////////////////////////////////////////////////////////////////
///                I.   BACKEND METHOD DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

// has_begin_document_method
//   trait: true if _Type exposes begin_document().
template<typename _Type,
         typename = void>
struct has_begin_document_method : std::false_type
{};

template<typename _Type>
struct has_begin_document_method<_Type, void_t<
    decltype(std::declval<_Type&>().begin_document())
>> : std::true_type
{};

// has_end_document_method
//   trait: true if _Type exposes end_document().
template<typename _Type,
         typename = void>
struct has_end_document_method : std::false_type
{};

template<typename _Type>
struct has_end_document_method<_Type, void_t<
    decltype(std::declval<_Type&>().end_document())
>> : std::true_type
{};

// has_begin_page_method
//   trait: true if _Type exposes begin_page(pdf_size).
template<typename _Type,
         typename = void>
struct has_begin_page_method : std::false_type
{};

template<typename _Type>
struct has_begin_page_method<_Type, void_t<
    decltype(std::declval<_Type&>().begin_page(
        std::declval<const pdf_size&>()))
>> : std::true_type
{};

// has_end_page_method
//   trait: true if _Type exposes end_page().
template<typename _Type,
         typename = void>
struct has_end_page_method : std::false_type
{};

template<typename _Type>
struct has_end_page_method<_Type, void_t<
    decltype(std::declval<_Type&>().end_page())
>> : std::true_type
{};

// has_draw_text_method
//   trait: true if _Type exposes draw_text(point, text, font, color).
template<typename _Type,
         typename = void>
struct has_draw_text_method : std::false_type
{};

template<typename _Type>
struct has_draw_text_method<_Type, void_t<
    decltype(std::declval<_Type&>().draw_text(
        std::declval<const pdf_point&>(),
        std::declval<const std::string&>(),
        std::declval<const pdf_font&>(),
        std::declval<const pdf_color&>()))
>> : std::true_type
{};

// has_draw_line_method
//   trait: true if _Type exposes draw_line(point, point, paint).
template<typename _Type,
         typename = void>
struct has_draw_line_method : std::false_type
{};

template<typename _Type>
struct has_draw_line_method<_Type, void_t<
    decltype(std::declval<_Type&>().draw_line(
        std::declval<const pdf_point&>(),
        std::declval<const pdf_point&>(),
        std::declval<const pdf_paint&>()))
>> : std::true_type
{};

// has_draw_rect_method
//   trait: true if _Type exposes draw_rect(rect, paint).
template<typename _Type,
         typename = void>
struct has_draw_rect_method : std::false_type
{};

template<typename _Type>
struct has_draw_rect_method<_Type, void_t<
    decltype(std::declval<_Type&>().draw_rect(
        std::declval<const pdf_rect&>(),
        std::declval<const pdf_paint&>()))
>> : std::true_type
{};

// has_set_metadata_method
//   trait: true if _Type exposes set_metadata(key, value).
template<typename _Type,
         typename = void>
struct has_set_metadata_method : std::false_type
{};

template<typename _Type>
struct has_set_metadata_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_metadata(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};

// has_capabilities_method
//   trait: true if _Type exposes capabilities().
template<typename _Type,
         typename = void>
struct has_capabilities_method : std::false_type
{};

template<typename _Type>
struct has_capabilities_method<_Type, void_t<
    decltype(std::declval<const _Type&>().capabilities())
>> : std::true_type
{};

// has_serialize_method
//   trait: true if _Type exposes serialize().
template<typename _Type,
         typename = void>
struct has_serialize_method : std::false_type
{};

template<typename _Type>
struct has_serialize_method<_Type, void_t<
    decltype(std::declval<_Type&>().serialize())
>> : std::true_type
{};

// has_save_method
//   trait: true if _Type exposes save(const char*).
template<typename _Type,
         typename = void>
struct has_save_method : std::false_type
{};

template<typename _Type>
struct has_save_method<_Type, void_t<
    decltype(std::declval<_Type&>().save(
        std::declval<const char*>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                II.  BACKEND COMPOSITE                                    ///
///////////////////////////////////////////////////////////////////////////////

// has_document_lifecycle
//   trait: true if _Type exposes the document begin/end pair.
template<typename _Type>
struct has_document_lifecycle
{
    static constexpr bool value =
        ( has_begin_document_method<_Type>::value &&
          has_end_document_method<_Type>::value );
};

// has_page_lifecycle
//   trait: true if _Type exposes the page begin/end pair.
template<typename _Type>
struct has_page_lifecycle
{
    static constexpr bool value =
        ( has_begin_page_method<_Type>::value &&
          has_end_page_method<_Type>::value );
};

// has_drawing_protocol
//   trait: true if _Type exposes the common-subset drawing
// operations (text, line, rect).
template<typename _Type>
struct has_drawing_protocol
{
    static constexpr bool value =
        ( has_draw_text_method<_Type>::value &&
          has_draw_line_method<_Type>::value &&
          has_draw_rect_method<_Type>::value );
};

// has_output_protocol
//   trait: true if _Type exposes both serialize() and save().
template<typename _Type>
struct has_output_protocol
{
    static constexpr bool value =
        ( has_serialize_method<_Type>::value &&
          has_save_method<_Type>::value );
};

// is_pdf_backend
//   trait: composite - _Type satisfies the full common-subset
// backend protocol structurally, whether or not it derives from
// pdf_backend.
template<typename _Type>
struct is_pdf_backend
{
    static constexpr bool value =
        ( has_document_lifecycle<_Type>::value     &&
          has_page_lifecycle<_Type>::value         &&
          has_drawing_protocol<_Type>::value       &&
          has_set_metadata_method<_Type>::value    &&
          has_capabilities_method<_Type>::value    &&
          has_output_protocol<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III. DOCUMENT METHOD DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_open_method
//   trait: true if _Type exposes open().
template<typename _Type,
         typename = void>
struct has_open_method : std::false_type
{};

template<typename _Type>
struct has_open_method<_Type, void_t<
    decltype(std::declval<_Type&>().open())
>> : std::true_type
{};

// has_close_method
//   trait: true if _Type exposes close().
template<typename _Type,
         typename = void>
struct has_close_method : std::false_type
{};

template<typename _Type>
struct has_close_method<_Type, void_t<
    decltype(std::declval<_Type&>().close())
>> : std::true_type
{};

// has_add_page_method
//   trait: true if _Type exposes add_page(pdf_page_size).
template<typename _Type,
         typename = void>
struct has_add_page_method : std::false_type
{};

template<typename _Type>
struct has_add_page_method<_Type, void_t<
    decltype(std::declval<_Type&>().add_page(
        std::declval<const pdf_page_size&>()))
>> : std::true_type
{};

// has_text_method
//   trait: true if _Type exposes text(point, string, options).
template<typename _Type,
         typename = void>
struct has_text_method : std::false_type
{};

template<typename _Type>
struct has_text_method<_Type, void_t<
    decltype(std::declval<_Type&>().text(
        std::declval<const pdf_point&>(),
        std::declval<const std::string&>(),
        std::declval<const pdf_text_options&>()))
>> : std::true_type
{};

// has_to_bytes_method
//   trait: true if _Type exposes to_bytes().
template<typename _Type,
         typename = void>
struct has_to_bytes_method : std::false_type
{};

template<typename _Type>
struct has_to_bytes_method<_Type, void_t<
    decltype(std::declval<_Type&>().to_bytes())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                IV.  DOCUMENT COMPOSITE                                   ///
///////////////////////////////////////////////////////////////////////////////

// is_pdf_document
//   trait: composite - _Type satisfies the document façade
// surface (lifecycle, paging, text, and output).
template<typename _Type>
struct is_pdf_document
{
    static constexpr bool value =
        ( has_open_method<_Type>::value       &&
          has_close_method<_Type>::value      &&
          has_add_page_method<_Type>::value   &&
          has_text_method<_Type>::value       &&
          has_save_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TEMPLATE RENDER DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_render_pdf_method
//   trait: true if _Type exposes render_pdf() returning a
// serialized document (a std::string of PDF bytes).
template<typename _Type,
         typename = void>
struct has_render_pdf_method : std::false_type
{};

template<typename _Type>
struct has_render_pdf_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_pdf())
>> : std::true_type
{};

// has_render_to_pdf_method
//   trait: true if _Type exposes render_to_pdf(pdf_document&),
// drawing itself into a caller-supplied document.
template<typename _Type,
         typename = void>
struct has_render_to_pdf_method : std::false_type
{};

template<typename _Type>
struct has_render_to_pdf_method<_Type, void_t<
    decltype(std::declval<const _Type&>().render_to_pdf(
        std::declval<pdf_document&>()))
>> : std::true_type
{};

// has_save_pdf_method
//   trait: true if _Type exposes save_pdf(const char*).
template<typename _Type,
         typename = void>
struct has_save_pdf_method : std::false_type
{};

template<typename _Type>
struct has_save_pdf_method<_Type, void_t<
    decltype(std::declval<const _Type&>().save_pdf(
        std::declval<const char*>()))
>> : std::true_type
{};

// is_pdf_renderable
//   trait: composite - _Type can render itself to a PDF document
// via at least the render_to_pdf protocol.
template<typename _Type>
struct is_pdf_renderable
{
    static constexpr bool value =
        ( has_render_to_pdf_method<_Type>::value ||
          has_render_pdf_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMBINED CLASSIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

// pdf_class
//   struct: comprehensive classification of a PDF subsystem type,
// covering backend, document, and template-render capabilities.
template<typename _Type>
struct pdf_class
{
    // -----------------------------------------------------------------
    // Backend Protocol
    // -----------------------------------------------------------------
    static constexpr bool has_doc_lifecycle =
        has_document_lifecycle<_Type>::value;
    static constexpr bool has_page_lifecycle =
        djinterp::pdf::has_page_lifecycle<_Type>::value;
    static constexpr bool has_drawing =
        has_drawing_protocol<_Type>::value;
    static constexpr bool has_metadata =
        has_set_metadata_method<_Type>::value;
    static constexpr bool has_caps =
        has_capabilities_method<_Type>::value;
    static constexpr bool has_output =
        has_output_protocol<_Type>::value;
    static constexpr bool is_backend =
        is_pdf_backend<_Type>::value;

    // -----------------------------------------------------------------
    // Document Façade
    // -----------------------------------------------------------------
    static constexpr bool has_paging =
        has_add_page_method<_Type>::value;
    static constexpr bool has_text =
        has_text_method<_Type>::value;
    static constexpr bool is_document =
        is_pdf_document<_Type>::value;

    // -----------------------------------------------------------------
    // Template Rendering
    // -----------------------------------------------------------------
    static constexpr bool has_render_pdf =
        has_render_pdf_method<_Type>::value;
    static constexpr bool has_render_to_pdf =
        has_render_to_pdf_method<_Type>::value;
    static constexpr bool has_save_pdf =
        has_save_pdf_method<_Type>::value;
    static constexpr bool is_renderable =
        is_pdf_renderable<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. VARIABLE TEMPLATES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_draw_text_method_v =
        has_draw_text_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_draw_line_method_v =
        has_draw_line_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_draw_rect_method_v =
        has_draw_rect_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_drawing_protocol_v =
        has_drawing_protocol<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_output_protocol_v =
        has_output_protocol<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_pdf_backend_v =
        is_pdf_backend<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_pdf_document_v =
        is_pdf_document<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_render_to_pdf_method_v =
        has_render_to_pdf_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_pdf_renderable_v =
        is_pdf_renderable<_Type>::value;

#endif  // variable templates


///////////////////////////////////////////////////////////////////////////////
///                VIII. C++20 CONCEPT WRAPPERS                             ///
///////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    // pdf_backend_type
    //   concept: constrains types satisfying the full common-subset
    // PDF backend protocol structurally.
    template<typename _Type>
    concept pdf_backend_type =
        is_pdf_backend<_Type>::value;

    // pdf_document_type
    //   concept: constrains types satisfying the document façade
    // surface.
    template<typename _Type>
    concept pdf_document_type =
        is_pdf_document<_Type>::value;

    // pdf_renderable_type
    //   concept: constrains types that can render themselves into a
    // PDF document.
    template<typename _Type>
    concept pdf_renderable_type =
        is_pdf_renderable<_Type>::value;

#endif  // __cpp_concepts


NS_END  // pdf
NS_END  // djinterp


#endif  // DJINTERP_PDF_TEMPLATE_TRAITS_
