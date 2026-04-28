/******************************************************************************
* djinterp [core]                                            token_traits.hpp
*
* Token SFINAE detection traits:
*   This header provides a suite of compile-time structural traits for
* detecting whether a type conforms to the token interface defined by
* the djinterp parsing framework.  Detection is purely structural —
* no tagging, no base-class checks, no RTTI — and is intentionally
* agnostic to the underlying input domain (text, binary, or otherwise).
* Any type exposing the documented typedefs satisfies the contract,
* regardless of its provenance.
*
* Traits provided:
*   - has_kind_type<T>          does T expose `kind_type`?
*   - has_value_type<T>         does T expose `value_type`?
*   - is_token<T>               full structural token check
*   - tokens_compatible<A, B>   do two tokens share kind_type?
*   - token_kind_type<T>        extracts kind_type (SFINAE-safe)
*   - token_value_type<T>       extracts value_type (SFINAE-safe)
*
*
* path:      /inc/cpp/parse/token_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_TOKEN_TRAITS_
#define DJINTERP_TOKEN_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "./parse.hpp"


NS_DJINTERP
NS_PARSE
NS_TRAITS


// ================================================================
//  has_kind_type
// ================================================================

NS_INTERNAL

    // has_kind_type_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_kind_type_helper : std::false_type
    {};

    // has_kind_type_helper (success case)
    //   trait: succeeds when _Type::kind_type is well-formed.
    template<typename _Type>
    struct has_kind_type_helper<_Type,
        void_t<typename _Type::kind_type>
    > : std::true_type
    {};

NS_END  // internal

// has_kind_type
//   trait: detects whether _Type exposes a nested `kind_type`
// typedef.
template<typename _Type>
struct has_kind_type : internal::has_kind_type_helper<_Type>
{};

// has_kind_type_v
//   value: convenience alias for has_kind_type<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_kind_type_v = has_kind_type<_Type>::value;
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
//  is_token
// ================================================================

NS_INTERNAL

    // is_token_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_token_helper : std::false_type
    {};

    // is_token_helper (success case)
    //   trait: succeeds when _Type structurally satisfies the
    // token contract: it exposes both kind_type and value_type
    // typedefs.  The pairing of these two names — neither
    // common alone in non-token types — is taken as a sufficient
    // structural signature for a token.
    template<typename _Type>
    struct is_token_helper<_Type,
        void_t<
            typename _Type::kind_type,
            typename _Type::value_type
        >
    > : std::true_type
    {};

NS_END  // internal

// is_token
//   trait: full structural check for token conformance.
// Returns true when _Type exposes both kind_type and value_type
// nested typedefs.
template<typename _Type>
struct is_token : internal::is_token_helper<_Type>
{};

// is_token_v
//   value: convenience alias for is_token<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_token_v = is_token<_Type>::value;
#endif


// ================================================================
//  tokens_compatible
// ================================================================

NS_INTERNAL

    // tokens_compatible_helper
    //   trait: primary template (failure case).
    template<typename _A,
             typename _B,
             bool     _BothTokens = ( is_token<_A>::value &&
                                      is_token<_B>::value ),
             typename             = void>
    struct tokens_compatible_helper : std::false_type
    {};

    // tokens_compatible_helper (success case)
    //   trait: succeeds when both types are tokens and share the
    // same kind_type.
    template<typename _A,
             typename _B>
    struct tokens_compatible_helper<
        _A,
        _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename _A::kind_type,
                typename _B::kind_type
            >::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// tokens_compatible
//   trait: detects whether two token types share the same
// kind_type and are therefore drawn from the same alphabet of
// categories.
template<typename _A,
         typename _B>
struct tokens_compatible
    : internal::tokens_compatible_helper<_A, _B>
{};

// tokens_compatible_v
//   value: convenience alias for tokens_compatible<_A, _B>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    constexpr bool tokens_compatible_v =
        tokens_compatible<_A, _B>::value;
#endif


// ================================================================
//  token_kind_type  /  token_value_type
// ================================================================
// SFINAE-safe type extractors.  Produce `void` when the queried
// type does not expose the expected member typedef.

NS_INTERNAL

    // token_kind_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct token_kind_type_helper
    {
        using type = void;
    };

    // token_kind_type_helper (success case)
    //   trait: extracts _Type::kind_type when available.
    template<typename _Type>
    struct token_kind_type_helper<
        _Type,
        void_t<typename _Type::kind_type>
    >
    {
        using type = typename _Type::kind_type;
    };

    // token_value_type_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct token_value_type_helper
    {
        using type = void;
    };

    // token_value_type_helper (success case)
    //   trait: extracts _Type::value_type when available.
    template<typename _Type>
    struct token_value_type_helper<
        _Type,
        void_t<typename _Type::value_type>
    >
    {
        using type = typename _Type::value_type;
    };

NS_END  // internal

// token_kind_type
//   trait: SFINAE-safe extraction of a token's kind_type.
// Produces void if _Type does not expose kind_type.
template<typename _Type>
struct token_kind_type : internal::token_kind_type_helper<_Type>
{};

// token_kind_type_t
//   type: convenience alias for token_kind_type<_Type>::type.
template<typename _Type>
using token_kind_type_t = typename token_kind_type<_Type>::type;

// token_value_type
//   trait: SFINAE-safe extraction of a token's value_type.
// Produces void if _Type does not expose value_type.
template<typename _Type>
struct token_value_type : internal::token_value_type_helper<_Type>
{};

// token_value_type_t
//   type: convenience alias for token_value_type<_Type>::type.
template<typename _Type>
using token_value_type_t = typename token_value_type<_Type>::type;


NS_END  // traits
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_TOKEN_TRAITS_
