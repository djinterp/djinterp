/******************************************************************************
* djinterp [text]                                         printer_concepts.hpp
*
* Printer concepts:
*   C++20 concepts layered over printer_traits.hpp. These concepts provide
* readable constraints for printable values and output targets without
* replacing the existing SFINAE trait surface.
*
*   The concepts mirror the public trait surface from printer_traits.hpp:
*     1.  Output targets
*     2.  Printable values
*     3.  Buffer targets
*     4.  Indentation support
*     5.  Template-backed rendering
*     6.  Aggregate printer profiles
*
*
* path:      /inc/djinterp/core/text/printer_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.15
******************************************************************************/

#ifndef DJINTERP_PRINTER_CONCEPTS_
#define DJINTERP_PRINTER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "printer_concepts.hpp requires C++ compilation"
#endif

#include "printer_traits.hpp"


NS_DJINTERP

#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// I.   OUTPUT TARGET CONCEPTS
// ===========================================================================

// write_method_target
//   concept: the type exposes write(const char*, std::streamsize).
template<typename _Type>
concept write_method_target =
    has_write_method<_Type>::value;

// stream_insertion_target
//   concept: the type supports operator<<(const char*).
template<typename _Type>
concept stream_insertion_target =
    has_stream_insertion<_Type>::value;

// ostream_target
//   concept: the type is std::ostream-derived.
template<typename _Type>
concept ostream_target =
    is_ostream<_Type>::value;

// file_pointer_target
//   concept: the type is FILE*.
template<typename _Type>
concept file_pointer_target =
    is_file_pointer<_Type>::value;

// string_output_target
//   concept: the type is a std::basic_string target.
template<typename _Type>
concept string_output_target =
    is_string_target<_Type>::value;

// output_target_type
//   concept: the type satisfies any printer output target path.
template<typename _Type>
concept output_target_type =
    is_output_target<_Type>::value;


// ===========================================================================
// II.  PRINTABLE VALUE CONCEPTS
// ===========================================================================

// to_string_printable
//   concept: the type exposes to_string().
template<typename _Type>
concept to_string_printable =
    has_to_string<_Type>::value;

// c_str_printable
//   concept: the type exposes c_str().
template<typename _Type>
concept c_str_printable =
    has_c_str<_Type>::value;

// data_size_printable
//   concept: the type exposes data() and size().
template<typename _Type>
concept data_size_printable =
    has_data_and_size<_Type>::value;

// c_string_printable
//   concept: the type is a C string.
template<typename _Type>
concept c_string_printable =
    is_c_string<_Type>::value;

// string_like_printable
//   concept: the type satisfies the string-like printable profile.
template<typename _Type>
concept string_like_printable =
    is_string_like<_Type>::value;

// arithmetic_printable
//   concept: the type is printable as an arithmetic value.
template<typename _Type>
concept arithmetic_printable =
    is_arithmetic_printable<_Type>::value;

// ostream_insertable_printable
//   concept: the type supports insertion into std::ostream.
template<typename _Type>
concept ostream_insertable_printable =
    has_stream_insertion_for<_Type>::value;

// printable_type
//   concept: the type satisfies any printable-value path.
template<typename _Type>
concept printable_type =
    is_printable<_Type>::value;


// ===========================================================================
// III. BUFFER TARGET CONCEPTS
// ===========================================================================

// char_pointer_target
//   concept: the type is a char pointer target.
template<typename _Type>
concept char_pointer_target =
    is_char_pointer<_Type>::value;


// ===========================================================================
// IV.  INDENTATION CONCEPTS
// ===========================================================================

// indent_string_printer
//   concept: the printer exposes indent_string().
template<typename _Type>
concept indent_string_printer =
    has_indent_string<_Type>::value;

// indent_depth_printer
//   concept: the printer exposes indent_depth().
template<typename _Type>
concept indent_depth_printer =
    has_indent_depth<_Type>::value;

// indent_mutable_printer
//   concept: the printer exposes set_indent(string, depth).
template<typename _Type>
concept indent_mutable_printer =
    has_set_indent<_Type>::value;

// indentation_printer
//   concept: the printer satisfies the full indentation protocol.
template<typename _Type>
concept indentation_printer =
    has_indent_support<_Type>::value;


// ===========================================================================
// V.   TEMPLATE RENDERING CONCEPTS
// ===========================================================================

// node_template_printer
//   concept: the printer exposes node_template().
template<typename _Type>
concept node_template_printer =
    has_node_template<_Type>::value;

// summary_template_printer
//   concept: the printer exposes summary_template().
template<typename _Type>
concept summary_template_printer =
    has_summary_template<_Type>::value;

// template_rendering_printer
//   concept: the printer exposes node and summary templates.
template<typename _Type>
concept template_rendering_printer =
    has_template_rendering<_Type>::value;


// ===========================================================================
// VI.  AGGREGATE PROFILE CONCEPTS
// ===========================================================================

// printer_target_type
//   concept: shorthand for output targets recognized by printer_class.
template<typename _Type>
concept printer_target_type =
    printer_class<_Type>::is_target;

// indented_printer_type
//   concept: shorthand for indentation-capable printers.
template<typename _Type>
concept indented_printer_type =
    printer_class<_Type>::has_indent;

// templated_printer_type
//   concept: shorthand for template-backed printers.
template<typename _Type>
concept templated_printer_type =
    printer_class<_Type>::has_templates;

// full_printer_type
//   concept: a printer target with indentation and template rendering support.
template<typename _Type>
concept full_printer_type =
    printer_class<_Type>::is_target     &&
    printer_class<_Type>::has_indent    &&
    printer_class<_Type>::has_templates;

#endif  // __cpp_concepts >= 201907L


NS_END  // djinterp


#endif  // DJINTERP_PRINTER_CONCEPTS_
