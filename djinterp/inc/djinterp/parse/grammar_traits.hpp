/******************************************************************************
* djinterp [core]                                          grammar_traits.hpp
*
* Grammar SFINAE detection traits:
*   This header provides a suite of compile-time structural traits for
* detecting whether a type conforms to the grammar or production
* interface defined by the djinterp parsing framework.  Detection is
* purely structural — no tagging, no base-class checks, no RTTI — and
* is intentionally agnostic to the underlying input domain.  The
* terminal and nonterminal symbol types may be drawn from any
* alphabet (text, binary, token streams, abstract tags); the traits
* impose no constraint on the user's choice.
*
* Traits provided:
*   - has_lhs<T>                    does T expose `lhs`?
*   - has_rhs<T>                    does T expose `rhs`?
*   - has_nonterminals<T>           does T expose `nonterminals`?
*   - has_terminals<T>              does T expose `terminals`?
*   - has_productions<T>            does T expose `productions`?
*   - has_start_symbol<T>           does T expose `start_symbol`?
*   - is_production<T>              full structural production check
*   - is_grammar<T>                 full structural grammar check
*   - is_epsilon_production<T>      is T a production with empty RHS?
*   - production_lhs<T>             extracts lhs (SFINAE-safe)
*   - production_rhs<T>             extracts rhs (SFINAE-safe)
*   - grammar_nonterminals<T>       extracts nonterminals (SFINAE-safe)
*   - grammar_terminals<T>          extracts terminals (SFINAE-safe)
*   - grammar_productions<T>        extracts productions (SFINAE-safe)
*   - grammar_start_symbol<T>       extracts start_symbol (SFINAE-safe)
*
*
* path:      /inc/cpp/parse/grammar_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_GRAMMAR_TRAITS_
#define DJINTERP_GRAMMAR_TRAITS_ 1

#include <cstddef>
#include <tuple>
#include <type_traits>
#include "./parse.hpp"


NS_DJINTERP
NS_PARSE
NS_TRAITS


// ================================================================
//  has_lhs
// ================================================================

NS_INTERNAL

    // has_lhs_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_lhs_helper : std::false_type
    {};

    // has_lhs_helper (success case)
    //   trait: succeeds when _Type::lhs is well-formed.
    template<typename _Type>
    struct has_lhs_helper<_Type,
        void_t<typename _Type::lhs>
    > : std::true_type
    {};

NS_END  // internal

// has_lhs
//   trait: detects whether _Type exposes a nested `lhs` typedef.
template<typename _Type>
struct has_lhs : internal::has_lhs_helper<_Type>
{};

// has_lhs_v
//   value: convenience alias for has_lhs<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_lhs_v = has_lhs<_Type>::value;
#endif


// ================================================================
//  has_rhs
// ================================================================

NS_INTERNAL

    // has_rhs_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_rhs_helper : std::false_type
    {};

    // has_rhs_helper (success case)
    //   trait: succeeds when _Type::rhs is well-formed.
    template<typename _Type>
    struct has_rhs_helper<_Type,
        void_t<typename _Type::rhs>
    > : std::true_type
    {};

NS_END  // internal

// has_rhs
//   trait: detects whether _Type exposes a nested `rhs` typedef.
template<typename _Type>
struct has_rhs : internal::has_rhs_helper<_Type>
{};

// has_rhs_v
//   value: convenience alias for has_rhs<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_rhs_v = has_rhs<_Type>::value;
#endif


// ================================================================
//  has_nonterminals
// ================================================================

NS_INTERNAL

    // has_nonterminals_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_nonterminals_helper : std::false_type
    {};

    // has_nonterminals_helper (success case)
    //   trait: succeeds when _Type::nonterminals is well-formed.
    template<typename _Type>
    struct has_nonterminals_helper<_Type,
        void_t<typename _Type::nonterminals>
    > : std::true_type
    {};

NS_END  // internal

// has_nonterminals
//   trait: detects whether _Type exposes a nested `nonterminals`
// typedef.
template<typename _Type>
struct has_nonterminals : internal::has_nonterminals_helper<_Type>
{};

// has_nonterminals_v
//   value: convenience alias for has_nonterminals<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_nonterminals_v =
        has_nonterminals<_Type>::value;
#endif


// ================================================================
//  has_terminals
// ================================================================

NS_INTERNAL

    // has_terminals_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_terminals_helper : std::false_type
    {};

    // has_terminals_helper (success case)
    //   trait: succeeds when _Type::terminals is well-formed.
    template<typename _Type>
    struct has_terminals_helper<_Type,
        void_t<typename _Type::terminals>
    > : std::true_type
    {};

NS_END  // internal

// has_terminals
//   trait: detects whether _Type exposes a nested `terminals`
// typedef.
template<typename _Type>
struct has_terminals : internal::has_terminals_helper<_Type>
{};

// has_terminals_v
//   value: convenience alias for has_terminals<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_terminals_v = has_terminals<_Type>::value;
#endif


// ================================================================
//  has_productions
// ================================================================

NS_INTERNAL

    // has_productions_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_productions_helper : std::false_type
    {};

    // has_productions_helper (success case)
    //   trait: succeeds when _Type::productions is well-formed.
    template<typename _Type>
    struct has_productions_helper<_Type,
        void_t<typename _Type::productions>
    > : std::true_type
    {};

NS_END  // internal

// has_productions
//   trait: detects whether _Type exposes a nested `productions`
// typedef.
template<typename _Type>
struct has_productions : internal::has_productions_helper<_Type>
{};

// has_productions_v
//   value: convenience alias for has_productions<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_productions_v =
        has_productions<_Type>::value;
#endif


// ================================================================
//  has_start_symbol
// ================================================================

NS_INTERNAL

    // has_start_symbol_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_start_symbol_helper : std::false_type
    {};

    // has_start_symbol_helper (success case)
    //   trait: succeeds when _Type::start_symbol is well-formed.
    template<typename _Type>
    struct has_start_symbol_helper<_Type,
        void_t<typename _Type::start_symbol>
    > : std::true_type
    {};

NS_END  // internal

// has_start_symbol
//   trait: detects whether _Type exposes a nested `start_symbol`
// typedef.
template<typename _Type>
struct has_start_symbol : internal::has_start_symbol_helper<_Type>
{};

// has_start_symbol_v
//   value: convenience alias for has_start_symbol<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_start_symbol_v =
        has_start_symbol<_Type>::value;
#endif


// ================================================================
//  is_production
// ================================================================

NS_INTERNAL

    // is_production_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_production_helper : std::false_type
    {};

    // is_production_helper (success case)
    //   trait: succeeds when _Type structurally satisfies the
    // production contract: it exposes both `lhs` and `rhs`
    // nested typedefs.  The pairing of these two names is
    // taken as a sufficient structural signature.
    template<typename _Type>
    struct is_production_helper<_Type,
        void_t<
            typename _Type::lhs,
            typename _Type::rhs
        >
    > : std::true_type
    {};

NS_END  // internal

// is_production
//   trait: full structural check for production conformance.
// Returns true when _Type exposes both `lhs` and `rhs` nested
// typedefs.
template<typename _Type>
struct is_production : internal::is_production_helper<_Type>
{};

// is_production_v
//   value: convenience alias for is_production<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_production_v = is_production<_Type>::value;
#endif


// ================================================================
//  is_grammar
// ================================================================

NS_INTERNAL

    // is_grammar_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct is_grammar_helper : std::false_type
    {};

    // is_grammar_helper (success case)
    //   trait: succeeds when _Type structurally satisfies the
    // grammar contract: it exposes nonterminals, terminals,
    // productions, and start_symbol nested typedefs.
    template<typename _Type>
    struct is_grammar_helper<_Type,
        void_t<
            typename _Type::nonterminals,
            typename _Type::terminals,
            typename _Type::productions,
            typename _Type::start_symbol
        >
    > : std::true_type
    {};

NS_END  // internal

// is_grammar
//   trait: full structural check for grammar conformance.
// Returns true when _Type exposes nonterminals, terminals,
// productions, and start_symbol nested typedefs — the four
// components of the formal grammar tuple.
template<typename _Type>
struct is_grammar : internal::is_grammar_helper<_Type>
{};

// is_grammar_v
//   value: convenience alias for is_grammar<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_grammar_v = is_grammar<_Type>::value;
#endif


// ================================================================
//  is_epsilon_production
// ================================================================

NS_INTERNAL

    // is_empty_tuple_helper
    //   trait: detects the empty std::tuple specifically, without
    // relying on std::tuple_size behaviour for non-tuple types
    // (which is implementation-defined on pre-C++17 toolchains).
    template<typename _Type>
    struct is_empty_tuple_helper : std::false_type
    {};

    // is_empty_tuple_helper<std::tuple<>>
    //   trait: specialization recognising the empty tuple.
    template<>
    struct is_empty_tuple_helper<std::tuple<>> : std::true_type
    {};

    // is_epsilon_production_helper
    //   trait: primary template (failure case).  Non-productions
    // and productions whose RHS is non-empty fall here.
    template<typename _Type,
             bool     _IsProduction = is_production<_Type>::value,
             typename               = void>
    struct is_epsilon_production_helper : std::false_type
    {};

    // is_epsilon_production_helper (success case)
    //   trait: succeeds when _Type is a production whose RHS is
    // the empty tuple — i.e. LHS → ε.
    template<typename _Type>
    struct is_epsilon_production_helper<
        _Type,
        true,
        typename std::enable_if<
            is_empty_tuple_helper<typename _Type::rhs>::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// is_epsilon_production
//   trait: detects whether _Type is a production whose RHS is
// the empty string — i.e. an ε-production of the form LHS → ε.
template<typename _Type>
struct is_epsilon_production
    : internal::is_epsilon_production_helper<_Type>
{};

// is_epsilon_production_v
//   value: convenience alias for is_epsilon_production<_Type>::value.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_epsilon_production_v =
        is_epsilon_production<_Type>::value;
#endif


// ================================================================
//  production_lhs  /  production_rhs
// ================================================================
// SFINAE-safe type extractors.  Produce `void` when the queried
// type does not expose the expected member typedef.

NS_INTERNAL

    // production_lhs_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct production_lhs_helper
    {
        using type = void;
    };

    // production_lhs_helper (success case)
    //   trait: extracts _Type::lhs when available.
    template<typename _Type>
    struct production_lhs_helper<
        _Type,
        void_t<typename _Type::lhs>
    >
    {
        using type = typename _Type::lhs;
    };

    // production_rhs_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct production_rhs_helper
    {
        using type = void;
    };

    // production_rhs_helper (success case)
    //   trait: extracts _Type::rhs when available.
    template<typename _Type>
    struct production_rhs_helper<
        _Type,
        void_t<typename _Type::rhs>
    >
    {
        using type = typename _Type::rhs;
    };

NS_END  // internal

// production_lhs
//   trait: SFINAE-safe extraction of a production's lhs.
// Produces void if _Type does not expose lhs.
template<typename _Type>
struct production_lhs : internal::production_lhs_helper<_Type>
{};

// production_lhs_t
//   type: convenience alias for production_lhs<_Type>::type.
template<typename _Type>
using production_lhs_t = typename production_lhs<_Type>::type;

// production_rhs
//   trait: SFINAE-safe extraction of a production's rhs.
// Produces void if _Type does not expose rhs.
template<typename _Type>
struct production_rhs : internal::production_rhs_helper<_Type>
{};

// production_rhs_t
//   type: convenience alias for production_rhs<_Type>::type.
template<typename _Type>
using production_rhs_t = typename production_rhs<_Type>::type;


// ================================================================
//  grammar component extractors
// ================================================================
// SFINAE-safe extractors for the four components of a grammar.
// Each produces `void` when the queried type does not expose the
// corresponding member typedef.

NS_INTERNAL

    // grammar_nonterminals_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct grammar_nonterminals_helper
    {
        using type = void;
    };

    // grammar_nonterminals_helper (success case)
    //   trait: extracts _Type::nonterminals when available.
    template<typename _Type>
    struct grammar_nonterminals_helper<
        _Type,
        void_t<typename _Type::nonterminals>
    >
    {
        using type = typename _Type::nonterminals;
    };

    // grammar_terminals_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct grammar_terminals_helper
    {
        using type = void;
    };

    // grammar_terminals_helper (success case)
    //   trait: extracts _Type::terminals when available.
    template<typename _Type>
    struct grammar_terminals_helper<
        _Type,
        void_t<typename _Type::terminals>
    >
    {
        using type = typename _Type::terminals;
    };

    // grammar_productions_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct grammar_productions_helper
    {
        using type = void;
    };

    // grammar_productions_helper (success case)
    //   trait: extracts _Type::productions when available.
    template<typename _Type>
    struct grammar_productions_helper<
        _Type,
        void_t<typename _Type::productions>
    >
    {
        using type = typename _Type::productions;
    };

    // grammar_start_symbol_helper
    //   trait: primary template (produces void).
    template<typename _Type,
             typename = void>
    struct grammar_start_symbol_helper
    {
        using type = void;
    };

    // grammar_start_symbol_helper (success case)
    //   trait: extracts _Type::start_symbol when available.
    template<typename _Type>
    struct grammar_start_symbol_helper<
        _Type,
        void_t<typename _Type::start_symbol>
    >
    {
        using type = typename _Type::start_symbol;
    };

NS_END  // internal

// grammar_nonterminals
//   trait: SFINAE-safe extraction of a grammar's nonterminal set.
// Produces void if _Type does not expose nonterminals.
template<typename _Type>
struct grammar_nonterminals
    : internal::grammar_nonterminals_helper<_Type>
{};

// grammar_nonterminals_t
//   type: convenience alias for grammar_nonterminals<_Type>::type.
template<typename _Type>
using grammar_nonterminals_t =
    typename grammar_nonterminals<_Type>::type;

// grammar_terminals
//   trait: SFINAE-safe extraction of a grammar's terminal set.
// Produces void if _Type does not expose terminals.
template<typename _Type>
struct grammar_terminals
    : internal::grammar_terminals_helper<_Type>
{};

// grammar_terminals_t
//   type: convenience alias for grammar_terminals<_Type>::type.
template<typename _Type>
using grammar_terminals_t =
    typename grammar_terminals<_Type>::type;

// grammar_productions
//   trait: SFINAE-safe extraction of a grammar's production set.
// Produces void if _Type does not expose productions.
template<typename _Type>
struct grammar_productions
    : internal::grammar_productions_helper<_Type>
{};

// grammar_productions_t
//   type: convenience alias for grammar_productions<_Type>::type.
template<typename _Type>
using grammar_productions_t =
    typename grammar_productions<_Type>::type;

// grammar_start_symbol
//   trait: SFINAE-safe extraction of a grammar's start symbol.
// Produces void if _Type does not expose start_symbol.
template<typename _Type>
struct grammar_start_symbol
    : internal::grammar_start_symbol_helper<_Type>
{};

// grammar_start_symbol_t
//   type: convenience alias for grammar_start_symbol<_Type>::type.
template<typename _Type>
using grammar_start_symbol_t =
    typename grammar_start_symbol<_Type>::type;


NS_END  // traits
NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_GRAMMAR_TRAITS_
