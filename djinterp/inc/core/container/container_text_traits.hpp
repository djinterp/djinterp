/******************************************************************************
* djinterp [container]                              container_text_traits.hpp
*
* Container-aware text output traits for the djinterp framework.
*   Bridges the container classification system (container_traits.hpp)
* with the foundational printer traits (printer_traits.hpp) to provide
* compile-time detection of which text operations a container and its
* elements support.
*
*   Element-level printability is determined by applying the printer_traits
* (djinterp::is_printable, djinterp::has_to_string,
* djinterp::has_stream_insertion_for, etc.) to the container's
* value_type.  Container-level detection covers framework-specific
* methods (.print(), .write() to buffer) and the C-interop function
* pointer protocol (fn_print, fn_to_string, fn_write).
*
*   Detection is organized into three output layers:
*     1. print     — formatted output to std::ostream or FILE*.
*     2. write     — raw character output to a sized buffer
*                    (char*, size_t).
*     3. to_string — conversion to std::string.
*
*   All detection is purely structural SFINAE.
*
* DEPENDENCIES:
*   printer_traits.hpp    - type-level printability detection
*   print.hpp             - write_to / write_value_to dispatch
*   container_traits.hpp  - container classification
*
* TABLE OF CONTENTS
* =================
* I.      Container-Level Method Detection
* II.     Element-Level Detection Helpers (internal)
* III.    Element Printability (via printer_traits)
* IV.     Element String Convertibility
* V.      Element Buffer Writability
* VI.     Element Format Support (C++20)
* VII.    Container Streamability
* VIII.   C-Interop Protocol Detection
* IX.     Text Strategy Classification
* X.      Convenience Predicates
* XI.     Combined Classification
*
*
* path:      \inc\container\meta\container_text_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TEXT_TRAITS_
#define DJINTERP_CONTAINER_TEXT_TRAITS_ 1

#include <cstddef>
#include <cstdio>
#include <iosfwd>
#include <string>
#include <type_traits>
#include "..\djinterp.hpp"
#include "..\type_traits.hpp"
#include "..\io\printer_traits.hpp"
#include "container_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS

// =============================================================================
// I.   Container-Level Method Detection
// =============================================================================
// These traits detect whether the container type itself exposes
// print(), write-to-buffer, or to_string() member functions
// matching the djinterp container protocol.
//
// Note: container-level .to_string() and .c_str() detection
// reuses djinterp::has_to_string and djinterp::has_c_str from
// printer_traits.hpp (applied to the container type directly).

// has_print_method
//   type trait: true if _Type has a
// .print(std::ostream&) member function.
D_TYPE_TRAIT_TRUE(has_print_method,
    decltype(std::declval<const _Type&>().print(
        std::declval<std::ostream&>())))

// has_print_to_file_method
//   type trait: true if _Type has a .print(FILE*) member
// function (C-interop path).
D_TYPE_TRAIT_TRUE(has_print_to_file_method,
    decltype(std::declval<const _Type&>().print(
        std::declval<std::FILE*>())))

// has_buffer_write_method
//   type trait: true if _Type has a
// .write(char*, std::size_t) member function returning
// the number of characters written.
// note: distinct from djinterp::has_write_method in
// printer_traits.hpp, which detects the ostream-style
// .write(const char*, streamsize) for output targets.
D_TYPE_TRAIT_TRUE(has_buffer_write_method,
    decltype(std::declval<const _Type&>().write(
        std::declval<char*>(),
        std::declval<std::size_t>())))


// =============================================================================
// II.  Element-Level Detection Helpers (internal)
// =============================================================================
// Extracts value_type from a container and applies
// printer_traits from djinterp:: to it.

NS_INTERNAL

    // safe_value_type
    //   helper: extracts value_type if present, otherwise
    // yields void.  Prevents hard errors when probing
    // non-container types.
    template<typename _Type, typename = void>
    struct safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using safe_value_type_t =
        typename safe_value_type<_Type>::type;

    // --- element buffer-write detection ---
    // element has a .write(char*, size_t) method (distinct
    // from ostream-style write detected by printer_traits).

    template<typename _Elem, typename = void>
    struct element_has_buffer_write : std::false_type
    {};

    template<typename _Elem>
    struct element_has_buffer_write<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().write(
                std::declval<char*>(),
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // --- element std::to_string detection ---
    // note: djinterp::has_to_string detects a .to_string()
    // member.  This detects the free function
    // std::to_string(elem) for arithmetic types.

    template<typename _Elem, typename = void>
    struct element_has_std_to_string : std::false_type
    {};

    template<typename _Elem>
    struct element_has_std_to_string<_Elem,
        std::void_t<decltype(
            std::to_string(
                std::declval<const _Elem&>()))>>
        : std::true_type
    {};

    // --- container operator<< detection ---
    // detects a whole-container operator<< overload
    // (distinct from per-element streaming).

    template<typename _Type, typename = void>
    struct container_streamable_check : std::false_type
    {};

    template<typename _Type>
    struct container_streamable_check<_Type,
        std::void_t<decltype(
            std::declval<std::ostream&>()
                << std::declval<const _Type&>())>>
        : std::true_type
    {};

    // --- C-interop function pointer protocol ---
    // detects fn_print, fn_to_string, fn_write static or
    // member fields matching the C typedefs from djinterp.h.

    template<typename _Type, typename = void>
    struct has_fn_print_member : std::false_type
    {};

    template<typename _Type>
    struct has_fn_print_member<_Type,
        std::void_t<decltype(_Type::fn_print)>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_fn_to_string_member : std::false_type
    {};

    template<typename _Type>
    struct has_fn_to_string_member<_Type,
        std::void_t<decltype(_Type::fn_to_string)>>
        : std::true_type
    {};

    template<typename _Type, typename = void>
    struct has_fn_write_member : std::false_type
    {};

    template<typename _Type>
    struct has_fn_write_member<_Type,
        std::void_t<decltype(_Type::fn_write)>>
        : std::true_type
    {};

    // --- element std::format detection (C++20) ---

    template<typename _Elem, typename = void>
    struct element_formattable_check : std::false_type
    {};

#if defined(__cpp_lib_format)

    template<typename _Elem>
    struct element_formattable_check<_Elem,
        std::void_t<decltype(
            std::format("{}", std::declval<const _Elem&>())
        )>> : std::true_type
    {};

#endif  // __cpp_lib_format

NS_END  // internal


// =============================================================================
// III. Element Printability (via printer_traits)
// =============================================================================
// These traits extract the container's value_type and delegate
// printability queries to the djinterp::printer_traits system.

// has_printable_elements
//   type trait: true if the container's value_type satisfies
// djinterp::is_printable (string-like, arithmetic,
// has .to_string(), or supports operator<<).
template<typename _Type>
struct has_printable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_printable<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_printable_elements_v =
    has_printable_elements<_Type>::value;

// has_streamable_elements
//   type trait: true if the container's value_type can be
// written to std::ostream via operator<<.
// Delegates to djinterp::has_stream_insertion_for.
template<typename _Type>
struct has_streamable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::has_stream_insertion_for<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_streamable_elements_v =
    has_streamable_elements<_Type>::value;

// has_arithmetic_elements
//   type trait: true if the container's value_type is an
// arithmetic type printable via snprintf/std::to_string.
// Delegates to djinterp::is_arithmetic_printable.
template<typename _Type>
struct has_arithmetic_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_arithmetic_printable<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_arithmetic_elements_v =
    has_arithmetic_elements<_Type>::value;

// has_string_like_elements
//   type trait: true if the container's value_type is
// string-like (c_str, data+size, or C string).
// Delegates to djinterp::is_string_like.
template<typename _Type>
struct has_string_like_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::is_string_like<elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_string_like_elements_v =
    has_string_like_elements<_Type>::value;


// =============================================================================
// IV.  Element String Convertibility
// =============================================================================

// has_string_convertible_elements
//   type trait: true if the container's value_type can be
// converted to std::string via a .to_string() member
// (djinterp::has_to_string) or via std::to_string().
template<typename _Type>
struct has_string_convertible_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( djinterp::has_to_string<elem_type>::value ||
          internal::element_has_std_to_string<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool has_string_convertible_elements_v =
    has_string_convertible_elements<_Type>::value;

// element_to_string_uses_member
//   type trait: true if the element's to_string capability
// comes from a .to_string() member.  Useful for dispatch.
// Delegates to djinterp::has_to_string.
template<typename _Type>
struct element_to_string_uses_member
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        djinterp::has_to_string<elem_type>::value;
};

template<typename _Type>
inline constexpr bool element_to_string_uses_member_v =
    element_to_string_uses_member<_Type>::value;

// element_to_string_uses_std
//   type trait: true if the element's to_string capability
// comes from std::to_string (arithmetic types), and not
// from a .to_string() member.
template<typename _Type>
struct element_to_string_uses_std
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        ( !djinterp::has_to_string<elem_type>::value &&
          internal::element_has_std_to_string<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool element_to_string_uses_std_v =
    element_to_string_uses_std<_Type>::value;


// =============================================================================
// V.   Element Buffer Writability
// =============================================================================

// has_buffer_writable_elements
//   type trait: true if the container's value_type has a
// .write(char*, size_t) buffer-write member function.
template<typename _Type>
struct has_buffer_writable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        internal::element_has_buffer_write<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_buffer_writable_elements_v =
    has_buffer_writable_elements<_Type>::value;


// =============================================================================
// VI.  Element Format Support (C++20)
// =============================================================================

// has_formattable_elements
//   type trait: true if the container's value_type can be
// formatted via std::format("{}",elem).
// Always false on pre-C++20 compilers.
template<typename _Type>
struct has_formattable_elements
{
    using clean_type = clean_t<_Type>;
    using elem_type  =
        internal::safe_value_type_t<clean_type>;

    static constexpr bool value =
        internal::element_formattable_check<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_formattable_elements_v =
    has_formattable_elements<_Type>::value;


// =============================================================================
// VII. Container Streamability
// =============================================================================

// is_container_streamable
//   type trait: true if the container itself can be written
// to std::ostream via operator<<.  Detects a
// whole-container overload, not per-element streaming.
template<typename _Type>
struct is_container_streamable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::container_streamable_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_container_streamable_v =
    is_container_streamable<_Type>::value;

// is_container_to_string_capable
//   type trait: true if the container itself has a
// .to_string() member.
// Delegates to djinterp::has_to_string applied to the
// container type (not its elements).
template<typename _Type>
struct is_container_to_string_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::has_to_string<clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_container_to_string_capable_v =
    is_container_to_string_capable<_Type>::value;

// is_container_c_str_capable
//   type trait: true if the container itself has a .c_str()
// member (string-like containers).
// Delegates to djinterp::has_c_str.
template<typename _Type>
struct is_container_c_str_capable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::has_c_str<clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_container_c_str_capable_v =
    is_container_c_str_capable<_Type>::value;

// is_container_string_like
//   type trait: true if the container itself is string-like
// (c_str, data+size, or C-string).
// Delegates to djinterp::is_string_like.
template<typename _Type>
struct is_container_string_like
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        djinterp::is_string_like<clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_container_string_like_v =
    is_container_string_like<_Type>::value;


// =============================================================================
// VIII. C-Interop Protocol Detection
// =============================================================================
// The C side of djinterp uses function pointer fields
// (fn_print, fn_to_string, fn_write) as a vtable-like text
// output protocol.

// has_c_print_protocol
//   type trait: true if _Type exposes a fn_print static or
// member field.
template<typename _Type>
struct has_c_print_protocol
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_fn_print_member<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_c_print_protocol_v =
    has_c_print_protocol<_Type>::value;

// has_c_to_string_protocol
//   type trait: true if _Type exposes a fn_to_string static
// or member field.
template<typename _Type>
struct has_c_to_string_protocol
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_fn_to_string_member<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_c_to_string_protocol_v =
    has_c_to_string_protocol<_Type>::value;

// has_c_write_protocol
//   type trait: true if _Type exposes a fn_write static or
// member field.
template<typename _Type>
struct has_c_write_protocol
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::has_fn_write_member<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool has_c_write_protocol_v =
    has_c_write_protocol<_Type>::value;

// has_c_text_protocol
//   type trait: true if _Type exposes any of the C-interop
// text function pointer fields.
template<typename _Type>
struct has_c_text_protocol
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_c_print_protocol_v<clean_type>     ||
          has_c_to_string_protocol_v<clean_type> ||
          has_c_write_protocol_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_c_text_protocol_v =
    has_c_text_protocol<_Type>::value;


// =============================================================================
// IX.  Text Strategy Classification
// =============================================================================

// --- print strategy ---

// DPrintStrategy
//   enum: compile-time print strategy tags.
enum class DPrintStrategy
{
    // container has .print(ostream&) — use directly
    native_stream,

    // container has .print(FILE*) — use C file path
    native_file,

    // container has operator<< — use stream insertion
    container_stream,

    // container is iterable and elements are printable
    // (via printer_traits) — iterate and write each
    element_print,

    // C-interop fn_print field
    c_protocol,

    // no print path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct print_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DPrintStrategy value =
            has_print_method_v<clean_type>
                ? DPrintStrategy::native_stream

            : has_print_to_file_method_v<clean_type>
                ? DPrintStrategy::native_file

            : is_container_streamable_v<clean_type>
                ? DPrintStrategy::container_stream

            : ( is_iterable_container_v<clean_type> &&
                has_printable_elements_v<clean_type> )
                ? DPrintStrategy::element_print

            : has_c_print_protocol_v<clean_type>
                ? DPrintStrategy::c_protocol

            : DPrintStrategy::unsupported;
    };

NS_END  // internal

// container_print_strategy
//   type trait: determines the best print output path.
template<typename _Type>
struct container_print_strategy
{
    static constexpr DPrintStrategy value =
        internal::print_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DPrintStrategy
    container_print_strategy_v =
        container_print_strategy<_Type>::value;

// --- to_string strategy ---

// DToStringStrategy
//   enum: compile-time to_string strategy tags.
enum class DToStringStrategy
{
    // container has .to_string()
    native,

    // container is string-like (c_str/data+size)
    string_like,

    // container has operator<< — serialize via
    // ostringstream
    stream_serialize,

    // container is iterable with string-convertible
    // elements
    element_convert,

    // C-interop fn_to_string field
    c_protocol,

    // no to_string path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct to_string_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DToStringStrategy value =
            is_container_to_string_capable_v<
                clean_type>
                ? DToStringStrategy::native

            : is_container_string_like_v<clean_type>
                ? DToStringStrategy::string_like

            : is_container_streamable_v<clean_type>
                ? DToStringStrategy::stream_serialize

            : ( is_iterable_container_v<clean_type> &&
                has_string_convertible_elements_v<
                    clean_type> )
                ? DToStringStrategy::element_convert

            : has_c_to_string_protocol_v<clean_type>
                ? DToStringStrategy::c_protocol

            : DToStringStrategy::unsupported;
    };

NS_END  // internal

// container_to_string_strategy
//   type trait: determines the best to_string path.
template<typename _Type>
struct container_to_string_strategy
{
    static constexpr DToStringStrategy value =
        internal::to_string_strategy_impl<
            _Type>::value;
};

template<typename _Type>
inline constexpr DToStringStrategy
    container_to_string_strategy_v =
        container_to_string_strategy<_Type>::value;

// --- write strategy ---

// DWriteStrategy
//   enum: compile-time write strategy tags.
enum class DWriteStrategy
{
    // container has .write(char*, size_t)
    native,

    // container has .to_string() — convert then copy
    via_to_string,

    // container is string-like — copy data() directly
    via_string_like,

    // container is iterable with buffer-writable elements
    element_write,

    // C-interop fn_write field
    c_protocol,

    // no write path available
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct write_strategy_impl
    {
        using clean_type = clean_t<_Type>;

        static constexpr DWriteStrategy value =
            has_buffer_write_method_v<clean_type>
                ? DWriteStrategy::native

            : is_container_to_string_capable_v<
                  clean_type>
                ? DWriteStrategy::via_to_string

            : is_container_string_like_v<clean_type>
                ? DWriteStrategy::via_string_like

            : ( is_iterable_container_v<clean_type> &&
                has_buffer_writable_elements_v<
                    clean_type> )
                ? DWriteStrategy::element_write

            : has_c_write_protocol_v<clean_type>
                ? DWriteStrategy::c_protocol

            : DWriteStrategy::unsupported;
    };

NS_END  // internal

// container_write_strategy
//   type trait: determines the best buffer-write path.
template<typename _Type>
struct container_write_strategy
{
    static constexpr DWriteStrategy value =
        internal::write_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr DWriteStrategy
    container_write_strategy_v =
        container_write_strategy<_Type>::value;


// =============================================================================
// X.   Convenience Predicates
// =============================================================================

// is_printable_container
//   type trait: true if any print strategy is available.
template<typename _Type>
struct is_printable_container
{
    static constexpr bool value =
        ( container_print_strategy_v<_Type> !=
          DPrintStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_printable_container_v =
    is_printable_container<_Type>::value;

// is_string_convertible_container
//   type trait: true if any to_string strategy is available.
template<typename _Type>
struct is_string_convertible_container
{
    static constexpr bool value =
        ( container_to_string_strategy_v<_Type> !=
          DToStringStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_string_convertible_container_v =
    is_string_convertible_container<_Type>::value;

// is_buffer_writable_container
//   type trait: true if any write strategy is available.
template<typename _Type>
struct is_buffer_writable_container
{
    static constexpr bool value =
        ( container_write_strategy_v<_Type> !=
          DWriteStrategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_buffer_writable_container_v =
    is_buffer_writable_container<_Type>::value;

// has_any_text_support
//   type trait: true if the container supports at least one
// text output path (print, to_string, or write).
template<typename _Type>
struct has_any_text_support
{
    static constexpr bool value =
        ( is_printable_container_v<_Type>          ||
          is_string_convertible_container_v<_Type> ||
          is_buffer_writable_container_v<_Type> );
};

template<typename _Type>
inline constexpr bool has_any_text_support_v =
    has_any_text_support<_Type>::value;


// =============================================================================
// XI.  Combined Classification
// =============================================================================

// container_text_class
//   struct: complete text output classification of a
// container type.  All members are static constexpr.
template<typename _Type>
struct container_text_class
{
    // --- container-level capabilities ---
    static constexpr bool has_print =
        has_print_method_v<_Type>;
    static constexpr bool has_print_file =
        has_print_to_file_method_v<_Type>;
    static constexpr bool has_buffer_write =
        has_buffer_write_method_v<_Type>;
    static constexpr bool has_to_string =
        is_container_to_string_capable_v<_Type>;
    static constexpr bool has_c_str =
        is_container_c_str_capable_v<_Type>;
    static constexpr bool is_string_like =
        is_container_string_like_v<_Type>;
    static constexpr bool is_streamable =
        is_container_streamable_v<_Type>;

    // --- element-level capabilities ---
    // (delegated to printer_traits via value_type)
    static constexpr bool elements_printable =
        has_printable_elements_v<_Type>;
    static constexpr bool elements_streamable =
        has_streamable_elements_v<_Type>;
    static constexpr bool elements_arithmetic =
        has_arithmetic_elements_v<_Type>;
    static constexpr bool elements_string_like =
        has_string_like_elements_v<_Type>;
    static constexpr bool elements_to_string =
        has_string_convertible_elements_v<_Type>;
    static constexpr bool elements_buffer_writable =
        has_buffer_writable_elements_v<_Type>;
    static constexpr bool elements_formattable =
        has_formattable_elements_v<_Type>;
    static constexpr bool element_str_via_member =
        element_to_string_uses_member_v<_Type>;
    static constexpr bool element_str_via_std =
        element_to_string_uses_std_v<_Type>;

    // --- C-interop protocol ---
    static constexpr bool has_c_print =
        has_c_print_protocol_v<_Type>;
    static constexpr bool has_c_to_string =
        has_c_to_string_protocol_v<_Type>;
    static constexpr bool has_c_write =
        has_c_write_protocol_v<_Type>;
    static constexpr bool has_c_text =
        has_c_text_protocol_v<_Type>;

    // --- strategies ---
    static constexpr DPrintStrategy
        print_strategy =
            container_print_strategy_v<_Type>;
    static constexpr DToStringStrategy
        to_string_strategy =
            container_to_string_strategy_v<_Type>;
    static constexpr DWriteStrategy
        write_strategy =
            container_write_strategy_v<_Type>;

    // --- aggregate ---
    static constexpr bool is_printable =
        is_printable_container_v<_Type>;
    static constexpr bool is_string_convertible =
        is_string_convertible_container_v<_Type>;
    static constexpr bool is_buffer_writable =
        is_buffer_writable_container_v<_Type>;
    static constexpr bool has_text_support =
        has_any_text_support_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TEXT_TRAITS_
