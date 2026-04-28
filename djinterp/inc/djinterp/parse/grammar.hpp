/******************************************************************************
* djinterp [core]                                                 grammar.hpp
*
* Generic formal-grammar primitives:
*   This header defines the abstract notion of a formal grammar within
* the djinterp parsing framework.  In the language-theoretic sense, a
* grammar is the four-tuple
*
*     G = (N, Σ, P, S)
*
*   where:
*
*     N  is the finite set of nonterminal symbols,
*     Σ  is the finite set of terminal symbols (the alphabet),
*     P  is the finite set of production rules, and
*     S  ∈ N is the start symbol.
*
*   The header is intentionally agnostic to the underlying input
* domain: the four set parameters are abstract types and may be drawn
* from any alphabet — text, binary, token streams, or arbitrary
* user-defined symbol tags.  No constraints are imposed on the user's
* choice of typelist representation.  The only piece that depends on
* the shape of `_Nonterminals` is the optional implied-start-symbol
* fallback, which extracts the first element when the set is
* tuple-shaped; users with alternative representations may always
* supply the start symbol explicitly.
*
*   The header provides:
*
*     - production<_LHS, _RHS...>
*         A single context-free production rule of the form
*         LHS → RHS_1 RHS_2 … RHS_n.  An empty parameter pack
*         models an ε-production.
*
*     - grammar<_Nonterminals, _Terminals, _Productions, _StartSymbol>
*         The four-tuple itself.  The start-symbol parameter is
*         optional: when omitted (or supplied as `void`) the grammar
*         attempts to derive it from the first element of the
*         nonterminal set when that set is tuple-shaped.
*
*   Structural detection traits live in the companion header
* `grammar_traits.hpp`, which is included here so that consumers of
* `grammar.hpp` automatically receive the trait suite.
*
*   No semantic actions, attribute schemes, or parser-construction
* artefacts are included — those belong to derivative modules
* layered on top of this abstract definition.
*
*
* path:      /inc/cpp/parse/grammar.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_GRAMMAR_
#define DJINTERP_GRAMMAR_ 1

#include <cstddef>
#include <tuple>
#include <type_traits>
#include "../core/djinterp.hpp"
#include "./parse.hpp"
#include "./grammar_traits.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  production
// ================================================================

// production
//   struct: a single context-free production rule of the form
// LHS → RHS_1 RHS_2 … RHS_n, where _LHS is a nonterminal symbol
// type and the _RHS pack is a (possibly empty) sequence of
// symbol types drawn from N ∪ Σ.  An empty _RHS pack models an
// ε-production (LHS → ε).
//
//   Symbol types are unconstrained — they may be tag structs,
// enum-class values lifted to types via std::integral_constant,
// or any other distinguishable type the user prefers.  The
// production is therefore agnostic to the input domain in which
// its symbols ultimately reside.
template<typename    _LHS,
         typename... _RHS>
struct production
{
    using lhs = _LHS;
    using rhs = std::tuple<_RHS...>;

    // arity
    //   value: the length of the RHS string.  Zero indicates an
    // ε-production.
    D_STATIC_CONSTEXPR std::size_t arity = sizeof...(_RHS);
};


// ================================================================
//  grammar
// ================================================================

NS_INTERNAL

    // start_symbol_resolver
    //   trait: primary template — when an explicit start symbol
    // is supplied (i.e. _Explicit is not void), use it directly.
    //   This helper is internal machinery for the grammar
    // template; it is not exposed as a public trait.
    template<typename _Nonterminals,
             typename _Explicit,
             typename = void>
    struct start_symbol_resolver
    {
        using type = _Explicit;
    };

    // start_symbol_resolver (implied case)
    //   trait: when no explicit start symbol is supplied
    // (_Explicit == void) and _Nonterminals is tuple-shaped,
    // the start symbol defaults to the first nonterminal —
    // a common and harmless convention.  Users with
    // non-tuple typelist representations must supply the
    // start symbol explicitly.
    template<typename _Nonterminals>
    struct start_symbol_resolver<
        _Nonterminals,
        void,
        void_t<typename std::tuple_element<0, _Nonterminals>::type>
    >
    {
        using type =
            typename std::tuple_element<0, _Nonterminals>::type;
    };

NS_END  // internal

// grammar
//   struct: a formal grammar G = (N, Σ, P, S).
//
//   _Nonterminals  represents N, the finite set of nonterminal
//                  symbol types.
//   _Terminals     represents Σ, the finite set of terminal
//                  symbol types (the alphabet over which the
//                  language is defined).
//   _Productions   represents P, the finite set of production
//                  rules — typically a typelist of `production`
//                  instantiations.
//   _StartSymbol   represents S ∈ N.  May be omitted (or supplied
//                  as `void`); when omitted, the start symbol
//                  defaults to the first element of
//                  _Nonterminals when that set is tuple-shaped.
//                  Users with alternative typelist
//                  representations must supply the start symbol
//                  explicitly.
//
//   The four set parameters are stored as type aliases without
// further interpretation; the grammar imposes no additional
// structural requirements on the user's choice of set
// representation.  The terminal and nonterminal alphabets are
// thus fully agnostic to the underlying input domain.
template<typename _Nonterminals,
         typename _Terminals,
         typename _Productions,
         typename _StartSymbol = void>
struct grammar
{
    using nonterminals = _Nonterminals;
    using terminals    = _Terminals;
    using productions  = _Productions;
    using start_symbol = typename internal::start_symbol_resolver<
        _Nonterminals,
        _StartSymbol
    >::type;
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_GRAMMAR_
