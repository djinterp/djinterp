/******************************************************************************
* djinterp [pdf]                                              pdf_concepts.hpp
*
* PDF concepts:
*   C++20 concepts layered over pdf_template_traits.hpp. These concepts give
* readable constraints for PDF backends, documents, and renderable templates
* without replacing the SFINAE trait surface they wrap.
*
*   This header complements the small built-in concept block in
* pdf_template_traits.hpp (pdf_backend_type, pdf_document_type,
* pdf_renderable_type), which it intentionally does not redefine.  It mirrors
* the structure of printer_concepts.hpp and text_template_concepts.hpp:
*     1.  Backend protocol concepts
*     2.  Document façade concepts
*     3.  Template / renderable concepts
*     4.  Capability concepts
*     5.  Aggregate profile concepts
*
*
* path:      /inc/djinterp/core/pdf/pdf_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.22
******************************************************************************/

#ifndef DJINTERP_PDF_CONCEPTS_
#define DJINTERP_PDF_CONCEPTS_ 1

#ifndef __cplusplus
    #error "pdf_concepts.hpp requires C++ compilation"
#endif

#include "./pdf_template_traits.hpp"


NS_DJINTERP
NS_PDF

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   BACKEND PROTOCOL CONCEPTS
// ===========================================================================

// document_lifecycle_backend
//   concept: the type exposes begin_document() and end_document().
template<typename _Type>
concept document_lifecycle_backend =
    has_document_lifecycle<_Type>::value;

// page_lifecycle_backend
//   concept: the type exposes begin_page(size) and end_page().
template<typename _Type>
concept page_lifecycle_backend =
    has_page_lifecycle<_Type>::value;

// text_drawing_backend
//   concept: the type exposes draw_text(...).
template<typename _Type>
concept text_drawing_backend =
    has_draw_text_method<_Type>::value;

// vector_drawing_backend
//   concept: the type exposes both draw_line and draw_rect.
template<typename _Type>
concept vector_drawing_backend =
    has_draw_line_method<_Type>::value &&
    has_draw_rect_method<_Type>::value;

// drawing_backend
//   concept: the type exposes the full common-subset drawing
// protocol (text, line, rect).
template<typename _Type>
concept drawing_backend =
    has_drawing_protocol<_Type>::value;

// metadata_backend
//   concept: the type exposes set_metadata(key, value).
template<typename _Type>
concept metadata_backend =
    has_set_metadata_method<_Type>::value;

// serializable_backend
//   concept: the type exposes serialize() and save(path).
template<typename _Type>
concept serializable_backend =
    has_output_protocol<_Type>::value;

// common_subset_backend
//   concept: shorthand for the full structural backend protocol
// (equivalent to pdf_backend_type from the traits header, restated
// here for symmetry with the other concept families).
template<typename _Type>
concept common_subset_backend =
    is_pdf_backend<_Type>::value;


// ===========================================================================
// II.  DOCUMENT FACADE CONCEPTS
// ===========================================================================

// openable_document
//   concept: the type exposes open() and close().
template<typename _Type>
concept openable_document =
    has_open_method<_Type>::value &&
    has_close_method<_Type>::value;

// paged_document
//   concept: the type exposes add_page(size).
template<typename _Type>
concept paged_document =
    has_add_page_method<_Type>::value;

// text_document
//   concept: the type exposes text(point, string, options).
template<typename _Type>
concept text_document =
    has_text_method<_Type>::value;

// savable_document
//   concept: the type exposes save(path).
template<typename _Type>
concept savable_document =
    has_save_method<_Type>::value;

// document_facade
//   concept: shorthand for the full document façade surface.
template<typename _Type>
concept document_facade =
    is_pdf_document<_Type>::value;


// ===========================================================================
// III. TEMPLATE / RENDERABLE CONCEPTS
// ===========================================================================

// pdf_byte_renderable
//   concept: the type exposes render_pdf() returning serialized
// document bytes.
template<typename _Type>
concept pdf_byte_renderable =
    has_render_pdf_method<_Type>::value;

// pdf_document_renderable
//   concept: the type exposes render_to_pdf(document&).
template<typename _Type>
concept pdf_document_renderable =
    has_render_to_pdf_method<_Type>::value;

// pdf_savable_template
//   concept: the type exposes save_pdf(path).
template<typename _Type>
concept pdf_savable_template =
    has_save_pdf_method<_Type>::value;

// renderable_pdf_template
//   concept: the type can render itself to a PDF document via at
// least one supported path.
template<typename _Type>
concept renderable_pdf_template =
    is_pdf_renderable<_Type>::value;

// full_pdf_template
//   concept: a renderable template that supports every render
// path - to a document, to bytes, and to disk.
template<typename _Type>
concept full_pdf_template =
    has_render_to_pdf_method<_Type>::value &&
    has_render_pdf_method<_Type>::value    &&
    has_save_pdf_method<_Type>::value;


// ===========================================================================
// IV.  CAPABILITY CONCEPTS
// ===========================================================================

// capability_reporting_backend
//   concept: the type exposes capabilities().
template<typename _Type>
concept capability_reporting_backend =
    has_capabilities_method<_Type>::value;


// ===========================================================================
// V.   AGGREGATE PROFILE CONCEPTS
// ===========================================================================

// backend_profile
//   concept: shorthand for the backend role recognized by pdf_class.
template<typename _Type>
concept backend_profile =
    pdf_class<_Type>::is_backend;

// document_profile
//   concept: shorthand for the document role recognized by pdf_class.
template<typename _Type>
concept document_profile =
    pdf_class<_Type>::is_document;

// renderable_profile
//   concept: shorthand for the renderable role recognized by
// pdf_class.
template<typename _Type>
concept renderable_profile =
    pdf_class<_Type>::is_renderable;

// complete_pdf_backend
//   concept: a backend that both satisfies the common subset and
// reports its capabilities and vector-drawing support.
template<typename _Type>
concept complete_pdf_backend =
    pdf_class<_Type>::is_backend  &&
    pdf_class<_Type>::has_caps    &&
    pdf_class<_Type>::has_drawing;

#endif  // __cpp_concepts >= 201907L


NS_END  // pdf
NS_END  // djinterp


#endif  // DJINTERP_PDF_CONCEPTS_
