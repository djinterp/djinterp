/******************************************************************************
* djinterp [core]                                          parser_traits.hpp
*
* Parser SFINAE detection traits:
*   This header provides a suite of compile-time structural traits for
* detecting whether a type conforms to the parser interface defined by
* the djinterp parsing framework.  Detection is purely structural —
* no tagging, no base-class checks, no RTTI.
*
* Traits provided:
*   - has_input_type<T>          does T expose `input_type`?
*   - has_result_type<T>         does T expose `result_type`?
*   - has_parse_method<T>        does T have a callable `parse`?
*   - has_do_parse_method<T>     does T have a callable `do_parse`?
*   - is_parser<T>              full structural parser check
*   - is_text_parser<T>         structural check for text parsers
*   - is_binary_parser<T>       structural check for binary parsers
*   - parsers_compatible<A,B>   do two parsers share input_type?
*   - parser_input_type<T>      extracts input_type (SFINAE-safe)
*   - parser_result_type<T>     extracts result_type (SFINAE-safe)
*
*
* path:      /inc/cpp/parse/parser_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.15
******************************************************************************/

#ifndef DJINTERP_PARSER_TRAITS_
#define DJINTERP_PARSER_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "./parse.hpp"


NS_DJINTERP
NS_PARSE
NS_TRAITS


// ================================================================
//  has_input_type
// ================================================================

NS_INTERNAL

    // has_input_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_input_type_helper : std::false_type
    {};
;

    // has_input_type_helper (success case)
    //   trait: succeeds when _Type::input_type is well-formed.
    template<typename _Type>
    struct has_input_type_helper<_Type,
        void_t<typename _Type::input_type>
    > : std::true_type
    {};
;

NS_END  // internal

// has_input_type
//   trait: detects whether _Type exposes a nested `input_type`
// typedef.
template<typename _Type>
struct has_input_type : internal::has_input_type_helper<_Type>
{};

// has_input_type_v
//   value: convenience alias for has_input_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_input_type_v = has_input_type<_Type>::value;
#endif


// ================================================================
//  has_result_type
// ================================================================

NS_INTERNAL

    // has_result_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_result_type_helper : std::false_type
    {};
;

    // has_result_type_helper (success case)
    //   trait: succeeds when _Type::result_type is well-formed.
    template<typename _Type>
    struct has_result_type_helper<_Type,
        void_t<typename _Type::result_type>
    > : std::true_type
    {};
;

NS_END  // internal

// has_result_type
//   trait: detects whether _Type exposes a nested `result_type`
// typedef.
template<typename _Type>
struct has_result_type : internal::has_result_type_helper<_Type>
{};

// has_result_type_v
//   value: convenience alias for has_result_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_result_type_v = has_result_type<_Type>::value;
#endif


// ================================================================
//  has_parse_method
// ================================================================

NS_INTERNAL

    // has_parse_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_parse_method_helper : std::false_type
    {};
;

    // has_parse_method_helper (success case — typed state)
    //   trait: succeeds when _Type has a `parse` callable that
    // accepts parse_state<_Type::input_type>&.
    template<typename _Type>
    struct has_parse_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().parse(
                std::declval<
                    parse_state<typename _Type::input_type>&
                >()
            )
        )>
    > : std::true_type
    {};
;

NS_END  // internal

// has_parse_method
//   trait: detects whether _Type has a `parse` member function
// accepting parse_state<input_type>&.
template<typename _Type>
struct has_parse_method : internal::has_parse_method_helper<_Type>
{};

// has_parse_method_v
//   value: convenience alias for has_parse_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_parse_method_v = has_parse_method<_Type>::value;
#endif


// ================================================================
//  has_do_parse_method
// ================================================================

NS_INTERNAL

    // has_do_parse_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_do_parse_method_helper : std::false_type
    {};
;

    // has_do_parse_method_helper (success case)
    //   trait: succeeds when _Type has a `do_parse` callable that
    // accepts parse_state<_Type::input_type>&.
    template<typename _Type>
    struct has_do_parse_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().do_parse(
                std::declval<
                    parse_state<typename _Type::input_type>&
                >()
            )
        )>
    > : std::true_type
    {};
;

NS_END  // internal

// has_do_parse_method
//   trait: detects whether _Type has a `do_parse` member function
// accepting parse_state<input_type>&.
template<typename _Type>
struct has_do_parse_method : internal::has_do_parse_method_helper<_Type>
{};

// has_do_parse_method_v
//   value: convenience alias for has_do_parse_method<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_do_parse_method_v =
        has_do_parse_method<_Type>::value;
#endif


// ================================================================
//  is_parser
// ================================================================

NS_INTERNAL

    // is_parser_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_parser_helper : std::false_type
    {};
;

    // is_parser_helper (success case)
    //   trait: succeeds when _Type structurally satisfies the
    // parser contract: it has input_type, result_type, and a
    // parse method whose return type is parse_result<result_type>.
    template<typename _Type>
    struct is_parser_helper<_Type,
        void_t<
            typename _Type::input_type,
            typename _Type::result_type,
            decltype(
                std::declval<_Type>().parse(
                    std::declval<
                        parse_state<typename _Type::input_type>&
                    >() ) )
        >>
    {
    private:
        using expected_return = parse_result<typename _Type::result_type>;

        using actual_return = decltype(
            std::declval<_Type>().parse(
                std::declval<
                    parse_state<typename _Type::input_type>&
                >()
            )
        );

    public:
        static constexpr bool value =
            std::is_same<actual_return, expected_return>::value;
    };

