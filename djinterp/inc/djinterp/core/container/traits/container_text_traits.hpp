/******************************************************************************
* djinterp [container]                             container_text_traits.hpp
*
* Text and stream traits for the djinterp container framework.
*   Detects whether a container can be converted to text or streamed
* to a buffer via the canonical method pair:
*
*   TEXT:    std::string to_text() const
*   STREAM: std::size_t stream_to(char* _buf,
*                                  std::size_t _cap) const
*
*   to_text() produces a complete string representation of the
* container in a single call (suitable for logging, display,
* serialization to text formats).
*
*   stream_to() writes as many bytes as fit into the provided buffer
* and returns the number of bytes written - an incremental / chunked
* interface suitable for fixed-size output buffers, network writes,
* or C interop via fn_write.
*
*   Additionally detects element-level text capabilities and
* standard library streamability (operator<<).
*
*
*
* path:      /inc/djinterp/core/container/traits/container_text_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      container-level detection
II.     element-level detection
III.    standard library stream detection
IV.     strategy classification
V.      convenience predicates
VI.     combined classification
*/

#ifndef DJINTERP_CONTAINER_TEXT_TRAITS_
#define DJINTERP_CONTAINER_TEXT_TRAITS_ 1

// std
#include <cstddef>
#include <iosfwd>
#include <string>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "container_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   Container-Level Detection
// ===========================================================================

// has_to_text_method
//   type trait: true if container has
//     std::string to_text() const
D_TYPE_TRAIT_TRUE(has_to_text_method,
                  decltype(std::declval<const _Type&>().to_text()))

// has_stream_to_method
//   type trait: true if container has
//     std::size_t stream_to(char*, std::size_t) const
D_TYPE_TRAIT_TRUE(has_stream_to_method,
                  decltype(std::declval<const _Type&>().stream_to(
                      std::declval<char*>(),
                      std::declval<std::size_t>())))

// has_to_string_method
//   type trait: true if container has a .to_string()
// method (common alternative to to_text).
D_TYPE_TRAIT_TRUE(has_to_string_method,
                  decltype(std::declval<const _Type&>().to_string()))


// ===========================================================================
// II.  Element-Level Detection
// ===========================================================================

