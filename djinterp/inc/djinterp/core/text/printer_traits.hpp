/******************************************************************************
* djinterp [core]                                          printer_traits.hpp
*
* djinterp printer traits header:
*   This header provides SFINAE-based compile-time detection of printable
* types and output target capabilities. It serves as the trait foundation
* for the print module, enabling tagless dispatch to the correct write
* strategy based on type properties alone.
*
*   DETECTED CAPABILITIES:
*   Output targets:
*     - has_write_method       : .write(const char*, size_t)
*     - has_stream_insertion   : operator<<(ostream&, T)
*     - is_ostream             : std::ostream-derived types
*     - is_file_pointer        : FILE* C-style I/O
*     - is_string_target       : std::string or std::string&
*     - is_buffer_target       : char* /size_t raw buffer pair
*     - is_output_target       : composite — any writable target
*
*   Printable types:
*     - has_to_string          : .to_string() member
*     - has_c_str              : .c_str() member (string-like)
*     - has_data_and_size      : .data() + .size() members
*     - is_string_like         : c_str or data+size or string type
*     - is_c_string            : const char* / char[]
*     - is_arithmetic_printable: arithmetic types (int, float, etc.)
*     - is_printable           : composite — any type the printer
*                                can convert to text
*
*   Indentation:
*     - has_indent_string      : .indent_string() accessor
*     - has_indent_depth       : .indent_depth() accessor
*     - has_set_indent         : .set_indent(str, depth) mutator
*     - has_indent_support     : composite — full indent protocol
*
*   Template-backed rendering:
*     - has_node_template      : .node_template() accessor
*     - has_summary_template   : .summary_template() accessor
*     - has_template_rendering : composite — template-backed printer
*
*   PORTABILITY:
*   This header uses env.h and cpp_features.h for C++ version detection.
* All traits are pure SFINAE — no tag types are introduced.
*
*
* path:      /inc/cpp/io/printer_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_PRINTER_TRAITS_
#define DJINTERP_PRINTER_TRAITS_ 1

#include <cstddef>
#include <cstdio>
#include <type_traits>
#include "../djinterp.hpp"


NS_DJINTERP


// =============================================================================
// I.   OUTPUT TARGET DETECTION
// =============================================================================

// has_write_method
//   trait: detects types with a .write(const char*, std::streamsize)
// member function (e.g. std::ostream, std::ofstream).
template<typename _Type,
         typename = void>
struct has_write_method : std::false_type
{};

template<typename _Type>
struct has_write_method<_Type, void_t<
    decltype(std::declval<_Type&>().write(
        std::declval<const char*>(),
        std::declval<std::streamsize>()))
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_write_method_v = has_write_method<_Type>::value;
#endif


// has_stream_insertion
//   trait: detects types supporting operator<<(const char*).
NS_INTERNAL

    template<typename _Type>
    using stream_insertion_expr = decltype(
        std::declval<_Type&>() << std::declval<const char*>());

NS_END  // internal

template<typename _Type,
         typename = void>
struct has_stream_insertion : std::false_type
{};

template<typename _Type>
struct has_stream_insertion<_Type, void_t<
    internal::stream_insertion_expr<_Type>
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_stream_insertion_v =
        has_stream_insertion<_Type>::value;
#endif


// is_ostream
//   trait: evaluates to true_type if _Type derives from
// std::ostream.
NS_INTERNAL

    template<typename _Type,
             typename = void>
    struct is_ostream_check : std::false_type
    {};

    template<typename _Type>
    struct is_ostream_check<_Type, typename std::enable_if<
        std::is_base_of<std::ostream,
                        typename std::remove_reference<_Type>::type
        >::value
    >::type> : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_ostream : internal::is_ostream_check<_Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_ostream_v = is_ostream<_Type>::value;
#endif


// is_file_pointer
//   trait: evaluates to true_type if _Type is FILE*.
template<typename _Type>
struct is_file_pointer
    : std::is_same<typename std::remove_cv<_Type>::type, std::FILE*>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_file_pointer_v = is_file_pointer<_Type>::value;
#endif


// is_string_target
//   trait: evaluates to true_type if _Type is a
// std::basic_string instantiation.
NS_INTERNAL

    template<typename _Type>
    struct is_std_string_check : std::false_type
    {};

    template<typename _CharT,
             typename _Traits,
             typename _Alloc>
    struct is_std_string_check<std::basic_string<_CharT, _Traits, _Alloc>>
        : std::true_type
    {};

NS_END  // internal

template<typename _Type>
struct is_string_target
    : internal::is_std_string_check<
        typename std::remove_cv<
            typename std::remove_reference<_Type>::type
        >::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_string_target_v = is_string_target<_Type>::value;
#endif


// is_output_target
//   trait: composite — any supported output destination.
template<typename _Type>
struct is_output_target
    : std::integral_constant<bool,
        ( is_ostream<_Type>::value       ||
          is_file_pointer<_Type>::value  ||
          is_string_target<_Type>::value ||
          has_write_method<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_output_target_v = is_output_target<_Type>::value;
#endif


// =============================================================================
// II.  PRINTABLE TYPE DETECTION
// =============================================================================

// has_to_string
//   trait: detects types with a .to_string() member function.
template<typename _Type,
         typename = void>
struct has_to_string : std::false_type
{};

template<typename _Type>
struct has_to_string<_Type, void_t<
    decltype(std::declval<const _Type&>().to_string())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_to_string_v = has_to_string<_Type>::value;
#endif


// has_c_str
//   trait: detects types with a .c_str() member function.
template<typename _Type,
         typename = void>
struct has_c_str : std::false_type
{};

template<typename _Type>
struct has_c_str<_Type, void_t<
    decltype(std::declval<const _Type&>().c_str())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_c_str_v = has_c_str<_Type>::value;
#endif


// has_data_and_size
//   trait: detects types with both .data() and .size() members.
template<typename _Type,
         typename = void>
struct has_data_and_size : std::false_type
{};

template<typename _Type>
struct has_data_and_size<_Type, void_t<
    decltype(std::declval<const _Type&>().data()),
    decltype(std::declval<const _Type&>().size())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_data_and_size_v =
        has_data_and_size<_Type>::value;
#endif


// is_c_string
//   trait: evaluates to true_type if _Type decays to
// const char* or char*.
template<typename _Type>
struct is_c_string
    : std::integral_constant<bool,
        ( std::is_same<typename std::decay<_Type>::type,
                       const char*>::value ||
          std::is_same<typename std::decay<_Type>::type,
                       char*>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_c_string_v = is_c_string<_Type>::value;
#endif


// is_string_like
//   trait: composite — C string, .c_str(), or .data()+.size().
template<typename _Type>
struct is_string_like
    : std::integral_constant<bool,
        ( is_c_string<_Type>::value      ||
          has_c_str<_Type>::value         ||
          has_data_and_size<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_string_like_v = is_string_like<_Type>::value;
#endif


// is_arithmetic_printable
//   trait: any arithmetic type printable via snprintf.
template<typename _Type>
struct is_arithmetic_printable
    : std::is_arithmetic<typename std::remove_cv<
        typename std::remove_reference<_Type>::type
      >::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_arithmetic_printable_v =
        is_arithmetic_printable<_Type>::value;
#endif


// has_stream_insertion_for
//   trait: detects whether _Type supports operator<< into
// an std::ostream.
template<typename _Type,
         typename = void>
struct has_stream_insertion_for : std::false_type
{};

template<typename _Type>
struct has_stream_insertion_for<_Type, void_t<
    decltype(std::declval<std::ostream&>() << std::declval<const _Type&>())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_stream_insertion_for_v =
        has_stream_insertion_for<_Type>::value;
#endif


// is_printable
//   trait: composite — any type the print module can render
// to text.
template<typename _Type>
struct is_printable
    : std::integral_constant<bool,
        ( is_string_like<_Type>::value         ||
          is_arithmetic_printable<_Type>::value ||
          has_to_string<_Type>::value           ||
          has_stream_insertion_for<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_printable_v = is_printable<_Type>::value;
#endif


// =============================================================================
// III. BUFFER TARGET TRAITS
// =============================================================================

// is_char_pointer
//   trait: evaluates to true_type if _Type is char* or char[].
template<typename _Type>
struct is_char_pointer
    : std::integral_constant<bool,
        ( std::is_same<typename std::remove_cv<
            typename std::remove_reference<_Type>::type
          >::type, char*>::value ||
          std::is_same<typename std::decay<_Type>::type,
                       char*>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_char_pointer_v = is_char_pointer<_Type>::value;
#endif


// =============================================================================
// IV.  INDENTATION PROTOCOL DETECTION
// =============================================================================

// has_indent_string
//   trait: detects types exposing indent_string().
template<typename _Type,
         typename = void>
struct has_indent_string : std::false_type
{};

template<typename _Type>
struct has_indent_string<_Type, void_t<
    decltype(std::declval<const _Type&>().indent_string())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_indent_string_v =
        has_indent_string<_Type>::value;
#endif


// has_indent_depth
//   trait: detects types exposing indent_depth().
template<typename _Type,
         typename = void>
struct has_indent_depth : std::false_type
{};

template<typename _Type>
struct has_indent_depth<_Type, void_t<
    decltype(std::declval<const _Type&>().indent_depth())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_indent_depth_v =
        has_indent_depth<_Type>::value;
#endif


// has_set_indent
//   trait: detects types exposing set_indent(string, depth).
template<typename _Type,
         typename = void>
struct has_set_indent : std::false_type
{};

template<typename _Type>
struct has_set_indent<_Type, void_t<
    decltype(std::declval<_Type&>().set_indent(
        std::declval<const std::string&>(),
        std::declval<std::size_t>()))
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_set_indent_v = has_set_indent<_Type>::value;
#endif


// has_indent_support
//   trait: composite — full indentation protocol.
template<typename _Type>
struct has_indent_support
    : std::integral_constant<bool,
        ( has_indent_string<_Type>::value &&
          has_set_indent<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_indent_support_v =
        has_indent_support<_Type>::value;
#endif


// =============================================================================
// V.   TEMPLATE-BACKED RENDERING DETECTION
// =============================================================================

// has_node_template
//   trait: detects types exposing node_template().
template<typename _Type,
         typename = void>
struct has_node_template : std::false_type
{};

template<typename _Type>
struct has_node_template<_Type, void_t<
    decltype(std::declval<const _Type&>().node_template())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_node_template_v =
        has_node_template<_Type>::value;
#endif


// has_summary_template
//   trait: detects types exposing summary_template().
template<typename _Type,
         typename = void>
struct has_summary_template : std::false_type
{};

template<typename _Type>
struct has_summary_template<_Type, void_t<
    decltype(std::declval<const _Type&>().summary_template())
>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_summary_template_v =
        has_summary_template<_Type>::value;
#endif


// has_template_rendering
//   trait: composite — printer backed by text_template
// objects for node and summary rendering.
template<typename _Type>
struct has_template_rendering
    : std::integral_constant<bool,
        ( has_node_template<_Type>::value &&
          has_summary_template<_Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_template_rendering_v =
        has_template_rendering<_Type>::value;
#endif


// =============================================================================
// VI.  COMBINED PRINTER CLASSIFICATION
// =============================================================================

// printer_class
//   struct: comprehensive classification of a printer type.
template<typename _Type>
struct printer_class
{
    // output target
    static constexpr bool is_target     =
        is_output_target<_Type>::value;

    // indentation
    static constexpr bool has_indent    =
        has_indent_support<_Type>::value;

    // template-backed rendering
    static constexpr bool has_templates =
        has_template_rendering<_Type>::value;
};


NS_END  // djinterp


#endif  // DJINTERP_PRINTER_TRAITS_
