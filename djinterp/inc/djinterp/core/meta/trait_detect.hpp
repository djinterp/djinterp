/*******************************************************************************
* djinterp [core]                                               trait_detect.hpp
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
*   Two engines, one meaning (decision 4.8). From C++11 every macro here is
* the void_t engine above. At C++98 the purpose-named shapes whose argument is
* a NAME or an OPERATOR -- D_TYPE_TRAIT_HAS_TYPE, _HAS_STATIC_MEMBER,
* _HAS_METHOD, _HAS_BINARY_OP, _HAS_UNARY_OP -- are emitted by a sizeof
* engine instead: each probe expression sits inside sizeof, so a failed
* substitution removes an overload (expression SFINAE, which GCC and Clang
* apply in C++98 mode). Their traits have the same `::value` for the same
* type at every level; tests/djinterp/core/meta/trait_detect_parity.cpp checks
* that at every rung. What takes a type EXPRESSION (the engine, the decltype
* builders, the variadic shapes) needs decltype and is absent below C++11,
* as are D_TYPE_TRAIT_IS_SPECIALIZATION_OF (variadic templates) and
* D_TYPE_TRAIT_MEMBER_TYPE_OR (an alias template).
*   The `_v` companion degrades with the language: an inline variable
* template on C++17+, a (non-inline) variable template on C++14, and nothing
* below (where variable templates do not exist) - the `::value` member is
* always present, only the `_v` shorthand is conditional.
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
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_META_TRAIT_DETECT_HPP
#define DJINTERP_META_TRAIT_DETECT_HPP 1

// djinterp
#include "../../djinterp.hpp"   // NS_*, D_ENV_* feature macros
#include "./type_utility.hpp"   // void_t, clean, clean_t
// std
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>      // std::true_type, std::false_type, ...
    #include <utility>          // std::declval
#endif


// ===========================================================================
//  the C++98 sizeof engine
// ===========================================================================
//   Below C++11 there is no decltype, so a probe is an EXPRESSION inside sizeof
// rather than a type. Defined at every level, so the parity test can set it
// beside the other engines; the public names select it below C++11: a trait's
// two overloads of d_probe differ in whether their parameter type can be
// formed, and sizeof of the chosen overload's result says which one won.

NS_DJINTERP
NS_INTERNAL

    // trait_detect_yes / trait_detect_no
    //   type: the two result sizes the engine tells apart.
    typedef char trait_detect_yes[1];
    typedef char trait_detect_no[2];

    // trait_detect_sink
    //   trait: turns a constant (a sizeof of a probe expression) into a type,
    // so a probe that cannot be formed removes its overload.
    template<int N>
    struct trait_detect_sink
    {
        typedef int type;
    };

    // trait_detect_type_sink
    //   trait: the same for a type, any type, references included.
    template<typename Type>
    struct trait_detect_type_sink
    {
        typedef int type;
    };

    // trait_detect_bool
    //   trait: the C++98 trait's base: a `value` usable in a constant
    // expression, as std::integral_constant's is.
    template<bool Value>
    struct trait_detect_bool
    {
        typedef bool value_type;

        static const bool value = Value;
    };

    // trait_detect_lval / trait_detect_rval
    //   function: an lvalue / a prvalue of any type, for unevaluated operands
    // only; declared, never defined (C++98's std::declval<T&>() and
    // std::declval<T>()).
    template<typename Type>
    Type& trait_detect_lval();
    template<typename Type>
    Type trait_detect_rval();

NS_END  // internal
NS_END  // djinterp

// D_INTERNAL_TYPE_TRAIT_98
//   macro (internal): emits TRAIT_NAME<Type>, whose value is true iff the
// parameter type PROBE (which names the overload's own parameter, Probe) can
// be formed for Probe = Type.
#define D_INTERNAL_TYPE_TRAIT_98(TRAIT_NAME, PROBE)                          \
    template<typename Type>                                                  \
    struct TRAIT_NAME                                                        \
    {                                                                        \
    private:                                                                 \
        template<typename Probe>                                             \
        static ::djinterp::internal::trait_detect_yes& d_probe(PROBE);       \
        template<typename Probe>                                             \
        static ::djinterp::internal::trait_detect_no&  d_probe(...);         \
                                                                             \
    public:                                                                  \
        typedef bool value_type;                                             \
                                                                             \
        static const bool value =                                            \
            ( sizeof(d_probe<Type>(0)) ==                                    \
              sizeof(::djinterp::internal::trait_detect_yes) );              \
    };

// D_INTERNAL_TYPE_TRAIT_98_EXPR
//   macro (internal): the parameter type that exists iff EXPR (which may
// mention Probe) is well-formed.
#define D_INTERNAL_TYPE_TRAIT_98_EXPR(EXPR)                                  \
    typename ::djinterp::internal::trait_detect_sink<sizeof((EXPR), 0)>::type

// D_INTERNAL_TYPE_TRAIT_98_LVALUE / D_INTERNAL_TYPE_TRAIT_98_CONST_LVALUE
//   macro (internal): an lvalue (a const lvalue) of Probe with any reference
// removed: what std::declval<Probe&>() (<const Probe&>) names once
// references collapse. Two macros, not one with an empty argument: ISO C++98
// has no empty macro arguments.
#define D_INTERNAL_TYPE_TRAIT_98_LVALUE                                      \
    ::djinterp::internal::trait_detect_lval<typename                         \
        ::djinterp::internal::clean_remove_reference<Probe>::type>()
#define D_INTERNAL_TYPE_TRAIT_98_CONST_LVALUE                                \
    ::djinterp::internal::trait_detect_lval<const typename                   \
        ::djinterp::internal::clean_remove_reference<Probe>::type>()

// D_INTERNAL_TYPE_TRAIT_98_HAS_TYPE
//   macro (internal): the C++98 engine's D_TYPE_TRAIT_HAS_TYPE: as above, from
// the sizeof engine: true iff clean<Type>::type has a nested type TYPE_NAME.
#define D_INTERNAL_TYPE_TRAIT_98_HAS_TYPE(TRAIT_NAME, TYPE_NAME)               \
    D_INTERNAL_TYPE_TRAIT_98(TRAIT_NAME,                                     \
        typename ::djinterp::internal::trait_detect_type_sink<               \
            typename ::djinterp::clean<Probe>::type::TYPE_NAME>::type)

// D_INTERNAL_TYPE_TRAIT_98_HAS_STATIC_MEMBER
//   macro (internal): the C++98 engine's D_TYPE_TRAIT_HAS_STATIC_MEMBER: true
// iff `Type::MEMBER_NAME` names a static member, data or function.
#define D_INTERNAL_TYPE_TRAIT_98_HAS_STATIC_MEMBER(TRAIT_NAME, MEMBER_NAME)    \
    D_INTERNAL_TYPE_TRAIT_98(TRAIT_NAME,                                     \
        D_INTERNAL_TYPE_TRAIT_98_EXPR(Probe::MEMBER_NAME))

// D_INTERNAL_TYPE_TRAIT_98_HAS_METHOD
//   macro (internal): the C++98 engine's D_TYPE_TRAIT_HAS_METHOD: true iff an
// lvalue of Type can call METHOD_NAME with a prvalue Type::value_type.
#define D_INTERNAL_TYPE_TRAIT_98_HAS_METHOD(TRAIT_NAME, METHOD_NAME)           \
    D_INTERNAL_TYPE_TRAIT_98(TRAIT_NAME,                                     \
        D_INTERNAL_TYPE_TRAIT_98_EXPR(                                       \
            ::djinterp::internal::trait_detect_lval<Probe>().METHOD_NAME(    \
                ::djinterp::internal::trait_detect_rval<                     \
                    typename Probe::value_type>())))

// D_INTERNAL_TYPE_TRAIT_98_HAS_BINARY_OP
//   macro (internal): the C++98 engine's D_TYPE_TRAIT_HAS_BINARY_OP: true iff
// two const lvalues of Type support binary OP.
#define D_INTERNAL_TYPE_TRAIT_98_HAS_BINARY_OP(TRAIT_NAME, OP)                 \
    D_INTERNAL_TYPE_TRAIT_98(TRAIT_NAME,                                     \
        D_INTERNAL_TYPE_TRAIT_98_EXPR(D_INTERNAL_TYPE_TRAIT_98_CONST_LVALUE \
                                      OP                                     \
                                      D_INTERNAL_TYPE_TRAIT_98_CONST_LVALUE))

// D_INTERNAL_TYPE_TRAIT_98_HAS_UNARY_OP
//   macro (internal): the C++98 engine's D_TYPE_TRAIT_HAS_UNARY_OP: true iff an
// lvalue of Type supports prefix OP.
#define D_INTERNAL_TYPE_TRAIT_98_HAS_UNARY_OP(TRAIT_NAME, OP)                  \
    D_INTERNAL_TYPE_TRAIT_98(TRAIT_NAME,                                     \
        D_INTERNAL_TYPE_TRAIT_98_EXPR(OP D_INTERNAL_TYPE_TRAIT_98_LVALUE))


// everything from here to the engine selection needs decltype: C++11 and up
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


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
        template<typename Type>                                             \
        inline constexpr bool TRAIT_NAME##_v = TRAIT_NAME<Type>::value;
#elif D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    #define D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)                               \
        template<typename Type>                                             \
        constexpr bool TRAIT_NAME##_v = TRAIT_NAME<Type>::value;
#else
    #define D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)  /* no variable templates */
