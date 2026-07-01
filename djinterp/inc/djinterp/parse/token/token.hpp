/******************************************************************************
* djinterp [parse]                                              token/token.hpp
*
* Generic token primitives — token, traits, and concepts merged.
*   A token, in the formal-language sense, is a discriminated lexeme:
* a value drawn from a finite set of categories (the *kind*) paired
* with an associated payload (the *value*).  Tokens are produced by
* a scanner and consumed by a token-stream parser; this header is
* agnostic to both ends and to the underlying input domain.
*
*   In the language of ch-parsing.tex, the set of token kinds is one
* concrete instance of the terminal alphabet Σ from the grammar
* tuple G = (N, Σ, P, S).  A parser whose input_type is a token
* type therefore operates over a Σ stream of pre-classified
* lexemes, with the structural decomposition handled by a scanner
* upstream.
*
*   No positional information, source span, severity flag, or other
* metadata is imposed by this header — those layer on via domain-
* specific token derivatives (text tokens carrying line/column,
* binary tokens carrying byte offsets, etc.).
*
*   This header consolidates what were three separate files —
* token.hpp, token_traits.hpp, token_concepts.hpp — into one
* primary module.  The traits suite is purely structural (no
* tagging, no base-class checks, no RTTI) and the C++20 concepts
* mirror the traits exactly.
*
* CONTENTS
*   I.    token<KindType, ValueType>          the (kind, value) pair
*   II.   operator== / operator!=             value equality
*   III.  has_kind_type / has_value_type      member-typedef detectors
*                                             (from the shared core)
*   IV.   is_token<T>                         structural conformance
*   V.    tokens_compatible<A, B>             shared-kind compatibility
*   VI.   token_kind_type / token_value_type  SFINAE-safe extractors
*   VII.  C++20 concepts mirroring the traits
*
* path:      /inc/djinterp/parse/token/token.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_TOKEN_
#define DJINTERP_PARSE_TOKEN_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/meta/member_traits.hpp"
#include "../parse.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  I.   token
// ================================================================

// token
//   struct: a discriminated lexeme — the (kind, value) pair that
// constitutes the most abstract notion of a token.  The kind is
// drawn from a finite set of categories (the user-supplied
// _KindType); the value carries the associated payload.
//
//   Both type parameters are abstract: the struct imposes no
// requirements on input domain, source-stream representation, or
// metadata.  Tokens may be produced from text, binary, token
// streams, or any other source.
//
//   _KindType   should support equality comparison.  Common choices
//               include user-defined enums, small integer types,
//               and string-like types.
//
//   _ValueType  is unconstrained.  Use an empty struct (or any
//               cheaply-constructible placeholder) for tokens that
//               carry no payload.
template<typename _KindType,
         typename _ValueType>
struct token
{
    using kind_type  = _KindType;
    using value_type = _ValueType;

    kind_type   kind;
    value_type  value;

    token()
        : kind(),
          value()
    {}

    token(
        const kind_type&  _kind,
        const value_type& _value
    )
        : kind (_kind),
          value(_value)
    {}
};


// ================================================================
//  II.  operator==  /  operator!=
// ================================================================

// operator== (token, token)
//   function: two tokens are equal iff their kinds and values
// compare equal.  Requires both kind_type and value_type to
// support equality comparison.
template<typename _KindType,
         typename _ValueType>
inline bool
operator==(
    const token<_KindType, _ValueType>& _a,
    const token<_KindType, _ValueType>& _b
)
{
    return ( (_a.kind  == _b.kind ) &&
             (_a.value == _b.value) );
}

// operator!= (token, token)
//   function: negation of operator==.
template<typename _KindType,
         typename _ValueType>
inline bool
operator!=(
    const token<_KindType, _ValueType>& _a,
    const token<_KindType, _ValueType>& _b
)
{
    return (!(_a == _b));
}


// ================================================================
//  III. member-typedef detectors (shared core)
// ================================================================
//   has_kind_type and has_value_type are provided by
// core/meta/member_traits.hpp at djinterp:: scope and are visible
// here via the enclosing namespace.  No re-definition needed.


// ================================================================
//  IV.  is_token
// ================================================================

NS_INTERNAL

    // is_token_helper
    //   trait: primary template (failure case).
    template<typename _T,
             typename = void>
    struct is_token_helper : std::false_type
    {};

    // is_token_helper (success case)
    //   trait: succeeds when _T exposes both kind_type and
    // value_type typedefs.  The pairing of these two names —
    // neither common alone in non-token types — is taken as the
    // structural signature of a token.
    template<typename _T>
    struct is_token_helper<
        _T,
        void_t<typename clean_t<_T>::kind_type,
               typename clean_t<_T>::value_type>
    > : std::true_type
    {};

NS_END  // internal

// is_token
//   trait: full structural check for token conformance.
template<typename _T>
struct is_token : internal::is_token_helper<_T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_token_v = is_token<_T>::value;
#endif


// ================================================================
//  V.   tokens_compatible
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
    //   trait: both are tokens sharing kind_type — drawn from the
    // same alphabet of categories.
    template<typename _A,
             typename _B>
    struct tokens_compatible_helper<
        _A, _B,
        true,
        typename std::enable_if<
            std::is_same<
                typename clean_t<_A>::kind_type,
                typename clean_t<_B>::kind_type>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// tokens_compatible
//   trait: two token types share kind_type and are therefore drawn
// from the same alphabet of categories.
template<typename _A,
         typename _B>
struct tokens_compatible
    : internal::tokens_compatible_helper<_A, _B>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _A,
             typename _B>
    static constexpr bool tokens_compatible_v =
        tokens_compatible<_A, _B>::value;
#endif


// ================================================================
//  VI.  SFINAE-safe extractors
// ================================================================

// token_kind_type / token_kind_type_t
//   trait/type: SFINAE-safe extraction of a token's kind_type;
// yields `void` when absent.
D_DEFINE_MEMBER_TYPE_OR(token_kind_type, kind_type, void)

// token_value_type / token_value_type_t
//   trait/type: SFINAE-safe extraction of a token's value_type.
D_DEFINE_MEMBER_TYPE_OR(token_value_type, value_type, void)


// ================================================================
//  VII. C++20 concepts
// ================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // token_kind_typed
    //   concept: a type exposing kind_type.
    template<typename _T>
    concept token_kind_typed = has_kind_type<_T>::value;

    // token_value_typed
    //   concept: a type exposing value_type.
    template<typename _T>
    concept token_value_typed = has_value_type<_T>::value;

    // token_surface
    //   concept: a type exposing both nested types.
    template<typename _T>
    concept token_surface =
        ( has_kind_type<_T>::value &&
          has_value_type<_T>::value );

    // token_concept
    //   concept: structurally conforming token.
    template<typename _T>
    concept token_concept = is_token<_T>::value;

    // tokens_same_alphabet
    //   concept: a token pair drawn from the same kind alphabet.
    template<typename _A,
             typename _B>
    concept tokens_same_alphabet = tokens_compatible<_A, _B>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_TOKEN_
