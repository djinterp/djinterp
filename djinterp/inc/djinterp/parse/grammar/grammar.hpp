/******************************************************************************
* djinterp [parse]                                          grammar/grammar.hpp
*
* Formal grammar primitives and polynomial-functor view.
*   Per ch-parsing.tex a grammar is the language-theoretic four-tuple
*
*       G = (N, Σ, P, S)
*
* with N the nonterminals, Σ the terminals, P the productions, and
* S ∈ N the start symbol; and a parsable carrier D is the initial
* algebra μF of a polynomial functor F — built from constants, +, ×,
* and composition — that is Traversable.  The grammar and the
* polynomial functor are two faces of the same thing:
*
*   - Each production LHS → RHS₁ RHS₂ … RHSₙ is one variant of F.
*   - The disjoint union of productions sharing an LHS is F's sum.
*   - The RHS sequence is F's product.
*   - Recursive references to nonterminals are F's recursion (the μ).
*   - The traversal order over F's children is the parser's
*     left-to-right consumption.
*
*   This header carries the four-tuple presentation (production +
* grammar) — the textual face the parser literature works with — and
* the type-level polynomial face the formal definition works with.
* Both are agnostic to the underlying input domain: terminal and
* nonterminal symbol types are abstract template parameters drawn
* from any alphabet (text, binary, token streams, user-defined tag
* symbols).
*
*   The grammar imposes no semantic actions, attribute schemes, or
* parser-construction artefacts; those belong to derivative modules
* layered on top.  The companion file parser/parser.hpp gives the
* parser carrier P A; prism.hpp gives compose = cata[φ] : μF → Σ*
* and the round-trip laws relating compose and parse.
*
* CONTENTS
*   I.    production<LHS, RHS...>           LHS → RHS₁ RHS₂ … RHSₙ
*   II.   grammar<N, Σ, P, S>               the four-tuple
*   III.  polynomial functor shape          constant / sum / product
*                                           / mu — the type-level F
*   IV.   has_lhs / has_rhs /               member-typedef detectors
*         has_nonterminals / has_terminals /
*         has_productions  / has_start_symbol
*   V.    is_production / is_grammar /      identity traits
*         is_epsilon_production
*   VI.   production_lhs / production_rhs   SFINAE-safe extractors
*         grammar_nonterminals / ... etc.
*   VII.  C++20 concepts mirroring the traits
*
* path:      /inc/djinterp/parse/grammar/grammar.hpp
* link(s):   ch-parsing.tex
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_PARSE_GRAMMAR_
#define DJINTERP_PARSE_GRAMMAR_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../core/meta/member_traits.hpp"
#include "../parse.hpp"


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
// where _LHS is a nonterminal symbol type and the _RHS pack is a
// (possibly empty) sequence of symbol types drawn from N ∪ Σ.  An
// empty _RHS pack models an ε-production (LHS → ε).
//
//   Symbol types are unconstrained — tag structs, enum-class
// values lifted via std::integral_constant, or any other
// distinguishable type the user prefers.  This makes the
// production agnostic to the input domain its symbols are drawn
// from.
//
//   In the polynomial-functor view a production is one variant of
// F at the nonterminal _LHS: the RHS string is the product of
// _RHS… children of that variant.  Multiple productions sharing an
// LHS are summed into F's sum at that variable.
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
//  II.  grammar
// ================================================================

NS_INTERNAL

    // start_symbol_resolver
    //   trait: primary template — explicit start symbol supplied.
    template<typename _Nonterminals,
             typename _Explicit,
             typename = void>
    struct start_symbol_resolver
    {
        using type = _Explicit;
    };

    // start_symbol_resolver (implied case)
    //   trait: no explicit start symbol supplied (_Explicit == void)
    // and _Nonterminals is tuple-shaped — default to the first
    // nonterminal.
    template<typename _Nonterminals>
    struct start_symbol_resolver<
        _Nonterminals,
        void,
        void_t<typename std::tuple_element<0, _Nonterminals>::type>>
    {
        using type =
            typename std::tuple_element<0, _Nonterminals>::type;
    };

NS_END  // internal


// grammar
//   struct: a formal grammar G = (N, Σ, P, S).
//
//   _Nonterminals  N — the finite set of nonterminal symbols.
//   _Terminals     Σ — the finite set of terminal symbols (the
//                  alphabet over which the language is defined).
//   _Productions   P — the finite set of production rules,
//                  typically a typelist of `production`
//                  instantiations.
//   _StartSymbol   S ∈ N.  May be omitted (or supplied as void);
//                  when omitted the start symbol defaults to the
//                  first nonterminal when _Nonterminals is tuple-
//                  shaped.  Non-tuple representations must supply
//                  the start symbol explicitly.
//
//   In the polynomial-functor view, the grammar tuple is the
// presentation of a polynomial endofunctor F on the category whose
// objects are the nonterminals: the productions describe F's sum-
// of-products shape, and the parsable carriers D for each
// nonterminal are F's initial algebras μF.
template<typename _Nonterminals,
         typename _Terminals,
         typename _Productions,
         typename _StartSymbol = void>
struct grammar
{
    using nonterminals = _Nonterminals;
    using terminals    = _Terminals;
    using productions  = _Productions;
    using start_symbol =
        typename internal::start_symbol_resolver<
            _Nonterminals,
            _StartSymbol>::type;
};


// ================================================================
//  III. polynomial functor shape
// ================================================================
//   Type-level building blocks for F directly, parallel to the
// grammar tuple but more aligned with the formal definition.  These
// are not used by the parser combinators (which build their carrier
// from primitives + combinators rather than from F-algebra reified
// machinery), but they are available for code that wants to talk
// about F in its own terms — for example a generic compose
// (catamorphism) over μF — see prism.hpp.

// poly_constant
//   struct: F<X> = K — the X-variable does not appear; the variant
// carries a fixed _Constant payload.
template<typename _Constant>
struct poly_constant
{
    using constant_type = _Constant;
};

// poly_recursion
//   struct: F<X> = X — the recursive position, where μF is
// substituted on closure.
struct poly_recursion
{};

// poly_sum
//   struct: F<X> = _F<X> + _G<X> — disjoint union of two polynomial
// variants.
template<typename _F,
         typename _G>
struct poly_sum
{
    using left  = _F;
    using right = _G;
};

// poly_product
//   struct: F<X> = _Variants₁<X> × _Variants₂<X> × … — n-ary product
// of polynomial children.
template<typename... _Variants>
struct poly_product
{
    using children = std::tuple<_Variants...>;

    D_STATIC_CONSTEXPR std::size_t arity = sizeof...(_Variants);
};

// poly_compose
//   struct: F<X> = _Outer<_Inner<X>> — composition of two polynomial
// functors.
template<typename _Outer,
         typename _Inner>
struct poly_compose
{
    using outer = _Outer;
    using inner = _Inner;
};

// poly_mu
//   struct: μF — the initial algebra of a polynomial functor.  This
// is the C++ stand-in for the recursive-fixpoint type D ≅ F<D>; in
// practice a parsable type carries this shape implicitly via its
// own constructor declarations, but poly_mu<F> is the formal name
// when generic algorithms want to talk about it.
template<typename _F>
struct poly_mu
{
    using functor = _F;
};


// ================================================================
//  IV.  member-typedef detectors
// ================================================================

// has_lhs
//   trait: detects a nested `lhs` typedef (a production's LHS).
D_DEFINE_HAS_MEMBER_TYPE(lhs)

// has_rhs
//   trait: detects a nested `rhs` typedef (a production's RHS).
D_DEFINE_HAS_MEMBER_TYPE(rhs)

// has_nonterminals
//   trait: detects a nested `nonterminals` typedef (the set N).
D_DEFINE_HAS_MEMBER_TYPE(nonterminals)

// has_terminals
//   trait: detects a nested `terminals` typedef (the alphabet Σ).
D_DEFINE_HAS_MEMBER_TYPE(terminals)

// has_productions
//   trait: detects a nested `productions` typedef (the set P).
D_DEFINE_HAS_MEMBER_TYPE(productions)

// has_start_symbol
//   trait: detects a nested `start_symbol` typedef (S ∈ N).
D_DEFINE_HAS_MEMBER_TYPE(start_symbol)


// ================================================================
//  V.   identity traits
// ================================================================

NS_INTERNAL

    // is_production_helper
    //   trait: primary template (failure case).
    template<typename _T,
             typename = void>
    struct is_production_helper : std::false_type
    {};

    // is_production_helper (success case)
    //   trait: succeeds when _T exposes both lhs and rhs nested
    // typedefs.
    template<typename _T>
    struct is_production_helper<
        _T,
        void_t<typename clean_t<_T>::lhs,
               typename clean_t<_T>::rhs>
    > : std::true_type
    {};

NS_END  // internal

// is_production
//   trait: full structural check for production conformance.
template<typename _T>
struct is_production : internal::is_production_helper<_T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_production_v = is_production<_T>::value;
#endif


NS_INTERNAL

    // is_grammar_helper
    //   trait: primary template (failure case).
    template<typename _T,
             typename = void>
    struct is_grammar_helper : std::false_type
    {};

    // is_grammar_helper (success case)
    //   trait: succeeds when _T exposes the four nested typedefs
    // of the formal grammar tuple.
    template<typename _T>
    struct is_grammar_helper<
        _T,
        void_t<typename clean_t<_T>::nonterminals,
               typename clean_t<_T>::terminals,
               typename clean_t<_T>::productions,
               typename clean_t<_T>::start_symbol>
    > : std::true_type
    {};

NS_END  // internal

// is_grammar
//   trait: full structural check for grammar conformance.
template<typename _T>
struct is_grammar : internal::is_grammar_helper<_T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_grammar_v = is_grammar<_T>::value;
#endif


NS_INTERNAL

    // is_empty_tuple_helper
    //   trait: detects std::tuple<> specifically.
    template<typename _T>
    struct is_empty_tuple_helper : std::false_type
    {};

    template<>
    struct is_empty_tuple_helper<std::tuple<> > : std::true_type
    {};

    // is_epsilon_production_helper
    //   trait: primary template (failure case).
    template<typename _T,
             bool     _IsProduction = is_production<_T>::value,
             typename               = void>
    struct is_epsilon_production_helper : std::false_type
    {};

    // is_epsilon_production_helper (success case)
    //   trait: succeeds when _T is a production whose RHS is the
    // empty tuple — i.e. LHS → ε.
    template<typename _T>
    struct is_epsilon_production_helper<
        _T,
        true,
        typename std::enable_if<
            is_empty_tuple_helper<
                typename clean_t<_T>::rhs>::value>::type
    > : std::true_type
    {};

NS_END  // internal

// is_epsilon_production
//   trait: detects a production whose RHS is empty — LHS → ε.
template<typename _T>
struct is_epsilon_production
    : internal::is_epsilon_production_helper<_T>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    static constexpr bool is_epsilon_production_v =
        is_epsilon_production<_T>::value;
#endif


// ================================================================
//  VI.  SFINAE-safe extractors
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
//  VII. C++20 concepts
// ================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // production_surface
    //   concept: a type exposing both lhs and rhs.
    template<typename _T>
    concept production_surface =
        ( has_lhs<_T>::value && has_rhs<_T>::value );

    // grammar_surface
    //   concept: a type exposing the full four-part grammar tuple.
    template<typename _T>
    concept grammar_surface =
        ( has_nonterminals<_T>::value &&
          has_terminals<_T>::value    &&
          has_productions<_T>::value  &&
          has_start_symbol<_T>::value );

    // production_concept
    //   concept: structurally conforming production.
    template<typename _T>
    concept production_concept = is_production<_T>::value;

    // grammar_concept
    //   concept: structurally conforming grammar.
    template<typename _T>
    concept grammar_concept = is_grammar<_T>::value;

    // epsilon_production_concept
    //   concept: a production whose RHS is empty.
    template<typename _T>
    concept epsilon_production_concept =
        is_epsilon_production<_T>::value;

    // nonempty_production_concept
    //   concept: a production whose RHS is non-empty.
    template<typename _T>
    concept nonempty_production_concept =
        ( is_production<_T>::value &&
          !is_epsilon_production<_T>::value );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_PARSE_GRAMMAR_