#endif

// D_TYPE_TRAIT_TYPE_ALIAS
//   macro: emit the `TRAIT_NAME##_t` companion of a unary trait exposing
// `::type`.  The `_t` counterpart to D_TYPE_TRAIT_VALUE_BOOL.
#define D_TYPE_TRAIT_TYPE_ALIAS(TRAIT_NAME)                                   \
    template<typename Type>                                                 \
    using TRAIT_NAME##_t = typename TRAIT_NAME<Type>::type;


// ===========================================================================
//  the SFINAE bool-trait engine
// ===========================================================================

// D_TYPE_TRAIT_TRUE_AS
//   macro: parameterized core of the detection family.  Emits the primary
// template (std::false_type) plus the well-formed partial specialization
// whose base is INHERIT_EXPR (an arbitrary type expression that may mention
// `Type`).  Does NOT emit the `_v` companion - the caller, or one of the
// shorthands below, adds it via D_TYPE_TRAIT_VALUE_BOOL.
// (Was D_TRAIT_IS_DETECTED_AS.)
#define D_TYPE_TRAIT_TRUE_AS(TRAIT_NAME, DETECTION_EXPR, INHERIT_EXPR)        \
    template<typename Type,                                                 \
             typename = void>                                                \
    struct TRAIT_NAME : std::false_type                                      \
    {};                                                                      \
                                                                             \
    template<typename Type>                                                 \
    struct TRAIT_NAME<Type, D_VOID_T<DETECTION_EXPR>> : INHERIT_EXPR        \
    {};

