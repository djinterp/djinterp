/******************************************************************************
* djinterp [meta]                                              trait_detect.hpp
*
*   The detection-trait MACRO TOOLKIT, factored out of type_traits.hpp so the
* whole framework draws its SFINAE / detection-idiom machinery from one place.
* Previously this lived as section 0 of type_traits.hpp and was partially
* duplicated by member_traits.hpp (which carried its own nested-typedef
* detector macro).  Both are now expressed here, once.
*
*   WHAT THIS HEADER OWNS:
*     - D_VOID_T                       portable void_t selector.
*     - D_TYPE_TRAIT_VALUE_BOOL        emit the `_v` companion of a unary trait.
*     - D_TYPE_TRAIT_TYPE_ALIAS        emit the `_t` companion of a unary trait.
*     - D_TYPE_TRAIT_TRUE(_AS/_FROM)   the SFINAE bool-trait engine.
*     - D_TYPE_TRAIT_EXPR_*            decltype-expression builders (the things
*                                      you feed to the engine as DETECTION_EXPR).
*     - D_TYPE_TRAIT_HAS_*             purpose-named sugar (type / method / op /
*                                      static-member detection).
*     - D_TYPE_TRAIT_IS_SPECIALIZATION_OF(_AS)
*                                      the partial-specialization "is this a
*                                      foo<...>?" family (a DIFFERENT mechanism
*                                      from the SFINAE engine).
*     - D_TYPE_TRAIT_MEMBER_TYPE_OR    extract a nested typedef or fall back.
*
*   NO ALIASES:
*   Every macro here resolves to something the others do not.  Where two
* spellings used to mean the same thing they have been collapsed to one:
*     - D_TRAIT_IS_DETECTED        -> D_TYPE_TRAIT_TRUE
*     - D_TRAIT_IS_DETECTED_AS     -> D_TYPE_TRAIT_TRUE_AS
*     - D_TRAIT_IS_DETECTED_FROM   -> D_TYPE_TRAIT_TRUE_FROM
*     - D_TRAIT_DETECT_*           -> D_TYPE_TRAIT_EXPR_*
*     - D_TRAIT_HAS_* / D_TRAIT_TYPE_ALIAS / D_TRAIT_VALUE_BOOL
*                                  -> D_TYPE_TRAIT_HAS_* / _TYPE_ALIAS / _VALUE_BOOL
*     - member_traits' D_DEFINE_HAS_MEMBER_TYPE(NAME) is GONE: it was
*       D_TYPE_TRAIT_HAS_TYPE with the trait name auto-derived, so callers now
*       say D_TYPE_TRAIT_HAS_TYPE(has_NAME, NAME) directly.
*     - member_traits' D_DEFINE_MEMBER_TYPE_OR -> D_TYPE_TRAIT_MEMBER_TYPE_OR.
*
*   The generic ENGINE stays generic; the per-purpose meaning lives in the
* HAS_* sugar.  The sugar bakes a concrete DETECTION_EXPR into the engine,
* which is a real resolution difference, not a rename.
*
*   PORTABILITY:
*   C++11 floor.  The `_v` companion degrades with the language: an inline
* variable template on C++17+, a (non-inline) variable template on C++14, and
* nothing on C++11 (where variable templates do not exist) - the `::value`
* member is always present, only the `_v` shorthand is conditional.
*
*   INVOCATION SCOPE:
*   These macros are defined at FILE SCOPE and are namespace-agnostic: the
* trait they emit lands in whatever namespace the macro is invoked in.  Macros
* that open an `internal` namespace (D_TYPE_TRAIT_MEMBER_TYPE_OR) and macros
* that mention `clean_t` unqualified MUST be invoked inside the djinterp
* namespace so those names resolve and `internal` nests correctly.
*
*
* path:      /inc/djinterp/core/meta/trait_detect.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.04
******************************************************************************/

#ifndef DJINTERP_META_TRAIT_DETECT_
#define DJINTERP_META_TRAIT_DETECT_ 1

// std
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"   // void_t, clean_t, NS_*, D_ENV_* feature macros


// ===========================================================================
//  void_t selector
// ===========================================================================

// D_VOID_T
//   macro: portable void_t for SFINAE sinks.  Resolves to std::void_t in
// C++17+, djinterp::void_t otherwise.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_VOID_T  std::void_t
#elif D_ENV_LANG_IS_CPP11_OR_HIGHER
    #define D_VOID_T  djinterp::void_t
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


// ===========================================================================
//  companion emitters  (_v / _t)
// ===========================================================================

