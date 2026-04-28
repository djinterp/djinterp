/******************************************************************************
* djinterp [core]                                                   token.hpp
*
* Generic token primitives:
*   This header defines the abstract notion of a token within the djinterp
* parsing framework.  A token, in the formal-language sense, is a
* discriminated lexeme — a value drawn from a finite set of categories
* (the *kind*) paired with an associated payload (the *value*).
*
*   The header is intentionally agnostic to the underlying input domain:
* tokens are not tied to text, binary, or any other input modality.
* No further structure is imposed — positional information, source
* spans, severity flags, and other metadata are layered on by
* domain-specific derivatives (text, binary, AST, etc.).
*
*   Both the kind and value types are abstract template parameters.
* Common choices for `_KindType` include user-defined enums, small
* integers, or tag types; the only requirement is that the type be
* useful as a discriminator (typically equality-comparable).
* `_ValueType` may be any payload — matched text, a parsed numeric,
* an AST sub-node, or an empty marker for tokens that carry no data.
*
*   Structural detection traits live in the companion header
* `token_traits.hpp`, which is included here so that consumers of
* `token.hpp` automatically receive the trait suite.
*
*
* path:      /inc/cpp/parse/token.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_TOKEN_
#define DJINTERP_TOKEN_ 1

#include <cstddef>
#include <type_traits>
#include "../core/djinterp.hpp"
#include "./parse.hpp"
#include "./token_traits.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  token
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
//   _KindType  should support equality comparison.  Common choices
//              include user-defined enums, small integer types,
//              and string-like types.
//
//   _ValueType is unconstrained.  Use an empty struct (or any
//              cheaply-constructible placeholder) for tokens
//              that carry no payload.
template<typename _KindType,
         typename _ValueType>
struct token
{
    using kind_type  = _KindType;
    using value_type = _ValueType;

    kind_type   kind;
    value_type  value;

    token()
        : kind  ()
        , value ()
    {}

    token(const kind_type&  _kind,
          const value_type& _value)
        : kind  (_kind)
        , value (_value)
    {}
};

// operator== (token, token)
//   function: two tokens are equal iff their kinds and values
// compare equal.  Requires both kind_type and value_type to
// support equality comparison.
template<typename _KindType,
         typename _ValueType>
bool operator==(const token<_KindType, _ValueType>& _a,
                const token<_KindType, _ValueType>& _b)
{
    return ( (_a.kind  == _b.kind) &&
             (_a.value == _b.value) );
}

// operator!= (token, token)
//   function: negation of operator==.
template<typename _KindType,
         typename _ValueType>
bool operator!=(const token<_KindType, _ValueType>& _a,
                const token<_KindType, _ValueType>& _b)
{
    return (!(_a == _b));
}


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_TOKEN_
