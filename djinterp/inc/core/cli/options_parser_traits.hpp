/******************************************************************************
* djinterp [containers]                                options_parser_traits.hpp
*
* Option-set parser SFINAE detection traits:
*   This header provides a suite of compile-time structural traits for
* detecting whether a type conforms to the option_set interface and for
* determining the minimum parser required to consume keys from a given
* option_set at parse time.
*
*   Detection is purely structural — no tagging, no base-class checks,
* no RTTI.  The traits divide into three tiers:
*
*   Tier 1 — Structural membership:
*     Individual probes for the member typedefs and methods that
*     characterize an option_set.
*       has_key_type<T>            does T expose `key_type`?
*       has_mapped_type<T>         does T expose `mapped_type`?
*       has_value_type<T>          does T expose `value_type`?
*       has_extent<T>              does T expose `extent`?
*       has_find_method<T>         does T have `find(key_type)`?
*       has_contains_method<T>     does T have `contains(key_type)`?
*       has_value_or_method<T>     does T have `value_or(key_type, ...)`?
*
*   Tier 2 — Composite conformance:
*     Full structural checks combining the individual probes.
*       is_option_set<T>           full option_set contract
*       is_static_option_set<T>    option_set with static extent
*       is_dynamic_option_set<T>   option_set with dynamic extent
*       is_sorted_option_set<T>    option_set whose extent != dyn & sorted
*
*   Tier 3 — Minimum parser classification:
*     Traits that inspect an option_set's key_type to determine the
*     simplest parser capable of producing keys for lookup.
*       option_key_class           enumeration of key parse classes
*       option_key_classify<T>     maps key_type to option_key_class
*       option_key_input_type<T>   deduces the parser input element type
*       option_key_is_text<T>      key is char-based text
*       option_key_is_integral<T>  key is an integral type
*       option_key_is_enum<T>      key is a scoped or unscoped enum
*       option_key_is_trivial<T>   key can be memcpy-parsed (POD)
*
*   Tier 4 — Type extractors (SFINAE-safe):
*       option_set_key_type<T>     extracts key_type (void fallback)
*       option_set_mapped_type<T>  extracts mapped_type (void fallback)
*
*
* TABLE OF CONTENTS
* =================
* I.    TIER 1 — STRUCTURAL MEMBERSHIP
*       1.  has_key_type
*       2.  has_mapped_type
*       3.  has_value_type
*       4.  has_extent
*       5.  has_find_method
*       6.  has_contains_method
*       7.  has_value_or_method
*
* II.   TIER 2 — COMPOSITE CONFORMANCE
*       1.  is_option_set
*       2.  is_static_option_set
*       3.  is_dynamic_option_set
*       4.  is_sorted_option_set
*
* III.  TIER 3 — MINIMUM PARSER CLASSIFICATION
*       1.  option_key_class         (enumeration)
*       2.  option_key_classify
*       3.  option_key_input_type
*       4.  option_key_is_text
*       5.  option_key_is_integral
*       6.  option_key_is_enum
*       7.  option_key_is_trivial
*
* IV.   TIER 4 — TYPE EXTRACTORS
*       1.  option_set_key_type
*       2.  option_set_mapped_type
*
*
* path:      /inc/containers/options_parser_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.XX.XX
******************************************************************************/

#ifndef DJINTERP_CONTAINERS_OPTIONS_PARSER_TRAITS_
#define DJINTERP_CONTAINERS_OPTIONS_PARSER_TRAITS_ 1

#include <cstddef>
#include <string>
#include <type_traits>
#include "../core/djinterp.hpp"
#include "./option_set.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif


NS_DJINTERP
NS_TRAITS


///////////////////////////////////////////////////////////////////////////////
///        I.    TIER 1 — STRUCTURAL MEMBERSHIP                             ///
///////////////////////////////////////////////////////////////////////////////


// ================================================================
//  has_key_type
// ================================================================

NS_INTERNAL

    // has_key_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_key_type_helper : std::false_type
    {};

    // has_key_type_helper (success case)
    //   trait: succeeds when _Type::key_type is well-formed.
    template<typename _Type>
    struct has_key_type_helper<_Type,
        void_t<typename _Type::key_type>
    > : std::true_type
    {};

NS_END  // internal

// has_key_type
//   trait: detects whether _Type exposes a nested `key_type`
// typedef.
template<typename _Type>
struct has_key_type : internal::has_key_type_helper<_Type>
{};

