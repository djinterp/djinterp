/*******************************************************************************
* djinterp [core]                                        pdf_template_traits.hpp
*
*   Structural SFINAE detection for the PDF subsystem.  Classifies two
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
* path:      /inc/djinterp/core/util/pdf/pdf_template_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.22
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    BACKEND METHOD DETECTION
      ------------------------

II.   BACKEND COMPOSITE
      -----------------

III.  DOCUMENT METHOD DETECTION
      -------------------------

IV.   DOCUMENT COMPOSITE
      ------------------

V.    COMBINED CLASSIFICATION
      -----------------------

VI.   VARIABLE TEMPLATES
      ------------------

VII.  C++20 CONCEPT WRAPPERS
      ----------------------
*/

#ifndef DJINTERP_UTIL_PDF_PDF_TEMPLATE_TRAITS_HPP
#define DJINTERP_UTIL_PDF_PDF_TEMPLATE_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <string>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "./pdf.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   BACKEND METHOD DETECTION                             ///
///////////////////////////////////////////////////////////////////////////////

// has_begin_document_method
//   trait: true if Type exposes begin_document().
template<typename Type,
         typename = void>
struct has_begin_document_method : std::false_type
{};

template<typename Type>
struct has_begin_document_method<Type, void_t<
    decltype(std::declval<Type&>().begin_document())
>> : std::true_type
{};

// has_end_document_method
//   trait: true if Type exposes end_document().
template<typename Type,
         typename = void>
struct has_end_document_method : std::false_type
{};

template<typename Type>
struct has_end_document_method<Type, void_t<
    decltype(std::declval<Type&>().end_document())
>> : std::true_type
{};

// has_begin_page_method
//   trait: true if Type exposes begin_page(pdf_size).
template<typename Type,
         typename = void>
struct has_begin_page_method : std::false_type
{};

template<typename Type>
struct has_begin_page_method<Type, void_t<
    decltype(std::declval<Type&>().begin_page(
        std::declval<const pdf_size&>()))
>> : std::true_type
{};

// has_end_page_method
//   trait: true if Type exposes end_page().
template<typename Type,
         typename = void>
struct has_end_page_method : std::false_type
{};

template<typename Type>
struct has_end_page_method<Type, void_t<
    decltype(std::declval<Type&>().end_page())
>> : std::true_type
{};

// has_draw_text_method
//   trait: true if Type exposes draw_text(point, text, font, color).
template<typename Type,
         typename = void>
struct has_draw_text_method : std::false_type
{};

template<typename Type>
struct has_draw_text_method<Type, void_t<
    decltype(std::declval<Type&>().draw_text(
        std::declval<const pdf_point&>(),
        std::declval<const std::string&>(),
        std::declval<const pdf_font&>(),
        std::declval<const pdf_color&>()))
>> : std::true_type
{};

// has_draw_line_method
//   trait: true if Type exposes draw_line(point, point, paint).
template<typename Type,
         typename = void>
struct has_draw_line_method : std::false_type
{};

template<typename Type>
struct has_draw_line_method<Type, void_t<
    decltype(std::declval<Type&>().draw_line(
        std::declval<const pdf_point&>(),
        std::declval<const pdf_point&>(),
        std::declval<const pdf_paint&>()))
>> : std::true_type
{};

// has_draw_rect_method
//   trait: true if Type exposes draw_rect(rect, paint).
template<typename Type,
         typename = void>
struct has_draw_rect_method : std::false_type
{};

template<typename Type>
struct has_draw_rect_method<Type, void_t<
    decltype(std::declval<Type&>().draw_rect(
        std::declval<const pdf_rect&>(),
        std::declval<const pdf_paint&>()))
>> : std::true_type
{};

// has_set_metadata_method
//   trait: true if Type exposes set_metadata(key, value).
template<typename Type,
         typename = void>
struct has_set_metadata_method : std::false_type
{};

template<typename Type>
struct has_set_metadata_method<Type, void_t<
    decltype(std::declval<Type&>().set_metadata(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()))
>> : std::true_type
{};