// D_TYPE_TRAIT_TRUE
//   macro: the bread-and-butter SFINAE trait.  Emits `TRAIT_NAME<Type>`,
// std::true_type when every detection expression in `...` is well-formed for
// Type and std::false_type otherwise, plus the `_v` companion.  Variadic:
// pass one expression for a simple check, or several to require all of them
// (the AND-shape of the void_t idiom).
// (Was D_TRAIT_IS_DETECTED; documented in containers_howto as the canonical
// SFINAE-bool-trait macro.)
#define D_TYPE_TRAIT_TRUE(TRAIT_NAME, ...)                                    \
    template<typename Type,                                                 \
             typename = void>                                                \
    struct TRAIT_NAME : std::false_type                                      \
    {};                                                                      \
                                                                             \
    template<typename Type>                                                 \
    struct TRAIT_NAME<Type, D_VOID_T<__VA_ARGS__>> : std::true_type         \
    {};                                                                      \
                                                                             \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)

// D_TYPE_TRAIT_TRUE_FROM
//   macro: like D_TYPE_TRAIT_TRUE, but on success inherits from
// `BASE_TRAIT<Type>` instead of std::true_type, so the trait can delegate
// further checks to another unary trait.
// (Was D_TRAIT_IS_DETECTED_FROM.)
#define D_TYPE_TRAIT_TRUE_FROM(TRAIT_NAME, DETECTION_EXPR, BASE_TRAIT)        \
    D_TYPE_TRAIT_TRUE_AS(TRAIT_NAME, DETECTION_EXPR, BASE_TRAIT<Type>)       \
    D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)


// ===========================================================================
//  decltype-expression builders  (feed these to the engine)
// ===========================================================================

// D_TYPE_TRAIT_EXPR_METHOD
//   macro: a decltype-expression detecting a call to
// `Type::METHOD_NAME(Type::value_type{})`.  Pass as the DETECTION_EXPR
// argument of a D_TYPE_TRAIT_TRUE* macro.
// (Was D_TRAIT_DETECT_METHOD.)
#define D_TYPE_TRAIT_EXPR_METHOD(METHOD_NAME)                                 \
    decltype(std::declval<Type&>().METHOD_NAME(                             \
        std::declval<typename Type::value_type>()))