// D_TYPE_TRAIT_VALUE_BOOL
//   macro: emit the `TRAIT_NAME##_v` companion of a unary trait exposing
// `::value`.  Inline variable template on C++17+, plain variable template on
// C++14, and a no-op on C++11 (no variable templates).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)                               \
        template<typename _Type>                                             \
        inline constexpr bool TRAIT_NAME##_v = TRAIT_NAME<_Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    #define D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)                               \
        template<typename _Type>                                             \
        constexpr bool TRAIT_NAME##_v = TRAIT_NAME<_Type>::value;
#else
    #define D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)  /* no variable templates */
#endif

// D_TYPE_TRAIT_TYPE_ALIAS
//   macro: emit the `TRAIT_NAME##_t` companion of a unary trait exposing
// `::type`.  The `_t` counterpart to D_TYPE_TRAIT_VALUE_BOOL.
#define D_TYPE_TRAIT_TYPE_ALIAS(TRAIT_NAME)                                   \
    template<typename _Type>                                                 \
    using TRAIT_NAME##_t = typename TRAIT_NAME<_Type>::type;


// ===========================================================================
//  the SFINAE bool-trait engine
// ===========================================================================

// D_TYPE_TRAIT_TRUE_AS
//   macro: parameterized core of the detection family.  Emits the primary
// template (std::false_type) plus the well-formed partial specialization
// whose base is INHERIT_EXPR (an arbitrary type expression that may mention
// `_Type`).  Does NOT emit the `_v` companion - the caller, or one of the
// shorthands below, adds it via D_TYPE_TRAIT_VALUE_BOOL.
// (Was D_TRAIT_IS_DETECTED_AS.)
#define D_TYPE_TRAIT_TRUE_AS(TRAIT_NAME, DETECTION_EXPR, INHERIT_EXPR)        \
    template<typename _Type,                                                 \
             typename = void>                                                \
    struct TRAIT_NAME : std::false_type                                      \
    {};                                                                      \
                                                                             \
    template<typename _Type>                                                 \
    struct TRAIT_NAME<_Type, D_VOID_T<DETECTION_EXPR>> : INHERIT_EXPR        \
    {};

// D_TYPE_TRAIT_TRUE
//   macro: the bread-and-butter SFINAE trait.  Emits `TRAIT_NAME<_Type>`,
// std::true_type when every detection expression in `...` is well-formed for
// _Type and std::false_type otherwise, plus the `_v` companion.  Variadic:
// pass one expression for a simple check, or several to require all of them
// (the AND-shape of the void_t idiom).
// (Was D_TRAIT_IS_DETECTED; documented in containers_howto as the canonical
// SFINAE-bool-trait macro.)
#define D_TYPE_TRAIT_TRUE(TRAIT_NAME, ...)                                    \
    template<typename _Type,                                                 \
             typename = void>                                                \
    struct TRAIT_NAME : std::false_type                                      \
    {};                                                                      \
                                                                             \
    template<typename _Type>                                                 \
    struct TRAIT_NAME<_Type, D_VOID_T<__VA_ARGS__>> : std::true_type         \
    {};                                                                      \
                                                                             \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TYPE_TRAIT_TRUE_FROM
//   macro: like D_TYPE_TRAIT_TRUE, but on success inherits from
// `BASE_TRAIT<_Type>` instead of std::true_type, so the trait can delegate
// further checks to another unary trait.
// (Was D_TRAIT_IS_DETECTED_FROM.)
#define D_TYPE_TRAIT_TRUE_FROM(TRAIT_NAME, DETECTION_EXPR, BASE_TRAIT)        \
    D_TYPE_TRAIT_TRUE_AS(TRAIT_NAME, DETECTION_EXPR, BASE_TRAIT<_Type>)       \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)


// ===========================================================================
//  decltype-expression builders  (feed these to the engine)
// ===========================================================================

// D_TYPE_TRAIT_EXPR_METHOD
//   macro: a decltype-expression detecting a call to
// `_Type::METHOD_NAME(_Type::value_type{})`.  Pass as the DETECTION_EXPR
// argument of a D_TYPE_TRAIT_TRUE* macro.
// (Was D_TRAIT_DETECT_METHOD.)
#define D_TYPE_TRAIT_EXPR_METHOD(METHOD_NAME)                                 \
    decltype(std::declval<_Type&>().METHOD_NAME(                             \
        std::declval<typename _Type::value_type>()))

// D_TYPE_TRAIT_EXPR_METHOD_ARGS
//   macro: a decltype-expression detecting a call to `_Type::METHOD_NAME(...)`
// with arguments of the variadic types supplied.
// (Was D_TRAIT_DETECT_METHOD_ARGS.)
#define D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, ...)                       \
    decltype(std::declval<_Type&>().METHOD_NAME(                             \
        std::declval<__VA_ARGS__>()))