NS_END  // internal

// is_parser
//   trait: full structural check for parser conformance.
// Returns true when _Type exposes input_type, result_type, and a
// parse(parse_state<input_type>&) -> parse_result<result_type>.
template<typename _Type>
struct is_parser
    : std::integral_constant<bool,
                             internal::is_parser_helper<_Type>::value>
{};

// is_parser_v
//   value: convenience alias for is_parser<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_parser_v = is_parser<_Type>::value;
#endif


// ================================================================
//  is_text_parser
// ================================================================

NS_INTERNAL

    // is_text_parser_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             bool     _IsParser = is_parser<_Type>::value,
             typename = void>
    struct is_text_parser_helper : std::false_type
    {};
;

    // is_text_parser_helper (success case)
    //   trait: succeeds when _Type is a parser whose input_type
    // is `char`.
    template<typename _Type>
    struct is_text_parser_helper<_Type,
        true,
        typename std::enable_if<
            std::is_same<typename _Type::input_type, char>::value
        >::type
    > : std::true_type
    {};
;

NS_END  // internal

// is_text_parser
//   trait: detects whether _Type is a structurally conforming
// parser whose input_type is `char`.
template<typename _Type>
struct is_text_parser : internal::is_text_parser_helper<_Type>
{};

// is_text_parser_v
//   value: convenience alias for is_text_parser<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_text_parser_v = is_text_parser<_Type>::value;
#endif


// ================================================================
//  is_binary_parser
// ================================================================

NS_INTERNAL

    // is_binary_parser_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             bool     _IsParser = is_parser<_Type>::value,
             typename           = void>
    struct is_binary_parser_helper : std::false_type
    {};
;

    // is_binary_parser_helper (success case)
    //   trait: succeeds when _Type is a parser whose input_type
    // is `unsigned char`.
    template<typename _Type>
    struct is_binary_parser_helper<
        _Type,
        true,
        typename std::enable_if<
            std::is_same<
                typename _Type::input_type,
                unsigned char
            >::value
        >::type
    > : std::true_type
    {};
;

NS_END  // internal

// is_binary_parser
//   trait: detects whether _Type is a structurally conforming parser whose
// input_type is `unsigned char`.
template<typename _Type>
struct is_binary_parser : internal::is_binary_parser_helper<_Type>
{};

// is_binary_parser_v
//   value: convenience alias for is_binary_parser<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_binary_parser_v = is_binary_parser<_Type>::value;
#endif


// ================================================================
//  parsers_compatible
// ================================================================

NS_INTERNAL

    // parsers_compatible_helper
    //   trait: primary template (failure case).
    template<typename _A,
             typename _B,
             bool     _BothParsers = ( is_parser<_A>::value &&
                                       is_parser<_B>::value ),
             typename               = void>
    struct parsers_compatible_helper : std::false_type
    {};
;

    // parsers_compatible_helper (success case)
    //   trait: succeeds when both types are parsers and share the same
    // input_type.
    template<typename _A,
             typename _B>
    struct parsers_compatible_helper<
        _A,
        _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename _A::input_type,
                typename _B::input_type
            >::value
        >::type
    > : std::true_type
    {};
;

NS_END  // internal

// parsers_compatible
//   trait: detects whether two parser types share the same input_type and are
// therefore composable.
template<typename _A,
         typename _B>
struct parsers_compatible
    : internal::parsers_compatible_helper<_A, _B>
{};

// parsers_compatible_v
//   value: convenience alias for parsers_compatible<_A, _B>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    constexpr bool parsers_compatible_v =
        parsers_compatible<_A, _B>::value;
#endif


// ================================================================
//  parser_input_type  /  parser_result_type
// ================================================================
// SFINAE-safe type extractors.  Produce `void` when the queried
// type does not expose the expected member typedef.

NS_INTERNAL

    // parser_input_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct parser_input_type_helper
    {
        using type = void;
    };

    // parser_input_type_helper (success case)
    //   trait: extracts _Type::input_type when available.
    template<typename _Type>
    struct parser_input_type_helper<
        _Type,
        void_t<typename _Type::input_type>
    >
    {
        using type = typename _Type::input_type;
    };

    // parser_result_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct parser_result_type_helper
    {
        using type = void;
    };

    // parser_result_type_helper (success case)
    //   trait: extracts _Type::result_type when available.
    template<typename _Type>
    struct parser_result_type_helper<
        _Type,
        void_t<typename _Type::result_type>
    >
    {
        using type = typename _Type::result_type;
    };

NS_END  // internal

// parser_input_type
//   trait: SFINAE-safe extraction of a parser's input_type.
// Produces void if _Type does not expose input_type.
template<typename _Type>
struct parser_input_type : internal::parser_input_type_helper<_Type>
{};

// parser_input_type_t
//   type: convenience alias for parser_input_type<_Type>::type.
template<typename _Type>
using parser_input_type_t = typename parser_input_type<_Type>::type;

// parser_result_type
//   trait: SFINAE-safe extraction of a parser's result_type.
// Produces void if _Type does not expose result_type.
template<typename _Type>
struct parser_result_type : internal::parser_result_type_helper<_Type>
{};

// parser_result_type_t
//   type: convenience alias for parser_result_type<_Type>::type.
template<typename _Type>
using parser_result_type_t = typename parser_result_type<_Type>::type;


NS_END  // traits
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSER_TRAITS_