// D_TYPE_TRAIT_EXPR_METHOD_ARGS
//   macro: a decltype-expression detecting a call to `Type::METHOD_NAME(...)`
// with arguments of the variadic types supplied.
// (Was D_TRAIT_DETECT_METHOD_ARGS.)
#define D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, ...)                       \
    decltype(std::declval<Type&>().METHOD_NAME(                             \
        std::declval<__VA_ARGS__>()))

// D_TYPE_TRAIT_EXPR_BINARY_OP
//   macro: a decltype-expression detecting `Type OP Type`.  Operands are
// `const Type&` so the probe accepts immutable operands and is not defeated
// by const-qualified types.
// (Was D_TRAIT_DETECT_BINARY_OP.)
#define D_TYPE_TRAIT_EXPR_BINARY_OP(OP)                                       \
    decltype(std::declval<const Type&>() OP std::declval<const Type&>())

// D_TYPE_TRAIT_EXPR_UNARY_OP
//   macro: a decltype-expression detecting prefix `OP Type` (`-x`, `*x`,
// `!x`, `++x`, ...).
// (Was D_TRAIT_DETECT_UNARY_OP.)
#define D_TYPE_TRAIT_EXPR_UNARY_OP(OP)                                        \
    decltype(OP std::declval<Type&>())


// ===========================================================================
//  purpose-named sugar
// ===========================================================================
//   Each macro bakes a concrete DETECTION_EXPR (or success-base) into the
// engine.  That baked content is the resolution difference that earns the
// macro its place - none of these is a rename of the engine.

// D_TYPE_TRAIT_HAS_TYPE
//   macro: trait true iff `Type` exposes a nested typedef `TYPE_NAME`.  The
// probe strips cv-ref via `clean_t` first, so `has_X<const T&>` agrees with
// `has_X<T>` - this is the behavior the old member_traits detector had, now
// the single canonical one.  (Subsumes member_traits' D_DEFINE_HAS_MEMBER_TYPE
// and the cv-naive D_TRAIT_HAS_TYPE.)
// D_TYPE_TRAIT_DETECTED
//   macro: the general detector -- emits the trait AND its _v companion.
//
//   THIS MACRO WAS NEVER DEFINED.  Thirty-one call sites across four headers
// use it (threadsafe_container_traits 13, concurrency_strategy_traits 9,
// flat_iterator_traits 5, hierarchical_iterator_traits 4), and every one of
// those headers therefore fails to compile:
//
//     error: expected constructor, destructor, or type conversion
//     error: 'has_lock_policy_type_v' was not declared in this scope
//
// The callers pass a full type-or-expression, not a bare member name (which is
// what D_TYPE_TRAIT_HAS_TYPE takes).  That is exactly D_TYPE_TRAIT_TRUE, which
// already emits the _v companion the callers use.  DETECTED is its old spelling.
#define D_TYPE_TRAIT_DETECTED(TRAIT_NAME, ...)                                \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, __VA_ARGS__)

#define D_INTERNAL_TYPE_TRAIT_11_HAS_TYPE(TRAIT_NAME, TYPE_NAME)               \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, typename clean_t<Type>::TYPE_NAME)

// D_TYPE_TRAIT_HAS_STATIC_MEMBER
//   macro: trait true iff `decltype(Type::MEMBER_NAME)` is well-formed (the
// name exists at class scope).  Does NOT constrain the member's type; combine
// with the multi-expression form of D_TYPE_TRAIT_TRUE for that.
// (Was D_TRAIT_HAS_STATIC_MEMBER.)
#define D_INTERNAL_TYPE_TRAIT_11_HAS_STATIC_MEMBER(TRAIT_NAME, MEMBER_NAME)    \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, decltype(Type::MEMBER_NAME))

// D_TYPE_TRAIT_HAS_METHOD
//   macro: trait true iff `Type` has `METHOD_NAME` callable with a single
// `Type::value_type` argument.
// (Was D_TRAIT_HAS_METHOD.)
#define D_INTERNAL_TYPE_TRAIT_11_HAS_METHOD(TRAIT_NAME, METHOD_NAME)           \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, D_TYPE_TRAIT_EXPR_METHOD(METHOD_NAME))

// D_TYPE_TRAIT_HAS_METHOD_ARGS
//   macro: trait true iff `Type` has `METHOD_NAME` callable with arguments
// of the variadic types supplied.
// (Was D_TRAIT_HAS_METHOD_ARGS.)
#define D_TYPE_TRAIT_HAS_METHOD_ARGS(TRAIT_NAME, METHOD_NAME, ...)            \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME,                                            \
        D_TYPE_TRAIT_EXPR_METHOD_ARGS(METHOD_NAME, __VA_ARGS__))