// has_capabilities_method
//   trait: true if Type exposes capabilities().
template<typename Type,
         typename = void>
struct has_capabilities_method : std::false_type
{};

template<typename Type>
struct has_capabilities_method<Type, void_t<
    decltype(std::declval<const Type&>().capabilities())
>> : std::true_type
{};

// has_serialize_method
//   trait: true if Type exposes serialize().
template<typename Type,
         typename = void>
struct has_serialize_method : std::false_type
{};

template<typename Type>
struct has_serialize_method<Type, void_t<
    decltype(std::declval<Type&>().serialize())
>> : std::true_type
{};

// has_save_method
//   trait: true if Type exposes save(const char*).
template<typename Type,
         typename = void>
struct has_save_method : std::false_type
{};

template<typename Type>
struct has_save_method<Type, void_t<
    decltype(std::declval<Type&>().save(
        std::declval<const char*>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                II.  BACKEND COMPOSITE                                    ///
///////////////////////////////////////////////////////////////////////////////

// has_document_lifecycle
//   trait: true if Type exposes the document begin/end pair.
template<typename Type>
struct has_document_lifecycle
{
    static constexpr bool value =
        ( has_begin_document_method<clean_t<Type>>::value &&
          has_end_document_method<clean_t<Type>>::value );
};

// has_page_lifecycle
//   trait: true if Type exposes the page begin/end pair.
template<typename Type>
struct has_page_lifecycle
{
    static constexpr bool value =
        ( has_begin_page_method<clean_t<Type>>::value &&
          has_end_page_method<clean_t<Type>>::value );
};

// has_drawing_protocol
//   trait: true if Type exposes the common-subset drawing
// operations (text, line, rect).
template<typename Type>
struct has_drawing_protocol
{
    static constexpr bool value =
        ( has_draw_text_method<clean_t<Type>>::value &&
          has_draw_line_method<clean_t<Type>>::value &&
          has_draw_rect_method<clean_t<Type>>::value );
};

// has_output_protocol
//   trait: true if Type exposes both serialize() and save().
template<typename Type>
struct has_output_protocol
{
    static constexpr bool value =
        ( has_serialize_method<clean_t<Type>>::value &&
          has_save_method<clean_t<Type>>::value );
};

// is_pdf_backend
//   trait: composite - Type satisfies the full common-subset
// backend protocol structurally, whether or not it derives from
// pdf_backend.
template<typename Type>
struct is_pdf_backend
{
    static constexpr bool value =
        ( has_document_lifecycle<clean_t<Type>>::value     &&
          has_page_lifecycle<clean_t<Type>>::value         &&
          has_drawing_protocol<clean_t<Type>>::value       &&
          has_set_metadata_method<clean_t<Type>>::value    &&
          has_capabilities_method<clean_t<Type>>::value    &&
          has_output_protocol<clean_t<Type>>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III. DOCUMENT METHOD DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_open_method
//   trait: true if Type exposes open().
template<typename Type,
         typename = void>
struct has_open_method : std::false_type
{};

template<typename Type>
struct has_open_method<Type, void_t<
    decltype(std::declval<Type&>().open())
>> : std::true_type
{};

// has_close_method
//   trait: true if Type exposes close().
template<typename Type,
         typename = void>
struct has_close_method : std::false_type
{};

template<typename Type>
struct has_close_method<Type, void_t<
    decltype(std::declval<Type&>().close())
>> : std::true_type
{};

// has_add_page_method
//   trait: true if Type exposes add_page(pdf_page_size).
template<typename Type,
         typename = void>
struct has_add_page_method : std::false_type
{};

template<typename Type>
struct has_add_page_method<Type, void_t<
    decltype(std::declval<Type&>().add_page(
        std::declval<const pdf_page_size&>()))
>> : std::true_type
{};

// has_text_method
//   trait: true if Type exposes text(point, string, options).
template<typename Type,
         typename = void>
struct has_text_method : std::false_type
{};

template<typename Type>
struct has_text_method<Type, void_t<
    decltype(std::declval<Type&>().text(
        std::declval<const pdf_point&>(),
        std::declval<const std::string&>(),
        std::declval<const pdf_text_options&>()))
>> : std::true_type
{};

// has_to_bytes_method
//   trait: true if Type exposes to_bytes().
template<typename Type,
         typename = void>
struct has_to_bytes_method : std::false_type
{};

template<typename Type>
struct has_to_bytes_method<Type, void_t<
    decltype(std::declval<Type&>().to_bytes())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                IV.  DOCUMENT COMPOSITE                                   ///
///////////////////////////////////////////////////////////////////////////////

// is_pdf_document
//   trait: composite - Type satisfies the document façade
// surface (lifecycle, paging, text, and output).
template<typename Type>
struct is_pdf_document
{
    static constexpr bool value =
        ( has_open_method<clean_t<Type>>::value       &&
          has_close_method<clean_t<Type>>::value      &&
          has_add_page_method<clean_t<Type>>::value   &&
          has_text_method<clean_t<Type>>::value       &&
          has_save_method<clean_t<Type>>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   COMBINED CLASSIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

// pdf_class
//   struct: comprehensive classification of a PDF subsystem type,
// covering backend, document, and template-render capabilities.
template<typename Type>
struct pdf_class
{
    // -----------------------------------------------------------------
    // Backend Protocol
    // -----------------------------------------------------------------
    static constexpr bool has_doc_lifecycle =
        has_document_lifecycle<clean_t<Type>>::value;
    static constexpr bool has_page_lifecycle =
        ::djinterp::has_page_lifecycle<clean_t<Type>>::value;
    static constexpr bool has_drawing =
        has_drawing_protocol<clean_t<Type>>::value;
    static constexpr bool has_metadata =
        has_set_metadata_method<clean_t<Type>>::value;
    static constexpr bool has_caps =
        has_capabilities_method<clean_t<Type>>::value;
    static constexpr bool has_output =
        has_output_protocol<clean_t<Type>>::value;
    static constexpr bool is_backend =
        is_pdf_backend<clean_t<Type>>::value;

    // -----------------------------------------------------------------
    // Document Façade
    // -----------------------------------------------------------------
    static constexpr bool has_paging =
        has_add_page_method<clean_t<Type>>::value;
    static constexpr bool has_text =
        has_text_method<clean_t<Type>>::value;
    static constexpr bool is_document =
        is_pdf_document<clean_t<Type>>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  VARIABLE TEMPLATES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename Type>
    D_CONSTEXPR bool has_draw_text_method_v =
        has_draw_text_method<clean_t<Type>>::value;

    template<typename Type>
    D_CONSTEXPR bool has_draw_line_method_v =
        has_draw_line_method<clean_t<Type>>::value;

    template<typename Type>
    D_CONSTEXPR bool has_draw_rect_method_v =
        has_draw_rect_method<clean_t<Type>>::value;

    template<typename Type>
    D_CONSTEXPR bool has_drawing_protocol_v =
        has_drawing_protocol<clean_t<Type>>::value;

    template<typename Type>
    D_CONSTEXPR bool has_output_protocol_v =
        has_output_protocol<clean_t<Type>>::value;

    template<typename Type>
    D_CONSTEXPR bool is_pdf_backend_v =
        is_pdf_backend<clean_t<Type>>::value;

    template<typename Type>
    D_CONSTEXPR bool is_pdf_document_v =
        is_pdf_document<clean_t<Type>>::value;

#endif  // variable templates


///////////////////////////////////////////////////////////////////////////////
///                VII. C++20 CONCEPT WRAPPERS                              ///
///////////////////////////////////////////////////////////////////////////////

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

    // pdf_backend_type
    //   concept: constrains types satisfying the full common-subset
    // PDF backend protocol structurally.
    template<typename Type>
    concept pdf_backend_type =
        is_pdf_backend<clean_t<Type>>::value;

    // pdf_document_type
    //   concept: constrains types satisfying the document façade
    // surface.
    template<typename Type>
    concept pdf_document_type =
        is_pdf_document<clean_t<Type>>::value;

#endif  // __cpp_concepts


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_PDF_PDF_TEMPLATE_TRAITS_HPP