// has_key_type_v
//   value: convenience alias for has_key_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_key_type_v = has_key_type<_Type>::value;
#endif


// ================================================================
//  has_mapped_type
// ================================================================

NS_INTERNAL

    // has_mapped_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_mapped_type_helper : std::false_type
    {};

    // has_mapped_type_helper (success case)
    //   trait: succeeds when _Type::mapped_type is well-formed.
    template<typename _Type>
    struct has_mapped_type_helper<_Type,
        void_t<typename _Type::mapped_type>
    > : std::true_type
    {};

NS_END  // internal

// has_mapped_type
//   trait: detects whether _Type exposes a nested `mapped_type`
// typedef.
template<typename _Type>
struct has_mapped_type : internal::has_mapped_type_helper<_Type>
{};

// has_mapped_type_v
//   value: convenience alias for has_mapped_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mapped_type_v = has_mapped_type<_Type>::value;
#endif


// ================================================================
//  has_value_type
// ================================================================

NS_INTERNAL

    // has_value_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_value_type_helper : std::false_type
    {};

    // has_value_type_helper (success case)
    //   trait: succeeds when _Type::value_type is well-formed.
    template<typename _Type>
    struct has_value_type_helper<_Type,
        void_t<typename _Type::value_type>
    > : std::true_type
    {};

NS_END  // internal

// has_value_type
//   trait: detects whether _Type exposes a nested `value_type`
// typedef.
template<typename _Type>
struct has_value_type : internal::has_value_type_helper<_Type>
{};

// has_value_type_v
//   value: convenience alias for has_value_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_value_type_v = has_value_type<_Type>::value;
#endif


// ================================================================
//  has_extent
// ================================================================

NS_INTERNAL

    // has_extent_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_extent_helper : std::false_type
    {};

    // has_extent_helper (success case)
    //   trait: succeeds when _Type::extent is a valid static
    // constant convertible to std::size_t.
    template<typename _Type>
    struct has_extent_helper<_Type,
        void_t<decltype(_Type::extent)>
    > : std::is_convertible<decltype(_Type::extent), std::size_t>
    {};

NS_END  // internal

// has_extent
//   trait: detects whether _Type exposes a static `extent`
// constant convertible to std::size_t.
template<typename _Type>
struct has_extent : internal::has_extent_helper<_Type>
{};

// has_extent_v
//   value: convenience alias for has_extent<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_extent_v = has_extent<_Type>::value;
#endif


// ================================================================
//  has_find_method
// ================================================================