NS_INTERNAL
    template<typename _Type, typename = void>
    struct text_safe_value_type
    {
        using type = void;
    };

    template<typename _Type>
    struct text_safe_value_type<_Type,
        std::void_t<typename _Type::value_type>>
    {
        using type = typename _Type::value_type;
    };

    template<typename _Type>
    using text_safe_value_type_t =
        typename text_safe_value_type<_Type>::type;

    // element has to_text()
    template<typename _Elem, typename = void>
    struct elem_has_to_text : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_to_text<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>()
                .to_text())>>
        : std::true_type
    {};

    // element has to_string()
    template<typename _Elem, typename = void>
    struct elem_has_to_string : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_to_string<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>()
                .to_string())>>
        : std::true_type
    {};

    // element has stream_to()
    template<typename _Elem, typename = void>
    struct elem_has_stream_to : std::false_type
    {};

    template<typename _Elem>
    struct elem_has_stream_to<_Elem,
        std::void_t<decltype(
            std::declval<const _Elem&>().stream_to(
                std::declval<char*>(),
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // element is arithmetic (std::to_string works)
    template<typename _Elem>
    struct elem_is_arithmetic
        : std::is_arithmetic<_Elem>
    {};

    // element is string-like
    template<typename _Elem>
    struct elem_is_string_like
        : std::is_same<_Elem, std::string>
    {};

NS_END  // internal

// has_text_convertible_elements
//   type trait: true if value_type can be converted to
// text via any mechanism.
template<typename _Type>
struct has_text_convertible_elements
{
    using elem_type = internal::text_safe_value_type_t<clean_t<_Type>>;

    static constexpr bool value =
        ( internal::elem_has_to_text<
              elem_type>::value          ||
          internal::elem_has_to_string<
              elem_type>::value          ||
          internal::elem_is_arithmetic<
              elem_type>::value          ||
          internal::elem_is_string_like<
              elem_type>::value );
};

template<typename _Type>
inline constexpr bool
    has_text_convertible_elements_v =
        has_text_convertible_elements<_Type>::value;

// has_streamable_elements
//   type trait: true if value_type has stream_to().
template<typename _Type>
struct has_streamable_elements
{
    using elem_type =
        internal::text_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        internal::elem_has_stream_to<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool has_streamable_elements_v =
    has_streamable_elements<_Type>::value;


// ===========================================================================
// III. Standard Library Stream Detection
// ===========================================================================

NS_INTERNAL

    // container has operator<<(ostream&, const C&)
    template<typename _Type, typename = void>
    struct is_ostream_insertable_check : std::false_type
    {};

    template<typename _Type>
    struct is_ostream_insertable_check<_Type,
        std::void_t<decltype(
            std::declval<std::ostream&>()
                << std::declval<const _Type&>())>>
        : std::true_type
    {};

    // element has operator<<
    template<typename _Elem, typename = void>
    struct elem_is_ostream_insertable : std::false_type
    {};

    template<typename _Elem>
    struct elem_is_ostream_insertable<_Elem,
        std::void_t<decltype(
            std::declval<std::ostream&>()
                << std::declval<const _Elem&>())>>
        : std::true_type
    {};

NS_END  // internal

// is_ostream_insertable
//   type trait: true if container supports operator<<.
template<typename _Type>
struct is_ostream_insertable
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        internal::is_ostream_insertable_check<
            clean_type>::value;
};

template<typename _Type>
inline constexpr bool is_ostream_insertable_v =
    is_ostream_insertable<_Type>::value;

// has_ostream_insertable_elements
//   type trait: true if value_type supports operator<<.
template<typename _Type>
struct has_ostream_insertable_elements
{
    using elem_type =
        internal::text_safe_value_type_t<
            clean_t<_Type>>;

    static constexpr bool value =
        internal::elem_is_ostream_insertable<
            elem_type>::value;
};

template<typename _Type>
inline constexpr bool
    has_ostream_insertable_elements_v =
        has_ostream_insertable_elements<
            _Type>::value;


// ===========================================================================
// IV.  Strategy Classification
// ===========================================================================

// --- text strategy ---

enum class container_text_strategy
{
    // container has to_text() - delegate
    native,

    // container has to_string() - delegate
    native_to_string,

    // container is ostream-insertable - use sstream
    ostream,

    // iterable + elements text-convertible - join
    element,

    // no text path
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct text_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr text_output_strategy value =
            has_to_text_method_v<C>
                ? container_text_strategy::native

            : has_to_string_method_v<C>
                ? container_text_strategy::native_to_string

            : is_ostream_insertable_v<C>
                ? container_text_strategy::ostream

            : ( is_iterable_container_v<C> &&
                has_text_convertible_elements_v<C> )
                ? container_text_strategy::element

            : container_text_strategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_text_strategy
{
    static constexpr container_text_strategy value =
        internal::text_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr container_text_strategy
    container_text_strategy_v =
        container_text_strategy<_Type>::value;

// --- stream strategy ---

enum class stream_strategy
{
    // container has stream_to(buf, cap) - delegate
    native,

    // iterable + elements have stream_to() -
    // per-element write
    element,

    // container has to_text() - convert then copy
    via_text,

    // no stream path
    unsupported
};

NS_INTERNAL

    template<typename _Type>
    struct stream_strategy_impl
    {
        using C = clean_t<_Type>;

        static constexpr stream_strategy value =
            has_stream_to_method_v<C>
                ? stream_strategy::native

            : ( is_iterable_container_v<C> &&
                has_streamable_elements_v<C> )
                ? stream_strategy::element

            : has_to_text_method_v<C>
                ? stream_strategy::via_text

            : stream_strategy::unsupported;
    };

NS_END  // internal

template<typename _Type>
struct container_stream_strategy
{
    static constexpr stream_strategy value =
        internal::stream_strategy_impl<_Type>::value;
};

template<typename _Type>
inline constexpr stream_strategy
    container_stream_strategy_v =
        container_stream_strategy<_Type>::value;


// ===========================================================================
// V.   Convenience Predicates
// ===========================================================================

template<typename _Type>
struct is_text_convertible
{
    static constexpr bool value =
        ( container_text_strategy_v<_Type> !=
          container_text_strategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_text_convertible_v =
    is_text_convertible<_Type>::value;

template<typename _Type>
struct is_streamable
{
    static constexpr bool value =
        ( container_stream_strategy_v<_Type> !=
          stream_strategy::unsupported );
};

template<typename _Type>
inline constexpr bool is_streamable_v =
    is_streamable<_Type>::value;


// ===========================================================================
// VI.  Combined Classification
// ===========================================================================

template<typename _Type>
struct container_text_class
{
    // container-level
    static constexpr bool has_to_text =
        has_to_text_method_v<_Type>;
    static constexpr bool has_stream_to =
        has_stream_to_method_v<_Type>;
    static constexpr bool has_to_string =
        has_to_string_method_v<_Type>;
    static constexpr bool ostream_insertable =
        is_ostream_insertable_v<_Type>;

    // element-level
    static constexpr bool elems_text_convertible =
        has_text_convertible_elements_v<_Type>;
    static constexpr bool elems_streamable =
        has_streamable_elements_v<_Type>;
    static constexpr bool elems_ostream_insertable =
        has_ostream_insertable_elements_v<_Type>;

    // strategies
    static constexpr container_text_strategy
        text_strategy =
            container_text_strategy_v<_Type>;
    static constexpr stream_strategy
        stream_strategy =
            container_stream_strategy_v<_Type>;

    // aggregate
    static constexpr bool is_text_convertible =
        is_text_convertible_v<_Type>;
    static constexpr bool is_streamable =
        is_streamable_v<_Type>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TEXT_TRAITS_