/******************************************************************************
* djinterp [test]                                                test_trait.hpp
*
*   DTest framework compile-time SFINAE and type-trait testing module.
* Provides a layered toolkit for asserting properties of type traits at
* compile time — detection of well-formed expressions, quantification
* over type packs, cv/ref invariance, logical relationships between
* traits, and trait/alias consistency — together with a runtime adapter
* that surfaces compile-time results through the test_object protocol.
*
*   DESIGN PHILOSOPHY:
*   Trait testing is a compile-time discipline.  Every primitive in this
* header reduces to a `static constexpr bool value` member.  The
* user-facing macros wrap those values in `static_assert`, so a failing
* trait test halts the build with a descriptive message rather than
* deferring failure to a runtime test pass.  The runtime adapter
* (Section X) exists only to make compile-time outcomes visible inside
* the framework's standard reporting flow — it does not re-evaluate.
*
*   LAYERS:
*     I.    portability gates
*     II.   detection idiom (portable is_detected family)
*     III.  quantified pack predicates (all_of, any_of, none_of,
*           count_of)
*     IV.   robustness predicates (cv / ref / cvref invariance)
*     V.    type equivalence helpers (with optional cv/ref stripping)
*     VI.   logical trait relationships (implies, equivalent_for,
*           disjoint_for)
*     VII.  pair / consistency predicates (alias_consistent)
*     VIII. compile-time test record + collection
*     IX.   static-assert macros
*     X.    runtime adapter (test_object protocol satisfier)
*     XI.   concept-aware helpers (C++20)
*
*   PORTABILITY:
*   C++11 minimum.  All standard-version gating routes through env.h
* and env_cpp_features.h:
*     D_ENV_LANG_IS_CPP11_OR_HIGHER       baseline requirement
*     D_ENV_LANG_IS_CPP14_OR_HIGHER       enables `_v` variable templates
*     D_ENV_LANG_IS_CPP17_OR_HIGHER       enables fold-expression bodies
*     D_ENV_CPP_FEATURE_LANG_CONCEPTS     enables Section XI
*     D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES   required
*
*   USAGE EXAMPLE:
*     // detect well-formed expressions:
*     template<typename _T>
*     using has_size = decltype(std::declval<_T&>().size());
*
*     D_TEST_TRAIT_DETECTED   (has_size, std::vector<int>);
*     D_TEST_TRAIT_NOT_DETECTED(has_size, int);
*
*     // quantify over packs:
*     D_TEST_TRAIT_ALL_OF (std::is_integral, int, char, long);
*     D_TEST_TRAIT_NONE_OF(std::is_pointer,  int, char, long);
*
*     // robustness:
*     D_TEST_TRAIT_CV_STABLE (std::is_integral, int);
*     D_TEST_TRAIT_REF_STABLE(std::is_integral, int);
*
*     // logical relationships:
*     D_TEST_TRAIT_IMPLIES   (std::is_integral, std::is_arithmetic,
*                             int, char, long);
*
*     // type-yielding alias consistency:
*     D_TEST_TRAIT_ALIAS_CONSISTENT(std::decay,    std::decay_t,    int&);
*
*
* TABLE OF CONTENTS
* =================
* I.    PORTABILITY GATES
* II.   DETECTION IDIOM
* III.  QUANTIFIED PACK PREDICATES
* IV.   ROBUSTNESS PREDICATES
* V.    TYPE EQUIVALENCE HELPERS
* VI.   LOGICAL TRAIT RELATIONSHIPS
* VII.  PAIR CONSISTENCY PREDICATES
* VIII. COMPILE-TIME TEST RECORD
* IX.   STATIC-ASSERT MACROS
* X.    RUNTIME ADAPTER
* XI.   CONCEPT-AWARE HELPERS (C++20)
*
*
* path:      /inc/djinterp/test/test_trait.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_TEST_TRAIT_
#define DJINTERP_TEST_TRAIT_ 1


// =========================================================================
// I.   PORTABILITY GATES
// =========================================================================

#ifndef DJINTERP_CPP_
    #error "test_trait.hpp requires djinterp.hpp to be included first"
#endif

#ifndef __cplusplus
    #error "test_trait.hpp can only be used in C++ compilation mode"
#endif

#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_trait.hpp requires C++11 or higher"
#endif

#if !D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES
    #error "test_trait.hpp requires variadic templates"
#endif


// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                II.  DETECTION IDIOM                                     ///
///////////////////////////////////////////////////////////////////////////////
// Portable equivalent of the C++ Library Fundamentals TS detection
// idiom (`std::experimental::is_detected`).  Lets users write a
// "probe" alias whose well-formedness is the trait being tested.

NS_INTERNAL

    // nonesuch
    //   type: placeholder representing "no such type" for the
    // detection idiom.
    struct nonesuch
    {
        nonesuch()                      = delete;
        ~nonesuch()                     = delete;
        nonesuch(const nonesuch&)       = delete;
        void operator=(const nonesuch&) = delete;
    };

    // detector
    //   trait: SFINAE detector primary template (failure case).
    template<typename                       _Default,
             typename                       _AlwaysVoid,
             template<typename...> class    _Op,
             typename...                    _Args>
    struct detector
    {
        using value_t = std::false_type;
        using type    = _Default;
    };

    // detector specialization
    //   trait: success case when _Op<_Args...> is well-formed.
    template<typename                    _Default,
             template<typename...> class _Op,
             typename...                 _Args>
    struct detector<_Default,
                    void_t<_Op<_Args...>>,
                    _Op,
                    _Args...>
    {
        using value_t = std::true_type;
        using type    = _Op<_Args...>;
    };

NS_END  // internal


// is_detected
//   trait: yields std::true_type if _Op<_Args...> is a
// well-formed type expression, std::false_type otherwise.
template<template<typename...> class _Op,
         typename...                 _Args>
using is_detected =
    typename internal::detector<internal::nonesuch,
                                void,
                                _Op,
                                _Args...>::value_t;

// detected_t
//   type: yields _Op<_Args...> when well-formed, otherwise
// internal::nonesuch.
template<template<typename...> class _Op,
         typename...                 _Args>
using detected_t =
    typename internal::detector<internal::nonesuch,
                                void,
                                _Op,
                                _Args...>::type;

// detected_or_t
//   type: yields _Op<_Args...> when well-formed, otherwise
// the supplied _Default type.
template<typename                    _Default,
         template<typename...> class _Op,
         typename...                 _Args>
using detected_or_t =
    typename internal::detector<_Default,
                                void,
                                _Op,
                                _Args...>::type;

// is_detected_exact
//   trait: true if _Op<_Args...> is well-formed AND yields
// exactly _Expected (no implicit conversion).
template<typename                    _Expected,
         template<typename...> class _Op,
         typename...                 _Args>
struct is_detected_exact
{
    static constexpr bool value =
        std::is_same<_Expected, detected_t<_Op, _Args...>>::value;
};

// is_detected_convertible
//   trait: true if _Op<_Args...> is well-formed AND its
// result is implicitly convertible to _Expected.
template<typename                    _Expected,
         template<typename...> class _Op,
         typename...                 _Args>
struct is_detected_convertible
{
    static constexpr bool value =
        std::is_convertible<detected_t<_Op, _Args...>,
                            _Expected>::value;
};


#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // is_detected_v
    //   value: convenience alias for is_detected<>::value.
    template<template<typename...> class _Op,
             typename...                 _Args>
    constexpr bool is_detected_v =
        is_detected<_Op, _Args...>::value;

    // is_detected_exact_v
    //   value: convenience alias for is_detected_exact<>::value.
    template<typename                    _Expected,
             template<typename...> class _Op,
             typename...                 _Args>
    constexpr bool is_detected_exact_v =
        is_detected_exact<_Expected, _Op, _Args...>::value;

    // is_detected_convertible_v
    //   value: convenience alias for
    // is_detected_convertible<>::value.
    template<typename                    _Expected,
             template<typename...> class _Op,
             typename...                 _Args>
    constexpr bool is_detected_convertible_v =
        is_detected_convertible<_Expected, _Op, _Args...>::value;

#endif  // C++14+


///////////////////////////////////////////////////////////////////////////////
///                III. QUANTIFIED PACK PREDICATES                          ///
///////////////////////////////////////////////////////////////////////////////
// Apply a unary or n-ary predicate across a pack of types.
// All four primitives reduce to `static constexpr bool value` so
// they compose with one another and with the macros in Section IX.

NS_INTERNAL

    // pack_count_helper
    //   trait: recursive count of types in the pack for which
    // _Predicate<_T>::value evaluates true.
    // primary template (base case).
    template<template<typename...> class _Predicate,
             typename...                 _Types>
    struct pack_count_helper
    {
        static constexpr std::size_t value = 0;
    };

    // pack_count_helper (recursive case)
    //   trait: peels one type off the front and recurses.
    template<template<typename...> class _Predicate,
             typename                    _Head,
             typename...                 _Tail>
    struct pack_count_helper<_Predicate, _Head, _Tail...>
    {
        static constexpr std::size_t value =
            ( (_Predicate<_Head>::value ? std::size_t(1) :
                                          std::size_t(0)) +
              pack_count_helper<_Predicate, _Tail...>::value );
    };

NS_END  // internal


// pack_count_of
//   trait: yields the number of types in the pack for which
// _Predicate<_T>::value evaluates true.
template<template<typename...> class _Predicate,
         typename...                 _Types>
struct pack_count_of
{
    static constexpr std::size_t value =
        internal::pack_count_helper<_Predicate, _Types...>::value;
};

// pack_all_of
//   trait: true if _Predicate<_T>::value is true for every
// type in the pack (vacuously true for the empty pack).
template<template<typename...> class _Predicate,
         typename...                 _Types>
struct pack_all_of
{
    static constexpr bool value =
        ( pack_count_of<_Predicate, _Types...>::value ==
          sizeof...(_Types) );
};

// pack_any_of
//   trait: true if _Predicate<_T>::value is true for at
// least one type in the pack.
template<template<typename...> class _Predicate,
         typename...                 _Types>
struct pack_any_of
{
    static constexpr bool value =
        ( pack_count_of<_Predicate, _Types...>::value > 0 );
};

// pack_none_of
//   trait: true if _Predicate<_T>::value is false for every
// type in the pack (vacuously true for the empty pack).
template<template<typename...> class _Predicate,
         typename...                 _Types>
struct pack_none_of
{
    static constexpr bool value =
        ( pack_count_of<_Predicate, _Types...>::value == 0 );
};

// pack_exactly_n_of
//   trait: true if _Predicate<_T>::value is true for exactly
// _N types in the pack.
template<std::size_t                 _N,
         template<typename...> class _Predicate,
         typename...                 _Types>
struct pack_exactly_n_of
{
    static constexpr bool value =
        ( pack_count_of<_Predicate, _Types...>::value == _N );
};


#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // pack_all_of_v / pack_any_of_v / pack_none_of_v /
    // pack_count_of_v
    //   value: convenience aliases for the value member.
    template<template<typename...> class _Predicate,
             typename...                 _Types>
    constexpr bool pack_all_of_v =
        pack_all_of<_Predicate, _Types...>::value;

    template<template<typename...> class _Predicate,
             typename...                 _Types>
    constexpr bool pack_any_of_v =
        pack_any_of<_Predicate, _Types...>::value;

    template<template<typename...> class _Predicate,
             typename...                 _Types>
    constexpr bool pack_none_of_v =
        pack_none_of<_Predicate, _Types...>::value;

    template<template<typename...> class _Predicate,
             typename...                 _Types>
    constexpr std::size_t pack_count_of_v =
        pack_count_of<_Predicate, _Types...>::value;

#endif  // C++14+


///////////////////////////////////////////////////////////////////////////////
///                IV.  ROBUSTNESS PREDICATES                               ///
///////////////////////////////////////////////////////////////////////////////
// Verify that a trait yields the same value across cv-qualified,
// reference-qualified, and combined variants of a base type.  These
// catch regressions where a trait specialization forgets to handle
// `const T`, `T&`, or `const T&&`.

// trait_cv_stable
//   trait: true if _Predicate yields the same value for
// _T, const _T, volatile _T, and const volatile _T.
template<template<typename...> class _Predicate,
         typename                    _T>
struct trait_cv_stable
{
private:
    static constexpr bool plain          = _Predicate<_T>::value;
    static constexpr bool with_const     =
        _Predicate<const _T>::value;
    static constexpr bool with_volatile  =
        _Predicate<volatile _T>::value;
    static constexpr bool with_cv        =
        _Predicate<const volatile _T>::value;

public:
    static constexpr bool value =
        ( (plain == with_const)    &&
          (plain == with_volatile) &&
          (plain == with_cv) );
};

// trait_ref_stable
//   trait: true if _Predicate yields the same value for
// _T, _T&, and _T&&.
template<template<typename...> class _Predicate,
         typename                    _T>
struct trait_ref_stable
{
private:
    static constexpr bool plain   = _Predicate<_T>::value;
    static constexpr bool lvalue  = _Predicate<_T&>::value;
    static constexpr bool rvalue  = _Predicate<_T&&>::value;

public:
    static constexpr bool value =
        ( (plain == lvalue) &&
          (plain == rvalue) );
};

// trait_cvref_stable
//   trait: true if _Predicate yields the same value for
// every cv- and reference-qualified variant of _T.
template<template<typename...> class _Predicate,
         typename                    _T>
struct trait_cvref_stable
{
    static constexpr bool value =
        ( trait_cv_stable<_Predicate, _T>::value  &&
          trait_ref_stable<_Predicate, _T>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TYPE EQUIVALENCE HELPERS                            ///
///////////////////////////////////////////////////////////////////////////////
// Compare two types with optional stripping of cv-qualifiers and
// references on either side.  Useful when a trait yields a decayed
// type and the test wants to ignore reference-binding nuances.

// type_equal
//   trait: true iff _A and _B are exactly the same type.
template<typename _A,
         typename _B>
struct type_equal
{
    static constexpr bool value = std::is_same<_A, _B>::value;
};

// type_equal_decayed
//   trait: true iff std::decay_t<_A> equals std::decay_t<_B>.
template<typename _A,
         typename _B>
struct type_equal_decayed
{
    static constexpr bool value =
        std::is_same<typename std::decay<_A>::type,
                     typename std::decay<_B>::type>::value;
};

// type_equal_cv_stripped
//   trait: true iff cv-stripped _A equals cv-stripped _B
// (preserves references).
template<typename _A,
         typename _B>
struct type_equal_cv_stripped
{
    static constexpr bool value =
        std::is_same<typename std::remove_cv<_A>::type,
                     typename std::remove_cv<_B>::type>::value;
};

// type_equal_clean
//   trait: true iff clean_t<_A> equals clean_t<_B>
// (cv- and reference-stripped — same notion as elsewhere
// in the framework).
template<typename _A,
         typename _B>
struct type_equal_clean
{
    static constexpr bool value =
        std::is_same<clean_t<_A>, clean_t<_B>>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  LOGICAL TRAIT RELATIONSHIPS                         ///
///////////////////////////////////////////////////////////////////////////////
// Quantify logical relationships between two predicates over a pack
// of types: implication, mutual equivalence, and disjointness.

NS_INTERNAL

    // implies_helper
    //   trait: per-type implication ((!P1 || P2) holds for _T).
    template<template<typename...> class _P1,
             template<typename...> class _P2,
             typename                    _T>
    struct implies_helper
    {
        static constexpr bool value =
            ( (!_P1<_T>::value) || _P2<_T>::value );
    };

    // equivalent_helper
    //   trait: per-type equivalence (P1 == P2 for _T).
    template<template<typename...> class _P1,
             template<typename...> class _P2,
             typename                    _T>
    struct equivalent_helper
    {
        static constexpr bool value =
            ( _P1<_T>::value == _P2<_T>::value );
    };

    // disjoint_helper
    //   trait: per-type disjointness (!(P1 && P2) for _T).
    template<template<typename...> class _P1,
             template<typename...> class _P2,
             typename                    _T>
    struct disjoint_helper
    {
        static constexpr bool value =
            ( !(_P1<_T>::value && _P2<_T>::value) );
    };

NS_END  // internal


// trait_implies_for
//   trait: true iff _P1<_T> implies _P2<_T> (i.e. whenever
// _P1<_T>::value is true, _P2<_T>::value is also true) for
// every type in the pack.
template<template<typename...> class _P1,
         template<typename...> class _P2,
         typename...                 _Types>
struct trait_implies_for
{
private:
    template<typename _T>
    using bound = internal::implies_helper<_P1, _P2, _T>;

public:
    static constexpr bool value =
        pack_all_of<bound, _Types...>::value;
};

// trait_equivalent_for
//   trait: true iff _P1<_T>::value == _P2<_T>::value for
// every type in the pack.
template<template<typename...> class _P1,
         template<typename...> class _P2,
         typename...                 _Types>
struct trait_equivalent_for
{
private:
    template<typename _T>
    using bound = internal::equivalent_helper<_P1, _P2, _T>;

public:
    static constexpr bool value =
        pack_all_of<bound, _Types...>::value;
};

// trait_disjoint_for
//   trait: true iff _P1<_T>::value and _P2<_T>::value are
// never both true for any type in the pack.
template<template<typename...> class _P1,
         template<typename...> class _P2,
         typename...                 _Types>
struct trait_disjoint_for
{
private:
    template<typename _T>
    using bound = internal::disjoint_helper<_P1, _P2, _T>;

public:
    static constexpr bool value =
        pack_all_of<bound, _Types...>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VII. PAIR CONSISTENCY PREDICATES                         ///
///////////////////////////////////////////////////////////////////////////////
// Verify that a `_t`-suffixed alias and its underlying
// `::type` member resolve to the same type.  Catches accidental
// drift between an alias and its source trait.

// alias_consistent
//   trait: true iff _Alias<_T> is the same type as
// _BaseTrait<_T>::type.
template<template<typename...> class _BaseTrait,
         template<typename...> class _Alias,
         typename                    _T>
struct alias_consistent
{
    static constexpr bool value =
        std::is_same<typename _BaseTrait<_T>::type,
                     _Alias<_T>>::value;
};

// alias_consistent_for
//   trait: true iff alias_consistent holds for every type
// in the pack.
template<template<typename...> class _BaseTrait,
         template<typename...> class _Alias,
         typename...                 _Types>
struct alias_consistent_for
{
private:
    template<typename _T>
    struct bound
    {
        static constexpr bool value =
            alias_consistent<_BaseTrait, _Alias, _T>::value;
    };

public:
    static constexpr bool value =
        pack_all_of<bound, _Types...>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                VIII. COMPILE-TIME TEST RECORD                           ///
///////////////////////////////////////////////////////////////////////////////
// Lightweight value-typed wrappers that capture a compile-time
// boolean result alongside an optional name.  Suites collect a pack
// of records; the runtime adapter in Section X reports their
// aggregate outcome through the test_object protocol.

// trait_record
//   struct: a single compile-time test outcome.  The result
// is fixed at the type level via _Result; the optional _Name
// pointer is supplied at construction for runtime reporting.
template<bool _Result>
struct trait_record
{
    static constexpr bool result = _Result;

    const char* name;

    D_CONSTEXPR trait_record() D_NOEXCEPT
        : name(nullptr)
    {}

    D_CONSTEXPR explicit trait_record(
        const char* _name
    ) D_NOEXCEPT
        : name(_name)
    {}

    // operator bool
    //   conversion: yields the compile-time result.  Satisfies
    // the test_object protocol's boolean conversion requirement.
    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
    {
        return _Result;
    }

    // status
    //   returns: passed when _Result is true, failed otherwise.
    D_CONSTEXPR test_status status() const D_NOEXCEPT
    {
        return _Result ? test_status::passed
                       : test_status::failed;
    }

    // is_leaf
    //   returns: true; trait records are always leaves.
    D_CONSTEXPR bool is_leaf() const D_NOEXCEPT
    {
        return true;
    }
};


NS_INTERNAL

    // count_passing_helper
    //   trait: recursive sum of `::result` across a pack of
    // trait_record types.  Primary template is the base case.
    template<typename... _Records>
    struct count_passing_helper
    {
        static constexpr std::size_t value = 0;
    };

    // count_passing_helper (recursive case)
    //   trait: peels one record off and recurses.
    template<typename    _Head,
             typename... _Tail>
    struct count_passing_helper<_Head, _Tail...>
    {
        static constexpr std::size_t value =
            ( (_Head::result ? std::size_t(1) :
                               std::size_t(0)) +
              count_passing_helper<_Tail...>::value );
    };

NS_END  // internal


// trait_suite
//   struct: a compile-time aggregate of trait_record types.
// `passed_count` counts records whose result is true; the
// suite "passes" iff every record passes (vacuously true for
// the empty suite).
template<typename... _Records>
struct trait_suite
{
    static constexpr std::size_t total =
        sizeof...(_Records);

    static constexpr std::size_t passed_count =
        internal::count_passing_helper<_Records...>::value;

    static constexpr std::size_t failed_count =
        (total - passed_count);

    static constexpr bool all_passed =
        (passed_count == total);
};


///////////////////////////////////////////////////////////////////////////////
///                IX.  STATIC-ASSERT MACROS                                ///
///////////////////////////////////////////////////////////////////////////////
// User-facing entry points.  Each macro reduces its arguments to a
// `static_assert`, halting the build with a descriptive message on
// failure.  Macros are designed to be usable inside namespace, class,
// and function scope alike.

// D_TEST_TRAIT_TRUE
//   macro: assert that _Trait<__VA_ARGS__>::value is true.
#define D_TEST_TRAIT_TRUE(_Trait, ...)                              \
    static_assert(                                                  \
        ((_Trait< __VA_ARGS__ >::value) == true),                   \
        "D_TEST_TRAIT_TRUE failed: "                                \
        #_Trait "<" #__VA_ARGS__ ">::value expected true")

// D_TEST_TRAIT_FALSE
//   macro: assert that _Trait<__VA_ARGS__>::value is false.
#define D_TEST_TRAIT_FALSE(_Trait, ...)                             \
    static_assert(                                                  \
        ((_Trait< __VA_ARGS__ >::value) == false),                  \
        "D_TEST_TRAIT_FALSE failed: "                               \
        #_Trait "<" #__VA_ARGS__ ">::value expected false")

// D_TEST_TYPE_EQ
//   macro: assert that _A and _B are exactly the same type.
#define D_TEST_TYPE_EQ(_A, _B)                                      \
    static_assert(                                                  \
        (::djinterp::test::type_equal< _A , _B >::value),           \
        "D_TEST_TYPE_EQ failed: "                                   \
        #_A " and " #_B " are not the same type")

// D_TEST_TYPE_NEQ
//   macro: assert that _A and _B are different types.
#define D_TEST_TYPE_NEQ(_A, _B)                                     \
    static_assert(                                                  \
        (!::djinterp::test::type_equal< _A , _B >::value),          \
        "D_TEST_TYPE_NEQ failed: "                                  \
        #_A " and " #_B " are unexpectedly the same type")

// D_TEST_TYPE_EQ_DECAYED
//   macro: assert that decayed forms of _A and _B agree.
#define D_TEST_TYPE_EQ_DECAYED(_A, _B)                              \
    static_assert(                                                  \
        (::djinterp::test::type_equal_decayed< _A , _B >::value),   \
        "D_TEST_TYPE_EQ_DECAYED failed: "                           \
        "decayed " #_A " and decayed " #_B " differ")

// D_TEST_TRAIT_DETECTED
//   macro: assert that _Op<__VA_ARGS__> is well-formed.
#define D_TEST_TRAIT_DETECTED(_Op, ...)                             \
    static_assert(                                                  \
        (::djinterp::test::is_detected<                             \
             _Op , __VA_ARGS__ >::value),                           \
        "D_TEST_TRAIT_DETECTED failed: "                            \
        #_Op "<" #__VA_ARGS__ "> not detected (expression "         \
        "is ill-formed)")

// D_TEST_TRAIT_NOT_DETECTED
//   macro: assert that _Op<__VA_ARGS__> is ill-formed.
#define D_TEST_TRAIT_NOT_DETECTED(_Op, ...)                         \
    static_assert(                                                  \
        (!::djinterp::test::is_detected<                            \
             _Op , __VA_ARGS__ >::value),                           \
        "D_TEST_TRAIT_NOT_DETECTED failed: "                        \
        #_Op "<" #__VA_ARGS__ "> unexpectedly detected")

// D_TEST_TRAIT_DETECTED_EXACT
//   macro: assert that _Op<__VA_ARGS__> resolves exactly to
// _Expected.
#define D_TEST_TRAIT_DETECTED_EXACT(_Expected, _Op, ...)            \
    static_assert(                                                  \
        (::djinterp::test::is_detected_exact<                       \
             _Expected , _Op , __VA_ARGS__ >::value),               \
        "D_TEST_TRAIT_DETECTED_EXACT failed: "                      \
        #_Op "<" #__VA_ARGS__ "> did not yield " #_Expected)

// D_TEST_TRAIT_ALL_OF
//   macro: assert that _Predicate is satisfied by every type
// in the pack.
#define D_TEST_TRAIT_ALL_OF(_Predicate, ...)                        \
    static_assert(                                                  \
        (::djinterp::test::pack_all_of<                             \
             _Predicate , __VA_ARGS__ >::value),                    \
        "D_TEST_TRAIT_ALL_OF failed: "                              \
        #_Predicate " is not satisfied by every type in {"          \
        #__VA_ARGS__ "}")

// D_TEST_TRAIT_ANY_OF
//   macro: assert that _Predicate is satisfied by at least one
// type in the pack.
#define D_TEST_TRAIT_ANY_OF(_Predicate, ...)                        \
    static_assert(                                                  \
        (::djinterp::test::pack_any_of<                             \
             _Predicate , __VA_ARGS__ >::value),                    \
        "D_TEST_TRAIT_ANY_OF failed: "                              \
        #_Predicate " is not satisfied by any type in {"            \
        #__VA_ARGS__ "}")

// D_TEST_TRAIT_NONE_OF
//   macro: assert that _Predicate is satisfied by no type in
// the pack.
#define D_TEST_TRAIT_NONE_OF(_Predicate, ...)                       \
    static_assert(                                                  \
        (::djinterp::test::pack_none_of<                            \
             _Predicate , __VA_ARGS__ >::value),                    \
        "D_TEST_TRAIT_NONE_OF failed: "                             \
        #_Predicate " is unexpectedly satisfied by some type "      \
        "in {" #__VA_ARGS__ "}")

// D_TEST_TRAIT_CV_STABLE
//   macro: assert that _Predicate is stable under
// cv-qualification of _T.
#define D_TEST_TRAIT_CV_STABLE(_Predicate, _T)                      \
    static_assert(                                                  \
        (::djinterp::test::trait_cv_stable<                         \
             _Predicate , _T >::value),                             \
        "D_TEST_TRAIT_CV_STABLE failed: "                           \
        #_Predicate " yields different results across cv "          \
        "variants of " #_T)

// D_TEST_TRAIT_REF_STABLE
//   macro: assert that _Predicate is stable under reference
// qualification of _T.
#define D_TEST_TRAIT_REF_STABLE(_Predicate, _T)                     \
    static_assert(                                                  \
        (::djinterp::test::trait_ref_stable<                        \
             _Predicate , _T >::value),                             \
        "D_TEST_TRAIT_REF_STABLE failed: "                          \
        #_Predicate " yields different results across "             \
        "reference variants of " #_T)

// D_TEST_TRAIT_CVREF_STABLE
//   macro: assert that _Predicate is stable under combined
// cv and reference qualification of _T.
#define D_TEST_TRAIT_CVREF_STABLE(_Predicate, _T)                   \
    static_assert(                                                  \
        (::djinterp::test::trait_cvref_stable<                      \
             _Predicate , _T >::value),                             \
        "D_TEST_TRAIT_CVREF_STABLE failed: "                        \
        #_Predicate " yields different results across cv/ref "      \
        "variants of " #_T)

// D_TEST_TRAIT_IMPLIES
//   macro: assert that _P1 implies _P2 for every type in the
// pack.  Use case: "is_integral implies is_arithmetic".
#define D_TEST_TRAIT_IMPLIES(_P1, _P2, ...)                         \
    static_assert(                                                  \
        (::djinterp::test::trait_implies_for<                       \
             _P1 , _P2 , __VA_ARGS__ >::value),                     \
        "D_TEST_TRAIT_IMPLIES failed: "                             \
        #_P1 " does not imply " #_P2 " over {" #__VA_ARGS__ "}")

// D_TEST_TRAIT_EQUIVALENT
//   macro: assert that _P1 and _P2 yield the same value for
// every type in the pack.
#define D_TEST_TRAIT_EQUIVALENT(_P1, _P2, ...)                      \
    static_assert(                                                  \
        (::djinterp::test::trait_equivalent_for<                    \
             _P1 , _P2 , __VA_ARGS__ >::value),                     \
        "D_TEST_TRAIT_EQUIVALENT failed: "                          \
        #_P1 " and " #_P2 " disagree over {" #__VA_ARGS__ "}")

// D_TEST_TRAIT_DISJOINT
//   macro: assert that _P1 and _P2 are never simultaneously
// true for any type in the pack.
#define D_TEST_TRAIT_DISJOINT(_P1, _P2, ...)                        \
    static_assert(                                                  \
        (::djinterp::test::trait_disjoint_for<                      \
             _P1 , _P2 , __VA_ARGS__ >::value),                     \
        "D_TEST_TRAIT_DISJOINT failed: "                            \
        #_P1 " and " #_P2 " overlap on some type in {"              \
        #__VA_ARGS__ "}")

// D_TEST_TRAIT_ALIAS_CONSISTENT
//   macro: assert that _Alias<_T> equals _BaseTrait<_T>::type.
#define D_TEST_TRAIT_ALIAS_CONSISTENT(_BaseTrait, _Alias, _T)       \
    static_assert(                                                  \
        (::djinterp::test::alias_consistent<                        \
             _BaseTrait , _Alias , _T >::value),                    \
        "D_TEST_TRAIT_ALIAS_CONSISTENT failed: "                    \
        #_Alias "<" #_T "> differs from "                           \
        #_BaseTrait "<" #_T ">::type")

// D_TEST_TRAIT_ALIAS_CONSISTENT_FOR
//   macro: assert alias_consistent for every type in the pack.
#define D_TEST_TRAIT_ALIAS_CONSISTENT_FOR(_BaseTrait, _Alias, ...)  \
    static_assert(                                                  \
        (::djinterp::test::alias_consistent_for<                    \
             _BaseTrait , _Alias , __VA_ARGS__ >::value),           \
        "D_TEST_TRAIT_ALIAS_CONSISTENT_FOR failed: "                \
        #_Alias " and " #_BaseTrait "::type disagree over {"        \
        #__VA_ARGS__ "}")


///////////////////////////////////////////////////////////////////////////////
///                X.   RUNTIME ADAPTER                                     ///
///////////////////////////////////////////////////////////////////////////////
// Bridge from a compile-time trait_suite into a runtime test_object
// that the framework's printer and event handler can walk.  The
// adapter does NOT re-evaluate any predicates — the suite's
// `passed_count` and `total` are already constants.

// trait_suite_object
//   class: test_object-protocol satisfier wrapping a
// compile-time trait_suite.  The status is determined entirely
// by the suite's `all_passed` constant; the adapter exists
// purely so existing report flows can surface compile-time
// outcomes without per-test runtime work.
template<typename _Suite>
class trait_suite_object
{
public:
    explicit trait_suite_object(
        const char* _name
    ) D_NOEXCEPT
        : m_name(_name)
    {}

    // operator bool
    //   conversion: true iff every record in the suite passed.
    D_CONSTEXPR explicit operator bool() const D_NOEXCEPT
    {
        return _Suite::all_passed;
    }

    // status
    //   returns: passed when the suite is all-green, failed
    // otherwise.
    D_CONSTEXPR test_status status() const D_NOEXCEPT
    {
        return _Suite::all_passed ? test_status::passed
                                  : test_status::failed;
    }

    // name
    //   returns: the human-readable name passed at construction.
    const char* name() const D_NOEXCEPT
    {
        return m_name;
    }

    // is_leaf
    //   returns: true; suite-objects are reported as leaves.
    D_CONSTEXPR bool is_leaf() const D_NOEXCEPT
    {
        return true;
    }

    // total
    //   returns: total number of records in the wrapped suite.
    D_CONSTEXPR std::size_t total() const D_NOEXCEPT
    {
        return _Suite::total;
    }

    // passed
    //   returns: number of passing records in the wrapped suite.
    D_CONSTEXPR std::size_t passed() const D_NOEXCEPT
    {
        return _Suite::passed_count;
    }

    // failed
    //   returns: number of failing records in the wrapped suite.
    D_CONSTEXPR std::size_t failed() const D_NOEXCEPT
    {
        return _Suite::failed_count;
    }

private:
    const char* m_name;
};


// make_trait_suite_object
//   factory: constructs a trait_suite_object<_Suite> with the
// supplied name.  Lets the suite type be deduced indirectly via
// an explicit template argument while keeping the call site
// compact.
template<typename _Suite>
trait_suite_object<_Suite>
make_trait_suite_object(
    const char* _name
) D_NOEXCEPT
{
    return trait_suite_object<_Suite>(_name);
}


///////////////////////////////////////////////////////////////////////////////
///                XI.  CONCEPT-AWARE HELPERS (C++20)                       ///
///////////////////////////////////////////////////////////////////////////////
// Concept assertion macros.  C++20 does not permit passing a concept
// as a template parameter, so these are pure macro entry points
// rather than wrapper traits.  For pack quantification over a
// concept, wrap the concept in a one-line unary trait struct and use
// the Section IX pack macros — example below the macros.
//
// When concepts are unavailable, this section is omitted entirely
// and users fall back to the predicate macros from Section IX
// (which operate uniformly across C++11+).

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// D_TEST_CONCEPT_TRUE
//   macro: assert that _Concept<__VA_ARGS__> is satisfied.
#define D_TEST_CONCEPT_TRUE(_Concept, ...)                          \
    static_assert(                                                  \
        (_Concept< __VA_ARGS__ >),                                  \
        "D_TEST_CONCEPT_TRUE failed: "                              \
        #_Concept "<" #__VA_ARGS__ "> is not satisfied")

// D_TEST_CONCEPT_FALSE
//   macro: assert that _Concept<__VA_ARGS__> is NOT satisfied.
#define D_TEST_CONCEPT_FALSE(_Concept, ...)                         \
    static_assert(                                                  \
        (!(_Concept< __VA_ARGS__ >)),                               \
        "D_TEST_CONCEPT_FALSE failed: "                             \
        #_Concept "<" #__VA_ARGS__ "> is unexpectedly satisfied")

// USAGE NOTE — concepts cannot be passed as template-template
// arguments in C++20, so pack quantification over a concept is done
// by wrapping the concept in a one-line unary trait struct and
// using the Section III/IV/VI combinators:
//
//     template<typename _T>
//     struct integral_pred {
//         static constexpr bool value = std::integral<_T>;
//     };
//
//     D_TEST_TRAIT_ALL_OF (integral_pred, int, char, long);
//     D_TEST_TRAIT_CV_STABLE(integral_pred, int);
//
// The wrapper costs one line and unlocks every Section III–VII
// combinator for the underlying concept.

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TRAIT_
