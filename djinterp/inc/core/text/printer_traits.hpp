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
*   PORTABILITY:
*   This header uses env.h and cpp_features.h for C++ version detection.
* All traits are pure SFINAE — no tag types are introduced.
*
* path:      /inc/cpp/io/printer_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
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
// Traits in this section detect whether a given type can serve as an output
// destination for the print module.


// -----------------------------------------------------------------------------
// A.  has_write_method
// -----------------------------------------------------------------------------

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

// has_write_method_v
//   value: convenience alias for has_write_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_write_method_v = has_write_method<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// B.  has_stream_insertion
// -----------------------------------------------------------------------------

NS_INTERNAL
    // stream_insertion_expr
    //   trait: helper alias for detecting operator<< with const char*.
    template<typename _Type>
    using stream_insertion_expr = decltype(
        std::declval<_Type&>() << std::declval<const char*>());

NS_END  // internal

// has_stream_insertion
//   trait: detects types supporting operator<<(const char*), typically
// std::ostream and its derivatives.
template<typename _Type,
         typename = void>
struct has_stream_insertion : std::false_type
{};

template<typename _Type>
struct has_stream_insertion<_Type, void_t<
    internal::stream_insertion_expr<_Type>
>> : std::true_type
{};

// has_stream_insertion_v
//   value: convenience alias for has_stream_insertion<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_stream_insertion_v =
        has_stream_insertion<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// C.  is_ostream
// -----------------------------------------------------------------------------

NS_INTERNAL

    // is_ostream_check
    //   trait: helper that tests derivation from std::ostream via
    // SFINAE on pointer convertibility.
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

// is_ostream
//   trait: evaluates to true_type if _Type derives from std::ostream.
template<typename _Type>
struct is_ostream : internal::is_ostream_check<_Type>
{};

// is_ostream_v
//   value: convenience alias for is_ostream<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_ostream_v = is_ostream<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// D.  is_file_pointer
// -----------------------------------------------------------------------------

// is_file_pointer
//   trait: evaluates to true_type if _Type is FILE* (C-style I/O).
template<typename _Type>
struct is_file_pointer
    : std::is_same<typename std::remove_cv<_Type>::type, std::FILE*>
{};

// is_file_pointer_v
//   value: convenience alias for is_file_pointer<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_file_pointer_v = is_file_pointer<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// E.  is_string_target
// -----------------------------------------------------------------------------

NS_INTERNAL

    // is_std_string_check
    //   trait: helper that matches std::basic_string instantiations.
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

// is_string_target
//   trait: evaluates to true_type if _Type is a std::basic_string
// instantiation (e.g. std::string). Strips cv-qualifiers and
// references before testing.
template<typename _Type>
struct is_string_target
    : internal::is_std_string_check<
        typename std::remove_cv<
            typename std::remove_reference<_Type>::type
        >::type>
{};

// is_string_target_v
//   value: convenience alias for is_string_target<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_string_target_v = is_string_target<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// F.  is_output_target
// -----------------------------------------------------------------------------

// is_output_target
//   trait: composite detection — evaluates to true_type if _Type is
// any supported output destination (ostream, FILE*, std::string, or
// any type with a .write() method).
template<typename _Type>
struct is_output_target
    : std::integral_constant<bool,
        ( is_ostream<_Type>::value        ||
          is_file_pointer<_Type>::value   ||
          is_string_target<_Type>::value  ||
          has_write_method<_Type>::value )>
{};

// is_output_target_v
//   value: convenience alias for is_output_target<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_output_target_v = is_output_target<_Type>::value;
#endif


// =============================================================================
// II.  PRINTABLE TYPE DETECTION
// =============================================================================
// Traits in this section detect whether a given type can be converted to
// text for printing.


// -----------------------------------------------------------------------------
// A.  has_to_string
// -----------------------------------------------------------------------------

// has_to_string
//   trait: detects types with a .to_string() member function returning
// something convertible to const char* or std::string.
template<typename _Type,
         typename = void>
struct has_to_string : std::false_type
{};

template<typename _Type>
struct has_to_string<_Type, void_t<
    decltype(std::declval<const _Type&>().to_string())
>> : std::true_type
{};

// has_to_string_v
//   value: convenience alias for has_to_string<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_to_string_v = has_to_string<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// B.  has_c_str
// -----------------------------------------------------------------------------

// has_c_str
//   trait: detects types with a .c_str() member function (string-like
// types such as std::string, std::string_view).
template<typename _Type,
         typename = void>
struct has_c_str : std::false_type
{};

template<typename _Type>
struct has_c_str<_Type, void_t<
    decltype(std::declval<const _Type&>().c_str())
>> : std::true_type
{};