// D_TYPE_TRAIT_HAS_METHOD_TYPED
//   macro: trait true iff `Type` has `METHOD_NAME` callable with the given
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
//   macro: trait true iff `Type` supports binary `OP` between two instances
// of itself.
// (Was D_TRAIT_HAS_BINARY_OP.)
#define D_INTERNAL_TYPE_TRAIT_11_HAS_BINARY_OP(TRAIT_NAME, OP)                 \
    D_TYPE_TRAIT_TRUE(TRAIT_NAME, D_TYPE_TRAIT_EXPR_BINARY_OP(OP))

// D_TYPE_TRAIT_HAS_UNARY_OP
//   macro: trait true iff `Type` supports prefix unary `OP`.
// (Was D_TRAIT_HAS_UNARY_OP.)
#define D_INTERNAL_TYPE_TRAIT_11_HAS_UNARY_OP(TRAIT_NAME, OP)                  \
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
// `TEMPLATE_NAME<Types...>` whose base is INHERIT_EXPR (may reference the
// deduced pack `Types...`).
//
//   Limitation: TEMPLATE_NAME must be a class template whose parameters are
// all typename-kind.  Templates with non-type or template-template parameters
// (e.g. std::array<T, N>) cannot be matched.
// (Was D_TRAIT_IS_SPECIALIZATION_OF_AS.)
#define D_TYPE_TRAIT_IS_SPECIALIZATION_OF_AS(TRAIT_NAME,                      \
                                             TEMPLATE_NAME,                  \
                                             INHERIT_EXPR)                   \
    template<typename Type>                                                 \
    struct TRAIT_NAME : std::false_type                                      \
    {};                                                                      \
                                                                             \
    template<typename... Types>                                             \
    struct TRAIT_NAME<TEMPLATE_NAME<Types...>> : INHERIT_EXPR               \
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
// `clean_t<Type>::MEMBER` when present and FALLBACK otherwise, plus its
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
        template<typename Type,                                             \
                 typename = void>                                            \
        struct TRAIT##_helper                                                \
        {                                                                    \
            using type = FALLBACK;                                           \
        };                                                                   \
                                                                             \
        /* TRAIT##_helper (success case)                                  */ \
        /*   trait: extracts clean_t<Type>::MEMBER when available.       */ \
        template<typename Type>                                             \
        struct TRAIT##_helper<Type,                                         \
            D_VOID_T<typename clean_t<Type>::MEMBER>>                       \
        {                                                                    \
            using type = typename clean_t<Type>::MEMBER;                    \
        };                                                                   \
                                                                             \
    NS_END  /* internal */                                                   \
                                                                             \
    /* TRAIT                                                              */ \
    /*   trait: SFINAE-safe extraction of MEMBER (or fallback).           */ \
    template<typename Type>                                                 \
    struct TRAIT : internal::TRAIT##_helper<Type>                           \
    {};                                                                      \
                                                                             \
    /* TRAIT##_t                                                          */ \
    /*   type: convenience alias for TRAIT<Type>::type.                  */ \
    template<typename Type>                                                 \
    using TRAIT##_t = typename TRAIT<Type>::type;


// ===========================================================================
//  the C++20 requires engine, and the engine selection
// ===========================================================================

