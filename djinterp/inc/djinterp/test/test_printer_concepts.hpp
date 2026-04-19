/******************************************************************************
* djinterp [test]                                    test_printer_concepts.hpp
*
* Test printer concepts:
*   C++20 concepts layered over test_printer_traits.hpp. These concepts
* provide readable constraints for test-printer-like types without replacing
* the existing SFINAE trait surface.
*
*   The concepts mirror the public classification axes from
* test_printer_traits.hpp:
*   - symbol customization
*   - numbering support
*   - section templates
*   - rendering hooks
*   - filtering support
*   - aggregate printer profiles
*
*
* path:      /inc/djinterp/test/test_printer_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.12
******************************************************************************/

#ifndef DJINTERP_TEST_PRINTER_CONCEPTS_
#define DJINTERP_TEST_PRINTER_CONCEPTS_ 1

#ifndef __cplusplus
    #error "test_printer_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "../core/djinterp.hpp"
#include "./test_printer_traits.hpp"


NS_DJINTERP
NS_TEST

///////////////////////////////////////////////////////////////////////////////
///                I.   SYMBOL CUSTOMIZATION CONCEPTS                       ///
///////////////////////////////////////////////////////////////////////////////

// symbol_function_test_printer
//   concept: the printer exposes symbol_function().
template<typename _Type>
concept symbol_function_test_printer =
    has_symbol_function<_Type>::value;

// symbol_mutable_test_printer
//   concept: the printer exposes set_symbol_function(...).
template<typename _Type>
concept symbol_mutable_test_printer =
    has_set_symbol_function<_Type>::value;

// status_string_test_printer
//   concept: the printer exposes status_string_function().
template<typename _Type>
concept status_string_test_printer =
    has_status_string_function<_Type>::value;

// symbol_customizable_test_printer
//   concept: the printer supports both reading and replacing the symbol
// mapping function.
template<typename _Type>
concept symbol_customizable_test_printer =
    has_symbol_function<_Type>::value &&
    has_set_symbol_function<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                II.  NUMBERING CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// numbered_test_printer
//   concept: the printer exposes the full numbering protocol.
template<typename _Type>
concept numbered_test_printer =
    has_numbering_support<_Type>::value;

// numbering_query_test_printer
//   concept: the printer exposes numbering_mode().
template<typename _Type>
concept numbering_query_test_printer =
    has_numbering_mode<_Type>::value;

// numbering_mutable_test_printer
//   concept: the printer exposes set_numbering_mode(...).
template<typename _Type>
concept numbering_mutable_test_printer =
    has_set_numbering_mode<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                III. TEMPLATE SECTION CONCEPTS                           ///
///////////////////////////////////////////////////////////////////////////////

// header_template_test_printer
//   concept: the printer exposes header_template().
template<typename _Type>
concept header_template_test_printer =
    has_header_template<_Type>::value;

// footer_template_test_printer
//   concept: the printer exposes footer_template().
template<typename _Type>
concept footer_template_test_printer =
    has_footer_template<_Type>::value;

// section_header_template_test_printer
//   concept: the printer exposes section_header_template().
template<typename _Type>
concept section_header_template_test_printer =
    has_section_header_template<_Type>::value;

// section_footer_template_test_printer
//   concept: the printer exposes section_footer_template().
template<typename _Type>
concept section_footer_template_test_printer =
    has_section_footer_template<_Type>::value;

// templated_section_test_printer
//   concept: the printer exposes the full set of section templates.
template<typename _Type>
concept templated_section_test_printer =
    has_full_section_templates<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                IV.  RENDERING CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// node_rendering_test_printer
//   concept: the printer exposes print_node(...).
template<typename _Type>
concept node_rendering_test_printer =
    has_print_node_method<_Type>::value;

// summary_rendering_test_printer
//   concept: the printer exposes print_summary().
template<typename _Type>
concept summary_rendering_test_printer =
    has_print_summary_method<_Type>::value;

// header_rendering_test_printer
//   concept: the printer exposes print_header().
template<typename _Type>
concept header_rendering_test_printer =
    has_print_header_method<_Type>::value;

// rendering_test_printer
//   concept: the printer exposes the common rendering surface used by
// full-featured printers.
template<typename _Type>
concept rendering_test_printer =
    has_print_node_method<_Type>::value &&
    has_print_summary_method<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                V.   FILTERING CONCEPTS                                  ///
///////////////////////////////////////////////////////////////////////////////

// custom_filter_test_printer
//   concept: the printer exposes set_node_filter(...).
template<typename _Type>
concept custom_filter_test_printer =
    has_node_filter_method<_Type>::value;

// status_filter_test_printer
//   concept: the printer exposes passing/skipped filter toggles.
template<typename _Type>
concept status_filter_test_printer =
    has_status_filter<_Type>::value;

// filtered_test_printer
//   concept: the printer exposes at least one filtering mechanism.
template<typename _Type>
concept filtered_test_printer =
    has_node_filter_method<_Type>::value ||
    has_status_filter<_Type>::value;


///////////////////////////////////////////////////////////////////////////////
///                VI.  AGGREGATE PROFILE CONCEPTS                          ///
///////////////////////////////////////////////////////////////////////////////

// classified_test_printer
//   concept: shorthand for any type recognized by test_printer_class.
template<typename _Type>
concept classified_test_printer =
    ( test_printer_class<_Type>::has_print_node ||
      test_printer_class<_Type>::has_print_summary ||
      test_printer_class<_Type>::has_symbol_fn );

// configurable_test_printer
//   concept: the printer supports runtime customization across at least one
// major axis.
template<typename _Type>
concept configurable_test_printer =
    symbol_mutable_test_printer<_Type> ||
    numbering_mutable_test_printer<_Type> ||
    filtered_test_printer<_Type>;

// sectioned_test_printer
//   concept: the printer supports templated section framing plus header
// rendering.
template<typename _Type>
concept sectioned_test_printer =
    templated_section_test_printer<_Type> &&
    header_rendering_test_printer<_Type>;

// full_test_printer
//   concept: a rich printer with symbol customization, numbering,
// templated sections, rendering, and filtering support.
template<typename _Type>
concept full_test_printer =
    symbol_customizable_test_printer<_Type> &&
    numbered_test_printer<_Type> &&
    templated_section_test_printer<_Type> &&
    rendering_test_printer<_Type> &&
    filtered_test_printer<_Type>;


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_PRINTER_CONCEPTS_