// D_TYPE_TRAIT_EXPR_BINARY_OP
//   macro: a decltype-expression detecting `_Type OP _Type`.  Operands are
// `const _Type&` so the probe accepts immutable operands and is not defeated
// by const-qualified types.
// (Was D_TRAIT_DETECT_BINARY_OP.)
#define D_TYPE_TRAIT_EXPR_BINARY_OP(OP)                                       \
    decltype(std::declval<const _Type&>() OP std::declval<const _Type&>())

// D_TYPE_TRAIT_EXPR_UNARY_OP
//   macro: a decltype-expression detecting prefix `OP _Type` (`-x`, `*x`,
// `!x`, `++x`, ...).
// (Was D_TRAIT_DETECT_UNARY_OP.)
#define D_TYPE_TRAIT_EXPR_UNARY_OP(OP)                                        \
    decltype(OP std::declval<_Type&>())


// ===========================================================================
//  purpose-named sugar
// ===========================================================================
//   Each macro bakes a concrete DETECTION_EXPR (or success-base) into the
// engine.  That baked content is the resolution difference that earns the
// macro its place - none of these is a rename of the engine.

// D_TYPE_TRAIT_HAS_TYPE
//   macro: trait true iff `_Type` exposes a nested typedef `TYPE_NAME`.  The
// probe strips cv-ref via `clean_t` first, so `has_X<const T&>` agrees with
// `has_X<T>` - this is the behavior the old member_traits detector had, now
// the single canonical one.  (Subsumes member_traits' D_DEFINE_HAS_MEMBER_TYPE
// and the cv-naive D_TRAIT_HAS_TYPE.)
#define D_TYPE_TRAIT_HAS_TYPE(TRAIT_NAME, TYPE_NAME)                          \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, typename clean_t<_Type>::TYPE_NAME)

// D_TYPE_TRAIT_HAS_STATIC_MEMBER
//   macro: trait true iff `decltype(_Type::MEMBER_NAME)` is well-formed (the
// name exists at class scope).  Does NOT constrain the member's type; combine
// with the multi-expression form of D_TYPE_TRAIT_TRUE for that.
// (Was D_TRAIT_HAS_STATIC_MEMBER.)
#define D_TYPE_TRAIT_HAS_STATIC_MEMBER(TRAIT_NAME, MEMBER_NAME)               \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, decltype(_Type::MEMBER_NAME))

// D_TYPE_TRAIT_HAS_METHOD
//   macro: trait true iff `_Type` has `METHOD_NAME` callable with a single
// `_Type::value_type` argument.
// (Was D_TRAIT_HAS_METHOD.)
#define D_TYPE_TRAIT_HAS_METHOD(TRAIT_NAME, METHOD_NAME)                      \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, D_TYPE_TRAIT_EXPR_METHOD(METHOD_NAME))

// D_TYPE_TRAIT_HAS_METHOD_ARGS
//   macro: trait true iff `_Type` has `METHOD_NAME` callable with arguments
// of the variadic types supplied.
// (Was D_TRAIT_HAS_METHOD_ARGS.)
#define D_TYPE_TRAIT_HAS_METHOD_ARGS(TRAIT_NAME, METHOD_NAME, ...)            \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME,                                            \
        D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, __VA_ARGS__))

// D_TYPE_TRAIT_HAS_METHOD_TYPED
//   macro: trait true iff `_Type` has `METHOD_NAME` callable with the given
// argument types AND whose return type is exactly RETURN_TYPE.  Inherits from
// std::is_same in the success case.
// (Was D_TRAIT_HAS_METHOD_TYPED.)
#define D_TYPE_TRAIT_HAS_METHOD_TYPED(TRAIT_NAME, METHOD_NAME, RETURN_TYPE, ...) \
    D_TYPE_TRAIT_TRUE_AS(TRAIT_NAME,                                          \
        D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),             \
        std::is_same<D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, __VA_ARGS__), \
                     RETURN_TYPE>)                                           \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TYPE_TRAIT_HAS_METHOD_CONVERTIBLE
//   macro: like D_TYPE_TRAIT_HAS_METHOD_TYPED, but the call's return type need
// only be CONVERTIBLE to RETURN_TYPE (e.g. a `size()` returning `unsigned`
// where you want `size_t`).
// (Was D_TRAIT_HAS_METHOD_CONVERTIBLE.)
#define D_TYPE_TRAIT_HAS_METHOD_CONVERTIBLE(TRAIT_NAME,                       \
                                            METHOD_NAME,                     \
                                            RETURN_TYPE,                     \
                                            ...)                             \
    D_TYPE_TRAIT_TRUE_AS(TRAIT_NAME,                                          \
        D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),             \
        std::is_convertible<                                                 \
            D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, __VA_ARGS__),         \
            RETURN_TYPE>)                                                    \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TYPE_TRAIT_HAS_BINARY_OP
