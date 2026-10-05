/*******************************************************************************
* djinterp [parse]                                                   grammar.hpp
*
* Formal grammar four-tuple G = (N, Σ, P, S).
*   This header carries the *textual* presentation of a grammar —
* the four-tuple the parser literature reads directly: nonterminals,
* terminals, productions, and a start symbol.  The polynomial
* functor F that this grammar is the sum-of-products presentation of
* now lives in its own file, grammar/polynomial.hpp (poly_var,
* poly_unit, poly_const, poly_sum, poly_product, their Functor /
* Traversable instances, and the type-level shapes), so that F can
* be folded generically by functional/recursion.hpp's cata.  This
* file includes polynomial.hpp, so a consumer of grammar.hpp still
* sees the whole vocabulary; the split is by responsibility, not by
* dependency.
*
*   Grammar tuple and polynomial functor are two faces of the same
* object: a production is one variant of F at its LHS nonterminal,
* productions sharing an LHS are summed into F's sum there, an RHS
* string is the product of that variant's children, and a recursive
* nonterminal is F's μ.  The tuple side is presentation; the functor
* side (polynomial.hpp) is the algebra the machinery folds.
*
* CONTENTS
*   I.    production<LHS, RHS...>           textual rule
*   II.   grammar<N, Σ, P, S>               textual four-tuple
*   III.  has_lhs / has_rhs /               member-typedef detectors
*         has_nonterminals / has_terminals /
*         has_productions  / has_start_symbol
*   IV.   is_production / is_grammar /      identity traits
*         is_epsilon_production
*   V.    SFINAE-safe extractors
*   VI.   C++20 concepts
*
*   (The polynomial functor F — poly_* value + type level and their
*    Functor / Traversable instances — is in grammar/polynomial.hpp.)
*
*
* path:      /inc/djinterp/parse/grammar/grammar.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_GRAMMAR_HPP
#define DJINTERP_PARSE_GRAMMAR_GRAMMAR_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../core/meta/type_utility.hpp"  // void_t, clean_t
#include "../../core/meta/member_traits.hpp"
#include "../parse.hpp"
#include "../../core/functional/polynomial.hpp"


NS_DJINTERP
NS_PARSE

// ================================================================
//  I.   production
// ================================================================

// production
//   struct: a single context-free production
//
//       LHS → RHS₁ RHS₂ … RHSₙ
//
// where LHS is a nonterminal symbol type and the RHS pack is a
// (possibly empty) sequence of symbol types drawn from N ∪ Σ.  An
// empty RHS pack models an ε-production (LHS → ε).
//
//   In the polynomial-functor view a production is one variant of
// F at the nonterminal LHS: the RHS string is the product of
// children of that variant.  Multiple productions sharing an LHS
// are summed into F's sum at that nonterminal.
template<typename    LHS,
         typename... RHS>
struct production
{
    using lhs = LHS;
    using rhs = std::tuple<RHS...>;

    // arity
    //   value: the length of the RHS string.  Zero indicates an
    // ε-production.
    D_STATIC_CONSTEXPR std::size_t arity = sizeof...(RHS);
};


// ================================================================
//  II.  grammar
// ================================================================

NS_INTERNAL

    // start_symbol_resolver
    //   trait: primary template — explicit start symbol supplied.
    template<typename Nonterminals,
             typename Explicit,
             typename = void>
    struct start_symbol_resolver
    {
        using type = Explicit;
    };

    // start_symbol_resolver (implied case)
    //   trait: no explicit start symbol supplied (Explicit == void)
    // and Nonterminals is tuple-shaped — default to the first
    // nonterminal.
    template<typename Nonterminals>
    struct start_symbol_resolver<
        Nonterminals,
        void,
        void_t<typename std::tuple_element<0, Nonterminals>::type>>
    {
        using type =
            typename std::tuple_element<0, Nonterminals>::type;
    };

NS_END  // internal


// grammar
//   struct: a formal grammar G = (N, Σ, P, S).  The grammar tuple
// is the textual presentation; section III below carries the
// value-level polynomial-functor presentation that participates in
// the framework's protocols.
template<typename Nonterminals,
         typename Terminals,
         typename Productions,
         typename StartSymbol = void>
struct grammar
{
    using nonterminals = Nonterminals;
    using terminals    = Terminals;
    using productions  = Productions;
    using start_symbol =
        typename internal::start_symbol_resolver<
            Nonterminals,
            StartSymbol>::type;
};


// ================================================================
//  III. member-typedef detectors
// ================================================================

// has_lhs
//   trait: detects a nested `lhs` typedef (a production's LHS).
D_DEFINE_HAS_MEMBER_TYPE(lhs)

// has_rhs
//   trait: detects a nested `rhs` typedef (a production's RHS).
D_DEFINE_HAS_MEMBER_TYPE(rhs)

// has_nonterminals
//   trait: detects a nested `nonterminals` typedef.
D_DEFINE_HAS_MEMBER_TYPE(nonterminals)

// has_terminals
//   trait: detects a nested `terminals` typedef.
D_DEFINE_HAS_MEMBER_TYPE(terminals)

// has_productions
//   trait: detects a nested `productions` typedef.
D_DEFINE_HAS_MEMBER_TYPE(productions)

// has_start_symbol
//   trait: detects a nested `start_symbol` typedef.
D_DEFINE_HAS_MEMBER_TYPE(start_symbol)


// ================================================================
//  IV.  identity traits
// ================================================================

NS_INTERNAL

    // is_production_helper
    template<typename T,
             typename = void>
    struct is_production_helper : std::false_type
    {};

    template<typename T>
    struct is_production_helper<
        T,
        void_t<typename clean_t<T>::lhs,
               typename clean_t<T>::rhs>
    > : std::true_type
    {};

NS_END  // internal

// is_production
//   trait: full structural check for production conformance.
template<typename T>
struct is_production : internal::is_production_helper<T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    static constexpr bool is_production_v = is_production<T>::value;
#endif


NS_INTERNAL

    // is_grammar_helper
    template<typename T,
             typename = void>
    struct is_grammar_helper : std::false_type
    {};

    template<typename T>
    struct is_grammar_helper<
        T,
        void_t<typename clean_t<T>::nonterminals,
               typename clean_t<T>::terminals,
               typename clean_t<T>::productions,
               typename clean_t<T>::start_symbol>
    > : std::true_type
    {};

NS_END  // internal

// is_grammar
//   trait: full structural check for grammar conformance.
template<typename T>
struct is_grammar : internal::is_grammar_helper<T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    static constexpr bool is_grammar_v = is_grammar<T>::value;
#endif


NS_INTERNAL

    // is_empty_tuple_helper
    template<typename T>
    struct is_empty_tuple_helper : std::false_type
    {};

    template<>
    struct is_empty_tuple_helper<std::tuple<> > : std::true_type
    {};

    // is_epsilon_production_helper
    template<typename T,
             bool     IsProduction = is_production<T>::value,
             typename               = void>
    struct is_epsilon_production_helper : std::false_type
    {};

    template<typename T>
    struct is_epsilon_production_helper<
        T,
        true,
        typename std::enable_if<
            is_empty_tuple_helper<
                typename clean_t<T>::rhs>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// is_epsilon_production
//   trait: detects a production whose RHS is empty — LHS → ε.
template<typename T>
struct is_epsilon_production
    : internal::is_epsilon_production_helper<T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    static constexpr bool is_epsilon_production_v =
        is_epsilon_production<T>::value;
#endif


// ================================================================
//  V.   SFINAE-safe extractors
// ================================================================

// production_lhs / production_lhs_t
D_DEFINE_MEMBER_TYPE_OR(production_lhs, lhs, void)

// production_rhs / production_rhs_t
D_DEFINE_MEMBER_TYPE_OR(production_rhs, rhs, void)

// grammar_nonterminals / grammar_nonterminals_t
D_DEFINE_MEMBER_TYPE_OR(grammar_nonterminals, nonterminals, void)

// grammar_terminals / grammar_terminals_t
D_DEFINE_MEMBER_TYPE_OR(grammar_terminals, terminals, void)

// grammar_productions / grammar_productions_t
D_DEFINE_MEMBER_TYPE_OR(grammar_productions, productions, void)

// grammar_start_symbol / grammar_start_symbol_t
D_DEFINE_MEMBER_TYPE_OR(grammar_start_symbol, start_symbol, void)


// ================================================================
//  VI.  C++20 concepts
// ================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // production_surface
    template<typename T>
    concept production_surface =
        ( has_lhs<T>::value && has_rhs<T>::value );

    // grammar_surface
    template<typename T>
    concept grammar_surface =
        ( has_nonterminals<T>::value &&
          has_terminals<T>::value    &&
          has_productions<T>::value  &&
          has_start_symbol<T>::value );

    // production_concept
    template<typename T>
    concept production_concept = is_production<T>::value;

    // grammar_concept
    template<typename T>
    concept grammar_concept = is_grammar<T>::value;

    // epsilon_production_concept
    template<typename T>
    concept epsilon_production_concept =
        is_epsilon_production<T>::value;

    // nonempty_production_concept
    template<typename T>
    concept nonempty_production_concept =
        ( is_production<T>::value &&
          !is_epsilon_production<T>::value );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_END  // parse
NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_PARSE_GRAMMAR_GRAMMAR_HPP