// has_c_str_v
//   value: convenience alias for has_c_str<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_c_str_v = has_c_str<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// C.  has_data_and_size
// -----------------------------------------------------------------------------

// has_data_and_size
//   trait: detects types with both .data() and .size() members,
// enabling contiguous buffer access (e.g. std::string, std::vector<char>,
// std::string_view, std::span<char>).
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

// has_data_and_size_v
//   value: convenience alias for has_data_and_size<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_data_and_size_v =
        has_data_and_size<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// D.  is_c_string
// -----------------------------------------------------------------------------

NS_INTERNAL

    // remove_cv_ref
    //   trait: helper to strip cv-qualifiers and references for older
    // C++ standards. Equivalent to std::remove_cvref_t in C++20.
    template<typename _Type>
    struct remove_cv_ref
    {
        using type = typename std::remove_cv<
            typename std::remove_reference<_Type>::type
        >::type;
    };

    // remove_cv_ref_t
    //   type: convenience alias for remove_cv_ref<_Type>::type.
    template<typename _Type>
    using remove_cv_ref_t = typename remove_cv_ref<_Type>::type;

NS_END  // internal

// is_c_string
//   trait: evaluates to true_type if _Type decays to const char*
// or char* (including char arrays).
template<typename _Type>
struct is_c_string
    : std::integral_constant<bool,
        ( std::is_same<typename std::decay<_Type>::type,
                       const char*>::value ||
          std::is_same<typename std::decay<_Type>::type,
                       char*>::value )>
{};

// is_c_string_v
//   value: convenience alias for is_c_string<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_c_string_v = is_c_string<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// E.  is_string_like
// -----------------------------------------------------------------------------

// is_string_like
//   trait: composite detection — evaluates to true_type if _Type is a
// C string, has .c_str(), or has .data() + .size(). Covers
// std::string, std::string_view, const char*, char[], and similar.
template<typename _Type>
struct is_string_like
    : std::integral_constant<bool,
        ( is_c_string<_Type>::value       ||
          has_c_str<_Type>::value          ||
          has_data_and_size<_Type>::value )>
{};

// is_string_like_v
//   value: convenience alias for is_string_like<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_string_like_v = is_string_like<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// F.  is_arithmetic_printable
// -----------------------------------------------------------------------------

// is_arithmetic_printable
//   trait: evaluates to true_type if _Type is an arithmetic type
// (integral or floating-point) that can be printed via snprintf or
// std::to_string.
template<typename _Type>
struct is_arithmetic_printable
    : std::is_arithmetic<typename std::remove_cv<
        typename std::remove_reference<_Type>::type
      >::type>
{};

// is_arithmetic_printable_v
//   value: convenience alias for is_arithmetic_printable<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_arithmetic_printable_v =
        is_arithmetic_printable<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// G.  has_stream_insertion_for
// -----------------------------------------------------------------------------

// has_stream_insertion_for
//   trait: detects whether _Type supports operator<< into an
// std::ostream. Distinct from has_stream_insertion which tests if a
// type itself IS a stream target — this tests if a type can be SENT
// to a stream.
template<typename _Type,
         typename = void>
struct has_stream_insertion_for : std::false_type
{};

template<typename _Type>
struct has_stream_insertion_for<_Type, void_t<
    decltype(std::declval<std::ostream&>() << std::declval<const _Type&>())
>> : std::true_type
{};

// has_stream_insertion_for_v
//   value: convenience alias for has_stream_insertion_for<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_stream_insertion_for_v =
        has_stream_insertion_for<_Type>::value;
#endif


// -----------------------------------------------------------------------------
// H.  is_printable
// -----------------------------------------------------------------------------

// is_printable
//   trait: composite detection — evaluates to true_type if _Type can
// be printed by the print module via any supported mechanism:
// string-like, arithmetic, has .to_string(), or supports operator<<
// into an ostream.
template<typename _Type>
struct is_printable
    : std::integral_constant<bool,
        ( is_string_like<_Type>::value           ||
          is_arithmetic_printable<_Type>::value   ||
          has_to_string<_Type>::value             ||
          has_stream_insertion_for<_Type>::value )>
{};

// is_printable_v
//   value: convenience alias for is_printable<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_printable_v = is_printable<_Type>::value;
#endif


// =============================================================================
// III. BUFFER TARGET TRAITS
// =============================================================================
// Traits for raw char buffer output (char* + size_t pairs). These are
// not struct-based SFINAE but rather support traits used by the
// buffer_writer in print.hpp.


// -----------------------------------------------------------------------------
// A.  is_char_pointer
// -----------------------------------------------------------------------------

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

// is_char_pointer_v
//   value: convenience alias for is_char_pointer<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_char_pointer_v = is_char_pointer<_Type>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_PRINTER_TRAITS_