//   macro: trait true iff `_Type` supports binary `OP` between two instances
// of itself.
// (Was D_TRAIT_HAS_BINARY_OP.)
#define D_TYPE_TRAIT_HAS_BINARY_OP(TRAIT_NAME, OP)                           \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, D_TYPE_TRAIT_EXPR_BINARY_OP(OP))

// D_TYPE_TRAIT_HAS_UNARY_OP
//   macro: trait true iff `_Type` supports prefix unary `OP`.
// (Was D_TRAIT_HAS_UNARY_OP.)
#define D_TYPE_TRAIT_HAS_UNARY_OP(TRAIT_NAME, OP)                            \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, D_TYPE_TRAIT_EXPR_UNARY_OP(OP))


// ===========================================================================
//  "is a specialization of X" family  (partial specialization, not SFINAE)
// ===========================================================================
//   A DIFFERENT mechanism from the engine above: it dispatches via partial
// specialization on a known class template rather than by substituting an
// expression.  Useful for "is this a foo<...>?" where there is no expression
// to probe.

// D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS
//   macro: core for the "is_X<Y>" partial-specialization pattern.  Emits the
// primary template (std::false_type) and a specialization on
// `TEMPLATE_NAME<_Types...>` whose base is INHERIT_EXPR (may reference the
// deduced pack `_Types...`).
//
//   Limitation: TEMPLATE_NAME must be a class template whose parameters are
// all typename-kind.  Templates with non-type or template-template parameters
// (e.g. std::array<T, N>) cannot be matched.
// (Was D_TRAIT_IS_SPECIALIZATION_OF_AS.)
#define D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS(TRAIT_NAME,                      \
                                             TEMPLATE_NAME,                  \
                                             INHERIT_EXPR)                   \
    template<typename _Type>                                                 \
    struct TRAIT_NAME : std::false_type                                      \
    {};                                                                      \
                                                                             \
    template<typename... _Types>                                             \
    struct TRAIT_NAME<TEMPLATE_NAME<_Types...>> : INHERIT_EXPR               \
    {};

// D_TYPE_TRAIT_IS_SPECIALIZATION_OF
//   macro: one-line "is this a TEMPLATE_NAME<...>?" trait - std::true_type in
// the matching specialization, plus the `_v` companion.
// (Was D_TRAIT_IS_SPECIALIZATION_OF.)
#define D_TYPE_TRAIT_IS_SPECIALIZATION_OF(TRAIT_NAME, TEMPLATE_NAME)          \
    D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS(TRAIT_NAME,                          \
                                         TEMPLATE_NAME,                      \
                                         std::true_type)                    \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)


// ===========================================================================
//  member-typedef extraction  (extract or fall back)
// ===========================================================================

// D_TYPE_TRAIT_MEMBER_TYPE_OR
//   macro: define a SFINAE-safe extractor `TRAIT` yielding
// `clean_t<_Type>::MEMBER` when present and FALLBACK otherwise, plus its
// `TRAIT##_t` alias.  Pass `void` for FALLBACK to reproduce the historical
// "produce void on absence" behavior.
//
//   Must be invoked inside the djinterp namespace: it opens an adjacent
// `internal` namespace for its helper.
// (Was member_traits' D_DEFINE_MEMBER_TYPE_OR.)
#define D_TYPE_TRAIT_MEMBER_TYPE_OR(TRAIT, MEMBER, FALLBACK)                  \
    NS_INTERNAL                                                              \
                                                                             \
        /* TRAIT##_helper                                                 */ \
        /*   trait: primary template (produces the fallback type).        */ \
        template<typename _Type,                                             \
                 typename = void>                                            \
        struct TRAIT##_helper                                                \
        {                                                                    \
            using type = FALLBACK;                                           \
        };                                                                   \
                                                                             \
        /* TRAIT##_helper (success case)                                  */ \
        /*   trait: extracts clean_t<_Type>::MEMBER when available.       */ \
        template<typename _Type>                                             \
        struct TRAIT##_helper<_Type,                                         \
            D_VOID_T<typename clean_t<_Type>::MEMBER>>                       \
        {                                                                    \
            using type = typename clean_t<_Type>::MEMBER;                    \
        };                                                                   \
                                                                             \
    NS_END  /* internal */                                                   \
                                                                             \
    /* TRAIT                                                              */ \
    /*   trait: SFINAE-safe extraction of MEMBER (or fallback).           */ \
    template<typename _Type>                                                 \
    struct TRAIT : internal::TRAIT##_helper<_Type>                           \
    {};                                                                      \
                                                                             \
    /* TRAIT##_t                                                          */ \
    /*   type: convenience alias for TRAIT<_Type>::type.                  */ \
    template<typename _Type>                                                 \
    using TRAIT##_t = typename TRAIT<_Type>::type;


#endif  // DJINTERP_META_TRAIT_DETECT_