NS_INTERNAL

    // has_find_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_find_method_helper : std::false_type
    {};

    // has_find_method_helper (success case)
    //   trait: succeeds when _Type has a `find` callable that
    // accepts a const key_type& and returns a pointer to
    // value_type.
    template<typename _Type>
    struct has_find_method_helper<
        _Type,
        void_t<decltype(
            std::declval<const _Type>().find(
                std::declval<const typename _Type::key_type&>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_find_method
//   trait: detects whether _Type has a `find` member function
// accepting const key_type&.
template<typename _Type>
struct has_find_method : internal::has_find_method_helper<_Type>
{};

// has_find_method_v
//   value: convenience alias for has_find_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_find_method_v = has_find_method<_Type>::value;
#endif


// ================================================================
//  has_contains_method
// ================================================================

NS_INTERNAL

    // has_contains_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_contains_method_helper : std::false_type
    {};

    // has_contains_method_helper (success case)
    //   trait: succeeds when _Type has a `contains` callable that
    // accepts a const key_type& and returns bool.
    template<typename _Type>
    struct has_contains_method_helper<
        _Type,
        void_t<decltype(
            std::declval<const _Type>().contains(
                std::declval<const typename _Type::key_type&>()
            )
        )>
    > : std::is_convertible<
            decltype(
                std::declval<const _Type>().contains(
                    std::declval<const typename _Type::key_type&>()
                )
            ),
            bool
        >
    {};

NS_END  // internal

// has_contains_method
//   trait: detects whether _Type has a `contains` member
// function accepting const key_type& and returning bool.
template<typename _Type>
struct has_contains_method
    : internal::has_contains_method_helper<_Type>
{};

// has_contains_method_v
//   value: convenience alias for has_contains_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_contains_method_v =
        has_contains_method<_Type>::value;
#endif


// ================================================================
//  has_value_or_method
// ================================================================

NS_INTERNAL

    // has_value_or_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_value_or_method_helper : std::false_type
    {};

    // has_value_or_method_helper (success case)
    //   trait: succeeds when _Type has a `value_or` callable that
    // accepts (const key_type&, const mapped_type&).
    template<typename _Type>
    struct has_value_or_method_helper<
        _Type,
        void_t<decltype(
            std::declval<const _Type>().value_or(
                std::declval<const typename _Type::key_type&>(),
                std::declval<const typename _Type::mapped_type&>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_value_or_method
//   trait: detects whether _Type has a `value_or` member
// function accepting (const key_type&, const mapped_type&).
template<typename _Type>
struct has_value_or_method
    : internal::has_value_or_method_helper<_Type>
{};

// has_value_or_method_v
//   value: convenience alias for has_value_or_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_value_or_method_v =
        has_value_or_method<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///        II.   TIER 2 — COMPOSITE CONFORMANCE                             ///
///////////////////////////////////////////////////////////////////////////////


// ================================================================
//  is_option_set
// ================================================================

NS_INTERNAL

    // is_option_set_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_option_set_helper : std::false_type
    {};

    // is_option_set_helper (success case)
    //   trait: succeeds when _Type structurally satisfies the
    // option_set contract: it exposes key_type, mapped_type,
    // value_type, extent, find, and contains.
    template<typename _Type>
    struct is_option_set_helper<_Type,
        void_t<
            typename _Type::key_type,
            typename _Type::mapped_type,
            typename _Type::value_type,
            decltype(_Type::extent),
            decltype(
                std::declval<const _Type>().find(
                    std::declval<const typename _Type::key_type&>()
                )
            ),
            decltype(
                std::declval<const _Type>().contains(
                    std::declval<const typename _Type::key_type&>()
                )
            )
        >
    > : std::true_type
    {};

NS_END  // internal

// is_option_set
//   trait: full structural check for option_set conformance.
// Returns true when _Type exposes key_type, mapped_type,
// value_type, extent, find(key_type), and contains(key_type).
template<typename _Type>
struct is_option_set
    : std::integral_constant<bool,
                             internal::is_option_set_helper<_Type>::value>
{};

// is_option_set_v
//   value: convenience alias for is_option_set<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_option_set_v = is_option_set<_Type>::value;
#endif


// ================================================================
//  is_static_option_set
// ================================================================

NS_INTERNAL

    // is_static_option_set_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             bool     _IsOptionSet = is_option_set<_Type>::value,
             typename              = void>
    struct is_static_option_set_helper : std::false_type
    {};

    // is_static_option_set_helper (success case)
    //   trait: succeeds when _Type is an option_set whose extent
    // is not dynamic_extent.
    template<typename _Type>
    struct is_static_option_set_helper<_Type,
        true,
        typename std::enable_if<
            (_Type::extent != dynamic_extent)
        >::type
    > : std::true_type
    {};

NS_END  // internal

// is_static_option_set
//   trait: detects whether _Type is a structurally conforming
// option_set with a static (compile-time) extent.
template<typename _Type>
struct is_static_option_set
    : internal::is_static_option_set_helper<_Type>
{};

// is_static_option_set_v
//   value: convenience alias for is_static_option_set<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_static_option_set_v =
        is_static_option_set<_Type>::value;
#endif


// ================================================================
//  is_dynamic_option_set
// ================================================================

NS_INTERNAL

    // is_dynamic_option_set_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             bool     _IsOptionSet = is_option_set<_Type>::value,
             typename              = void>
    struct is_dynamic_option_set_helper : std::false_type
    {};

    // is_dynamic_option_set_helper (success case)
    //   trait: succeeds when _Type is an option_set whose extent
    // equals dynamic_extent.
    template<typename _Type>
    struct is_dynamic_option_set_helper<_Type,
        true,
        typename std::enable_if<
            (_Type::extent == dynamic_extent)
        >::type
    > : std::true_type
    {};

NS_END  // internal

// is_dynamic_option_set
//   trait: detects whether _Type is a structurally conforming
// option_set with dynamic (runtime) extent.
template<typename _Type>
struct is_dynamic_option_set
    : internal::is_dynamic_option_set_helper<_Type>
{};

// is_dynamic_option_set_v
//   value: convenience alias for is_dynamic_option_set<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_dynamic_option_set_v =
        is_dynamic_option_set<_Type>::value;
#endif


// ================================================================
//  is_sorted_option_set
// ================================================================

NS_INTERNAL

    // is_sorted_option_set_probe
    //   trait: primary template (not an option_set).
    template<typename _Type,
             typename = void>
    struct is_sorted_option_set_probe : std::false_type
    {};

    // is_sorted_option_set_probe (specialization)
    //   trait: direct pattern match on the option_set template
    // parameters, extracting the _Sorted flag.  This is the
    // only trait in this header that inspects the template
    // signature rather than using purely structural detection,
    // because the sorted policy is not exposed as a member
    // typedef or static constant on the public interface.
    template<typename    _Key,
             typename    _Value,
             std::size_t _N,
             bool        _Sorted,
             typename    _Allocator>
    struct is_sorted_option_set_probe<
        option_set<_Key, _Value, _N, _Sorted, _Allocator>,
        void
    > : std::integral_constant<bool, _Sorted>
    {};

NS_END  // internal

// is_sorted_option_set
//   trait: detects whether _Type is an option_set whose _Sorted
// template parameter is true.  Uses direct template pattern
// matching because the sorted policy is not part of the
// structural interface.
template<typename _Type>
struct is_sorted_option_set
    : internal::is_sorted_option_set_probe<
          typename std::remove_cv<_Type>::type
      >
{};

// is_sorted_option_set_v
//   value: convenience alias for is_sorted_option_set<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_sorted_option_set_v =
        is_sorted_option_set<_Type>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///        III.  TIER 3 — MINIMUM PARSER CLASSIFICATION                     ///
///////////////////////////////////////////////////////////////////////////////
// Given an option_set (or any type satisfying is_option_set),
// these traits inspect the key_type to determine what kind of
// parser is needed to produce keys from raw input.
//
// The classification hierarchy (from lightest to heaviest):
//
//   integral   — key is a built-in integral type.  The parser
//                need only read a fixed number of bytes and
//                reinterpret them as the key type.  Input
//                element: unsigned char (binary).
//
//   enum_key   — key is an enumeration.  Equivalent to integral
//                parsing on the underlying type, plus a cast.
//                Input element: unsigned char (binary).
//
//   trivial    — key is a trivially-copyable non-integral, non-
//                enum type (e.g. a small POD struct used as a
//                key).  Requires memcpy-style binary parsing.
//                Input element: unsigned char (binary).
//
//   text       — key is a character-based string type
//                (std::string, std::string_view, const char*).
//                Requires a text parser that accumulates
//                characters until a delimiter.
//                Input element: char.
//
//   complex    — key is none of the above (e.g. a non-trivial
//                class with custom comparison).  No automatic
//                parser can be inferred; the user must supply a
//                custom key parser.
//                Input element: void (indeterminate).


// ================================================================
//  option_key_class
// ================================================================

// option_key_class
//   enum: classifies the parse complexity of an option_set's
// key_type.  Values are ordered from lightest (integral) to
// heaviest (complex).
enum class option_key_class : int
{
    integral = 0,
    enum_key = 1,
    trivial  = 2,
    text     = 3,
    complex  = 4
};


// ================================================================
//  option_key_is_text  (internal helpers)
// ================================================================

NS_INTERNAL

    // is_text_key
    //   trait: primary template (not a text key).
    template<typename _Key,
             typename = void>
    struct is_text_key : std::false_type
    {};

    // is_text_key<std::string>
    //   trait: std::string is a text key.
    template<>
    struct is_text_key<std::string, void> : std::true_type
    {};

    // is_text_key<const char*>
    //   trait: const char* is a text key.
    template<>
    struct is_text_key<const char*, void> : std::true_type
    {};

    // is_text_key<char*>
    //   trait: char* is a text key.
    template<>
    struct is_text_key<char*, void> : std::true_type
    {};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // is_text_key<std::string_view>
    //   trait: std::string_view is a text key.
    template<>
    struct is_text_key<std::string_view, void> : std::true_type
    {};
#endif

NS_END  // internal


// ================================================================
//  option_key_classify
// ================================================================

NS_INTERNAL

    // option_key_classify_helper
    //   trait: primary template — assumes complex.
    template<typename _Key,
             bool     _IsIntegral = std::is_integral<_Key>::value,
             bool     _IsEnum     = std::is_enum<_Key>::value,
             bool     _IsText     = is_text_key<_Key>::value,
             bool     _IsTrivial  = std::is_trivially_copyable<_Key>::value>
    struct option_key_classify_helper
    {
        static constexpr option_key_class value =
            option_key_class::complex;
    };

    // option_key_classify_helper (integral)
    //   trait: integral keys are the lightest parse class.
    template<typename _Key,
             bool     _IsEnum,
             bool     _IsText,
             bool     _IsTrivial>
    struct option_key_classify_helper<_Key,
                                     true,
                                     _IsEnum,
                                     _IsText,
                                     _IsTrivial>
    {
        static constexpr option_key_class value =
            option_key_class::integral;
    };

    // option_key_classify_helper (enum)
    //   trait: enum keys parse like their underlying integral
    // type plus a cast.
    template<typename _Key,
             bool     _IsText,
             bool     _IsTrivial>
    struct option_key_classify_helper<_Key,
                                     false,
                                     true,
                                     _IsText,
                                     _IsTrivial>
    {
        static constexpr option_key_class value =
            option_key_class::enum_key;
    };

    // option_key_classify_helper (text)
    //   trait: text keys require character-level parsing.
    template<typename _Key,
             bool     _IsTrivial>
    struct option_key_classify_helper<_Key,
                                     false,
                                     false,
                                     true,
                                     _IsTrivial>
    {
        static constexpr option_key_class value =
            option_key_class::text;
    };

    // option_key_classify_helper (trivial, non-text)
    //   trait: trivially-copyable non-integral, non-enum,
    // non-text keys can be memcpy-parsed from binary input.
    template<typename _Key>
    struct option_key_classify_helper<_Key,
                                     false,
                                     false,
                                     false,
                                     true>
    {
        static constexpr option_key_class value =
            option_key_class::trivial;
    };

NS_END  // internal

// option_key_classify
//   trait: maps a key type to its option_key_class, indicating
// the minimum parser complexity required to produce keys of
// that type from raw input.
template<typename _Key>
struct option_key_classify
{
    static constexpr option_key_class value =
        internal::option_key_classify_helper<
            typename std::remove_cv<_Key>::type
        >::value;
};

// option_key_classify_v
//   value: convenience alias for option_key_classify<_Key>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Key>
    constexpr option_key_class option_key_classify_v =
        option_key_classify<_Key>::value;
#endif


// ================================================================
//  option_key_input_type
// ================================================================

NS_INTERNAL

    // option_key_input_type_helper
    //   trait: maps an option_key_class to the corresponding
    // parser input element type.  Text keys parse from char;
    // binary keys parse from unsigned char; complex keys
    // produce void (no automatic parser).
    template<option_key_class _Class>
    struct option_key_input_type_helper
    {
        using type = void;
    };

    // option_key_input_type_helper<integral>
    //   trait: integral keys are parsed from binary input.
    template<>
    struct option_key_input_type_helper<option_key_class::integral>
    {
        using type = unsigned char;
    };

    // option_key_input_type_helper<enum_key>
    //   trait: enum keys are parsed from binary input.
    template<>
    struct option_key_input_type_helper<option_key_class::enum_key>
    {
        using type = unsigned char;
    };

    // option_key_input_type_helper<trivial>
    //   trait: trivial keys are parsed from binary input.
    template<>
    struct option_key_input_type_helper<option_key_class::trivial>
    {
        using type = unsigned char;
    };

    // option_key_input_type_helper<text>
    //   trait: text keys are parsed from character input.
    template<>
    struct option_key_input_type_helper<option_key_class::text>
    {
        using type = char;
    };

NS_END  // internal

// option_key_input_type
//   trait: deduces the parser input element type required to
// parse keys of type _Key.  Produces `char` for text keys,
// `unsigned char` for binary-parseable keys, and `void` for
// complex keys that require a user-supplied parser.
template<typename _Key>
struct option_key_input_type
{
    using type = typename internal::option_key_input_type_helper<
        option_key_classify<_Key>::value
    >::type;
};

// option_key_input_type_t
//   type: convenience alias for option_key_input_type<_Key>::type.
template<typename _Key>
using option_key_input_type_t =
    typename option_key_input_type<_Key>::type;


// ================================================================
//  option_key_is_text
// ================================================================

// option_key_is_text
//   trait: true when _Key classifies as a text key (string,
// string_view, const char*, char*).
template<typename _Key>
struct option_key_is_text
    : std::integral_constant<bool,
          (option_key_classify<_Key>::value ==
           option_key_class::text)
      >
{};

// option_key_is_text_v
//   value: convenience alias for option_key_is_text<_Key>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Key>
    constexpr bool option_key_is_text_v =
        option_key_is_text<_Key>::value;
#endif


// ================================================================
//  option_key_is_integral
// ================================================================

// option_key_is_integral
//   trait: true when _Key classifies as an integral key.
template<typename _Key>
struct option_key_is_integral
    : std::integral_constant<bool,
          (option_key_classify<_Key>::value ==
           option_key_class::integral)
      >
{};

// option_key_is_integral_v
//   value: convenience alias for option_key_is_integral<_Key>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Key>
    constexpr bool option_key_is_integral_v =
        option_key_is_integral<_Key>::value;
#endif


// ================================================================
//  option_key_is_enum
// ================================================================

// option_key_is_enum
//   trait: true when _Key classifies as an enumeration key.
template<typename _Key>
struct option_key_is_enum
    : std::integral_constant<bool,
          (option_key_classify<_Key>::value ==
           option_key_class::enum_key)
      >
{};

// option_key_is_enum_v
//   value: convenience alias for option_key_is_enum<_Key>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Key>
    constexpr bool option_key_is_enum_v =
        option_key_is_enum<_Key>::value;
#endif


// ================================================================
//  option_key_is_trivial
// ================================================================

// option_key_is_trivial
//   trait: true when _Key classifies as a trivially-copyable
// non-integral, non-enum, non-text key.
template<typename _Key>
struct option_key_is_trivial
    : std::integral_constant<bool,
          (option_key_classify<_Key>::value ==
           option_key_class::trivial)
      >
{};

// option_key_is_trivial_v
//   value: convenience alias for option_key_is_trivial<_Key>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Key>
    constexpr bool option_key_is_trivial_v =
        option_key_is_trivial<_Key>::value;
#endif


///////////////////////////////////////////////////////////////////////////////
///        IV.   TIER 4 — TYPE EXTRACTORS                                   ///
///////////////////////////////////////////////////////////////////////////////
// SFINAE-safe type extractors.  Produce `void` when the queried
// type does not expose the expected member typedef.


// ================================================================
//  option_set_key_type
// ================================================================

NS_INTERNAL

    // option_set_key_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct option_set_key_type_helper
    {
        using type = void;
    };

    // option_set_key_type_helper (success case)
    //   trait: extracts _Type::key_type when available.
    template<typename _Type>
    struct option_set_key_type_helper<
        _Type,
        void_t<typename _Type::key_type>
    >
    {
        using type = typename _Type::key_type;
    };

NS_END  // internal

// option_set_key_type
//   trait: SFINAE-safe extraction of an option_set's key_type.
// Produces void if _Type does not expose key_type.
template<typename _Type>
struct option_set_key_type
    : internal::option_set_key_type_helper<_Type>
{};

// option_set_key_type_t
//   type: convenience alias for option_set_key_type<_Type>::type.
template<typename _Type>
using option_set_key_type_t =
    typename option_set_key_type<_Type>::type;


// ================================================================
//  option_set_mapped_type
// ================================================================

NS_INTERNAL

    // option_set_mapped_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct option_set_mapped_type_helper
    {
        using type = void;
    };

    // option_set_mapped_type_helper (success case)
    //   trait: extracts _Type::mapped_type when available.
    template<typename _Type>
    struct option_set_mapped_type_helper<
        _Type,
        void_t<typename _Type::mapped_type>
    >
    {
        using type = typename _Type::mapped_type;
    };

NS_END  // internal

// option_set_mapped_type
//   trait: SFINAE-safe extraction of an option_set's
// mapped_type.  Produces void if _Type does not expose
// mapped_type.
template<typename _Type>
struct option_set_mapped_type
    : internal::option_set_mapped_type_helper<_Type>
{};

// option_set_mapped_type_t
//   type: convenience alias for
// option_set_mapped_type<_Type>::type.
template<typename _Type>
using option_set_mapped_type_t =
    typename option_set_mapped_type<_Type>::type;


NS_END  // traits
NS_END  // djinterp


#endif  // DJINTERP_CONTAINERS_OPTIONS_PARSER_TRAITS_
