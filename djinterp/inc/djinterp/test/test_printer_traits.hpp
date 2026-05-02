/******************************************************************************
* djinterp [test]                                     test_printer_traits.hpp
*
*   Structural SFINAE detection for test printer types.  Detects
* capabilities that distinguish simple printers from full-featured
* template-backed test output engines: symbol functions, numbering,
* section templates, depth-aware indentation, filtering, and
* for-each rendering.
*
*   All detection is purely structural - no tag types.  Expose the
* right members and the trait system classifies you automatically.
*
*
* TABLE OF CONTENTS
* =================
* I.    SYMBOL FUNCTION DETECTION
* II.   NUMBERING DETECTION
* III.  TEMPLATE SECTION DETECTION
* IV.   WALK / FOR-EACH DETECTION
* V.    FILTER DETECTION
* VI.   COMBINED CLASSIFICATION
* VII.  VARIABLE TEMPLATES
*
*
* path:      /inc/djinterp/test/meta/test_printer_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEST_PRINTER_TRAITS_
#define DJINTERP_TEST_PRINTER_TRAITS_ 1

// std
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   SYMBOL FUNCTION DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_symbol_function
//   trait: true if _Type exposes symbol_function() returning
// a callable.
template<typename _Type,
         typename = void>
struct has_symbol_function : std::false_type
{};

template<typename _Type>
struct has_symbol_function<
    _Type, 
    void_t<decltype(std::declval<const _Type&>().symbol_function())>
> : std::true_type
{};

// has_set_symbol_function
//   trait: true if _Type exposes set_symbol_function(fn).
template<typename _Type,
         typename = void>
struct has_set_symbol_function : std::false_type
{};

template<typename _Type>
struct has_set_symbol_function<_Type, void_t<
    decltype(std::declval<_Type&>().set_symbol_function(
        std::declval<std::function<
            std::string(test_status)>>()))
>> : std::true_type
{};

// has_status_string_function
//   trait: true if _Type exposes status_string_function().
template<typename _Type,
         typename = void>
struct has_status_string_function : std::false_type
{};

template<typename _Type>
struct has_status_string_function<_Type, void_t<
    decltype(std::declval<const _Type&>().status_string_function())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                II.  NUMBERING DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

// has_numbering_mode
//   trait: true if _Type exposes numbering_mode().
template<typename _Type,
         typename = void>
struct has_numbering_mode : std::false_type
{};

template<typename _Type>
struct has_numbering_mode<_Type, void_t<
    decltype(std::declval<const _Type&>().numbering_mode())
>> : std::true_type
{};

// has_set_numbering_mode
//   trait: true if _Type exposes set_numbering_mode(...).
template<typename _Type,
         typename = void>
struct has_set_numbering_mode : std::false_type
{};

template<typename _Type>
struct has_set_numbering_mode<_Type, void_t<
    decltype(std::declval<_Type&>().set_numbering_mode(
        std::declval<int>()))
>> : std::true_type
{};

// has_numbering_support
//   trait: composite - full numbering protocol.
template<typename _Type>
struct has_numbering_support
{
    static constexpr bool value =
        ( has_numbering_mode<_Type>::value &&
          has_set_numbering_mode<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                III. TEMPLATE SECTION DETECTION                           ///
///////////////////////////////////////////////////////////////////////////////

// has_header_template
//   trait: true if _Type exposes header_template().
template<typename _Type,
         typename = void>
struct has_header_template : std::false_type
{};

template<typename _Type>
struct has_header_template<_Type, void_t<
    decltype(std::declval<const _Type&>().header_template())
>> : std::true_type
{};

// has_footer_template
//   trait: true if _Type exposes footer_template().
template<typename _Type,
         typename = void>
struct has_footer_template : std::false_type
{};

template<typename _Type>
struct has_footer_template<_Type, void_t<
    decltype(std::declval<const _Type&>().footer_template())
>> : std::true_type
{};

// has_section_header_template
//   trait: true if _Type exposes section_header_template().
template<typename _Type,
         typename = void>
struct has_section_header_template : std::false_type
{};

template<typename _Type>
struct has_section_header_template<_Type, void_t<
    decltype(std::declval<const _Type&>().section_header_template())
>> : std::true_type
{};

// has_section_footer_template
//   trait: true if _Type exposes section_footer_template().
template<typename _Type,
         typename = void>
struct has_section_footer_template : std::false_type
{};

template<typename _Type>
struct has_section_footer_template<_Type, void_t<
    decltype(std::declval<const _Type&>().section_footer_template())
>> : std::true_type
{};

// has_full_section_templates
//   trait: composite - all section templates present.
template<typename _Type>
struct has_full_section_templates
{
    static constexpr bool value =
        ( has_header_template<_Type>::value         &&
          has_footer_template<_Type>::value         &&
          has_section_header_template<_Type>::value  &&
          has_section_footer_template<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  WALK / FOR-EACH DETECTION                            ///
///////////////////////////////////////////////////////////////////////////////

// has_print_node_method
//   trait: true if _Type exposes print_node(...).
template<typename _Type,
         typename = void>
struct has_print_node_method : std::false_type
{};

template<typename _Type>
struct has_print_node_method<_Type, void_t<
    decltype(std::declval<const _Type&>().print_node(
        std::declval<test_status>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<std::size_t>(),
        std::declval<std::size_t>()))
>> : std::true_type
{};

// has_print_summary_method
//   trait: true if _Type exposes print_summary(...).
template<typename _Type,
         typename = void>
struct has_print_summary_method : std::false_type
{};

template<typename _Type>
struct has_print_summary_method<_Type, void_t<
    decltype(std::declval<const _Type&>().print_summary())
>> : std::true_type
{};

// has_print_header_method
//   trait: true if _Type exposes print_header(...).
template<typename _Type,
         typename = void>
struct has_print_header_method : std::false_type
{};

template<typename _Type>
struct has_print_header_method<_Type, void_t<
    decltype(std::declval<const _Type&>().print_header())
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                V.   FILTER DETECTION                                      ///
///////////////////////////////////////////////////////////////////////////////

// has_node_filter_method
//   trait: true if _Type exposes set_node_filter(fn).
template<typename _Type,
         typename = void>
struct has_node_filter_method : std::false_type
{};

template<typename _Type>
struct has_node_filter_method<_Type, void_t<
    decltype(std::declval<_Type&>().set_node_filter(
        std::declval<std::function<
            bool(test_status, std::size_t)>>()))
>> : std::true_type
{};

// has_status_filter
//   trait: true if _Type exposes set_print_passing(bool)
// and set_print_skipped(bool).
template<typename _Type,
         typename = void>
struct has_status_filter : std::false_type
{};

template<typename _Type>
struct has_status_filter<_Type, void_t<
    decltype(std::declval<_Type&>().set_print_passing(
        std::declval<bool>())),
    decltype(std::declval<_Type&>().set_print_skipped(
        std::declval<bool>()))
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                VI.  COMBINED CLASSIFICATION                              ///
///////////////////////////////////////////////////////////////////////////////

// test_printer_class
//   struct: comprehensive classification of a test printer.
template<typename _Type>
struct test_printer_class
{
    // symbol customization
    static constexpr bool has_symbol_fn =
        has_symbol_function<_Type>::value;
    static constexpr bool has_set_symbol_fn =
        has_set_symbol_function<_Type>::value;
    static constexpr bool has_status_str_fn =
        has_status_string_function<_Type>::value;

    // numbering
    static constexpr bool has_numbering =
        has_numbering_support<_Type>::value;

    // template sections
    static constexpr bool has_header =
        has_header_template<_Type>::value;
    static constexpr bool has_footer =
        has_footer_template<_Type>::value;
    static constexpr bool has_section_hdr =
        has_section_header_template<_Type>::value;
    static constexpr bool has_section_ftr =
        has_section_footer_template<_Type>::value;
    static constexpr bool has_all_sections =
        has_full_section_templates<_Type>::value;

    // rendering
    static constexpr bool has_print_node =
        has_print_node_method<_Type>::value;
    static constexpr bool has_print_summary =
        has_print_summary_method<_Type>::value;
    static constexpr bool has_print_header =
        has_print_header_method<_Type>::value;

    // filtering
    static constexpr bool has_custom_filter =
        has_node_filter_method<_Type>::value;
    static constexpr bool has_status_filters =
        has_status_filter<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. VARIABLE TEMPLATES                                   ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool has_symbol_function_v =
        has_symbol_function<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_numbering_support_v =
        has_numbering_support<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_full_section_templates_v =
        has_full_section_templates<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_print_node_method_v =
        has_print_node_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_node_filter_method_v =
        has_node_filter_method<_Type>::value;

#endif  // variable templates


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_PRINTER_TRAITS_