// D_INTERNAL_TYPE_TRAIT_20
//   macro (internal): emits TRAIT_NAME<Type> (the same two-parameter shape
// as the void_t engine's), a std::bool_constant of a requires-expression over
// REQUIREMENTS, plus the `_v` companion: the trait defined from the
// constraint, as decision 4.8 has it. C++20 with concepts.
#if ( (D_ENV_LANG_IS_CPP20_OR_HIGHER) &&                                     \
      (D_ENV_CPP_FEATURE_LANG_CONCEPTS) )
    #define D_INTERNAL_TYPE_TRAIT_20(TRAIT_NAME, REQUIREMENTS)                \
        template<typename Type,                                             \
                 typename = void>                                            \
        struct TRAIT_NAME                                                    \
            : std::bool_constant<requires { REQUIREMENTS }>                  \
        {};                                                                  \
                                                                             \
        D_TYPE_TRAIT_VALUE_BOOL(TRAIT_NAME)

    // D_INTERNAL_TYPE_TRAIT_20_HAS_*
    //   macro (internal): the five named shapes, as requirements.
    #define D_INTERNAL_TYPE_TRAIT_20_HAS_TYPE(TRAIT_NAME, TYPE_NAME)          \
        D_INTERNAL_TYPE_TRAIT_20(TRAIT_NAME,                                 \
            typename ::djinterp::clean_t<Type>::TYPE_NAME;)
    #define D_INTERNAL_TYPE_TRAIT_20_HAS_STATIC_MEMBER(TRAIT_NAME,            \
                                                       MEMBER_NAME)           \
        D_INTERNAL_TYPE_TRAIT_20(TRAIT_NAME,                                 \
            typename ::djinterp::internal::trait_detect_type_sink<           \
                decltype(Type::MEMBER_NAME)>::type;)
    #define D_INTERNAL_TYPE_TRAIT_20_HAS_METHOD(TRAIT_NAME, METHOD_NAME)      \
        D_INTERNAL_TYPE_TRAIT_20(TRAIT_NAME,                                 \
            std::declval<Type&>().METHOD_NAME(                              \
                std::declval<typename Type::value_type>());)
    #define D_INTERNAL_TYPE_TRAIT_20_HAS_BINARY_OP(TRAIT_NAME, OP)           \
        D_INTERNAL_TYPE_TRAIT_20(TRAIT_NAME,                                 \
            std::declval<const Type&>() OP std::declval<const Type&>();)
    #define D_INTERNAL_TYPE_TRAIT_20_HAS_UNARY_OP(TRAIT_NAME, OP)            \
        D_INTERNAL_TYPE_TRAIT_20(TRAIT_NAME,                                 \
            OP std::declval<Type&>();)

    // D_TYPE_TRAIT_HAS_TYPE / _HAS_STATIC_MEMBER / _HAS_METHOD /
    // _HAS_BINARY_OP / _HAS_UNARY_OP
    //   macro: the named shapes, as documented above, from the requires
    // engine at C++20.
    #define D_TYPE_TRAIT_HAS_TYPE              D_INTERNAL_TYPE_TRAIT_20_HAS_TYPE
    #define D_TYPE_TRAIT_HAS_STATIC_MEMBER                                    \
        D_INTERNAL_TYPE_TRAIT_20_HAS_STATIC_MEMBER
    #define D_TYPE_TRAIT_HAS_METHOD                                            \
        D_INTERNAL_TYPE_TRAIT_20_HAS_METHOD
    #define D_TYPE_TRAIT_HAS_BINARY_OP                                        \
        D_INTERNAL_TYPE_TRAIT_20_HAS_BINARY_OP
    #define D_TYPE_TRAIT_HAS_UNARY_OP                                          \
        D_INTERNAL_TYPE_TRAIT_20_HAS_UNARY_OP
#else
    // ... and from the void_t engine at C++11 to C++17
    #define D_TYPE_TRAIT_HAS_TYPE              D_INTERNAL_TYPE_TRAIT_11_HAS_TYPE
    #define D_TYPE_TRAIT_HAS_STATIC_MEMBER                                    \
        D_INTERNAL_TYPE_TRAIT_11_HAS_STATIC_MEMBER
    #define D_TYPE_TRAIT_HAS_METHOD                                            \
        D_INTERNAL_TYPE_TRAIT_11_HAS_METHOD
    #define D_TYPE_TRAIT_HAS_BINARY_OP                                        \
        D_INTERNAL_TYPE_TRAIT_11_HAS_BINARY_OP
    #define D_TYPE_TRAIT_HAS_UNARY_OP                                          \
        D_INTERNAL_TYPE_TRAIT_11_HAS_UNARY_OP
#endif  // C++20 with concepts


#else  // below C++11: the named shapes from the sizeof engine

    #define D_TYPE_TRAIT_HAS_TYPE              D_INTERNAL_TYPE_TRAIT_98_HAS_TYPE
    #define D_TYPE_TRAIT_HAS_STATIC_MEMBER                                    \
        D_INTERNAL_TYPE_TRAIT_98_HAS_STATIC_MEMBER
    #define D_TYPE_TRAIT_HAS_METHOD                                            \
        D_INTERNAL_TYPE_TRAIT_98_HAS_METHOD
    #define D_TYPE_TRAIT_HAS_BINARY_OP                                        \
        D_INTERNAL_TYPE_TRAIT_98_HAS_BINARY_OP
    #define D_TYPE_TRAIT_HAS_UNARY_OP                                          \
        D_INTERNAL_TYPE_TRAIT_98_HAS_UNARY_OP

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_META_TRAIT_DETECT_HPP
