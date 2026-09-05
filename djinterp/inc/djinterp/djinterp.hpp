/******************************************************************************
* djinterp [root]                                                 djinterp.hpp
*
*   C++ core header for the djinterp framework. Extends the C core with
* namespace macros, constexpr support, and foundational type utilities
* including type cleaning, compile-time repetition, and self-referential
* type resolution.
*
*
* path:      /inc/djinterp/djinterp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2023.11.12
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    C++ KEYWORDS & NAMESPACE MACROS
      -------------------------------
      i.    C++ keywords
            a. D_KEYWORD_CPP
            b. D_KEYWORD_PARADIGM
            c. D_KEYWORD_RE_STD
            d. D_KEYWORD_STL
            e. D_KEYWORD_TRAITS
            f. D_KEYWORD_CONCEPTS
            g. D_KEYWORD_CONTAINER
      ii.   namespace macros
            a. D_NAMESPACE
            b. NS_END
            c. NS_CONCEPTS
            d. NS_CONTAINER
            e. NS_DATABASE
            f. NS_DJINTERP
            g. NS_ERROR
            h. NS_EXCEPTION
            i. NS_INTERNAL
            j. NS_MATH
            k. NS_MESSAGE
            l. NS_PARADIGM
            m. NS_RESTD
            n. NS_TEST
            o. NS_TESTING
            p. NS_TRAITS

II.   QUALIFIER SUPPORT
      -----------------
      a. D_CONSTEXPR
      b. D_STATIC_CONSTEXPR
      c. D_CONSTEXPR_INLINE
      d. D_STATIC_CONSTEXPR_INLINE
      e. D_INLINE_VAR
      f. D_CONSTEXPR_INLINE_VAR
      g. D_CONCEPT_PARAM
      h. D_NOEXCEPT
      i. D_NOEXCEPT_IF
      j. D_MOVE_ENABLED
      k. D_EXPLICIT_BOOL
      l. D_DELETED_FN

III.  CORE TYPE UTILITIES (namespace djinterp)
      ----------------------------------------
      i.    void_t
      ii.   abs_value
            a. abs_value_v
            b. abs_value_to_size_t
      iii.  clean
            a. clean_t
      iv.   constexpr_swap
      v.    repeat_type
            a. repeat_type_helper (internal)
            b. repeat_type_t
      vi.   self / resolve_self
            a. self
            b. is_self
            c. is_self<self>
            d. is_self_v
            e. resolve_self (+ specializations)
            f. resolve_self_t
*/

#ifndef DJINTERP_CPP_
#define DJINTERP_CPP_ 1


//   THE LANGUAGE FLOOR -- deliberately ahead of every #include, and reading
// __cplusplus raw. <initializer_list> below is a C++11 header, so on a C++98
// compiler libstdc++ diagnoses first ("this file requires ... ISO C++ 2011")
// and the message that says what to DO about it is never reached. A floor gate
// that depends on anything cannot be the first thing to fail, and being first
// is its only job.
//
//   Consequence for the tier policy: C++98 is not a supported tier, so a
// module must not carry a C++98 fallback "just in case" -- an unreachable
// branch is a second implementation nobody compiles. The kit below is the
// authority on what each tier spells; it no longer asks whether moves exist.
#if !defined(__cplusplus) ||                                                  \
    (__cplusplus < 201103L)
#   error "djinterp requires C++11 or later. Compile with -std=c++11 "      \\
        "(or newer), or /std:c++14 on MSVC."
#endif

// std
#include <cstddef>
#include <initializer_list>
#include <memory>
// djinterp
//   also pulls in env.h + env_attributes.h + env_vendor_attributes.h
#include "./c/djinterp.h"
#include "./env/cpp/env_cpp98.h"   // C++98 LIBRARY-header detection; still
                                   // relevant above the floor -- it answers
                                   // "does this implementation ship <map>",
                                   // not "is this C++98"
#include "./env/cpp/env_cpp_features.h"



// I.    C++ keywords & namespace macros

// I.i   C++ keywords

// D_KEYWORD_CPP
//   keyword: resolves to `cpp`.
// Used to specify that a unit of code pertains to the C++
// standard.
#define D_KEYWORD_CPP               cpp

// D_KEYWORD_PARADIGM
//   keyword: resolves to `paradigm`.
// Used to specify that a unit of code pertains to a pattern or software
// paradigm.
#define D_KEYWORD_PARADIGM          paradigm

// D_KEYWORD_RE_STD
//   keyword: resolves to `re_std`.
// Used to specify the re-std, a more-portable, backwards-compatible version
// of the std library.
#define D_KEYWORD_RE_STD            re_std

// D_KEYWORD_STL
//   keyword: resolves to `stl`.
// Used to specify that a unit of code pertains to the STL
// (Standard Template Library) part of the C++ standard.
#define D_KEYWORD_STL               stl

// D_KEYWORD_TRAITS
//   keyword: resolves to `traits`.
// Used to specify that a unit of code uses template
// metaprogramming and SFINAE for compile-time logic.
#define D_KEYWORD_TRAITS            traits

// D_KEYWORD_CONCEPTS
//   keyword: resolves to `concepts`.
// Used to specify that a unit of code provides C++20
// concept definitions.
#define D_KEYWORD_CONCEPTS          concepts

// D_KEYWORD_CONTAINER
//   keyword: resolves to `container`.
// Used to specify that a unit of code pertains to the
// container subsystem.
#define D_KEYWORD_CONTAINER         container


// I.ii  Namespace macros

// D_NAMESPACE
//   macro: wraps a block of code in a namespace with the name
// specified by parameter `NAME`.
#define D_NAMESPACE(NAME)           namespace NAME {

// NS_END
//   macro: namespace idiom; used to close any namespace.
#define NS_END                      };

// NS_CONCEPTS
//   namespace: the `concepts` namespace for C++20 concept
// definitions layered over a subsystem's trait surface.
#define NS_CONCEPTS                 D_NAMESPACE(D_KEYWORD_CONCEPTS)

// NS_CONTAINER
//   namespace: the `container` namespace for the container
// subsystem and its trait/concept surface.
#define NS_CONTAINER                D_NAMESPACE(D_KEYWORD_CONTAINER)

// NS_DATABASE
//   namespace: database namespace containing functionality pertaining to
// databases and database systems.
#define NS_DATABASE                 D_NAMESPACE(D_KEYWORD_DATABASE)

// NS_DJINTERP
//   namespace: the `djinterp` top-level namespace. Should be
// used as the top-level namespace of any and all modules
// within the djinterp tool-chain.
#define NS_DJINTERP                 D_NAMESPACE(D_KEYWORD_FRAMEWORK_NAME)

// NS_ERROR
//   namespace: the `error` namespace for error-handling types
// and utilities.
#define NS_ERROR                    D_NAMESPACE(D_KEYWORD_ERROR)

// NS_EXCEPTION
//   namespace: the `exception` namespace for severe
// error-handling types and utilities.
#define NS_EXCEPTION                D_NAMESPACE(D_KEYWORD_EXCEPTION)

// NS_INTERNAL
//   namespace: declares an `internal` namespace. Used to hide
// implementation details from regular use, such as "helper"
// types, structs and functions. Should be closed with `NS_END`.
#define NS_INTERNAL                 D_NAMESPACE(D_KEYWORD_INTERNAL)

// NS_MATH
//   namespace: the `math` namespace for mathematical
// utilities.
#define NS_MATH                     D_NAMESPACE(D_KEYWORD_MATH)

// NS_MESSAGE
//   namespace: the `message` namespace for variables, macros,
// and types that convey (usually string-based) human-readable
// information. Often (but not limited to) debugging and
// error-handling.
#define NS_MESSAGE                  D_NAMESPACE(D_KEYWORD_MESSAGE)

// NS_PARADIGM
//   namespace: resolves to `namespace paradigm {`
// This namespace contains common software patterns or idioms.
#define NS_PARADIGM                 D_NAMESPACE(D_KEYWORD_PARADIGM)

// NS_RESTD
//   keyword: the re-std namespace, containing more-portable,
// backwards-compatible versions of std library types, functions,
// and more.
#define NS_RESTD                    D_NAMESPACE(D_KEYWORD_RE_STD)

// NS_TEST
//   namespace: the `test` namespace for unit testing
// utilities.
#define NS_TEST                     D_NAMESPACE(D_KEYWORD_TEST)

// NS_TESTING
//   namespace: the `testing` namespace for holding unit tests.
#define NS_TESTING                  D_NAMESPACE(D_KEYWORD_TESTING)

// NS_TRAITS
//   namespace: the `traits` namespace for the compile-time
// trait surface of a subsystem (e.g. djinterp::container::traits).
#define NS_TRAITS                   D_NAMESPACE(D_KEYWORD_TRAITS)

// II.   Qualifier support
//   C++-only qualifiers: the constexpr family. The storage / inlining
// qualifiers (D_STATIC, D_INLINE, D_STATIC_INLINE) are defined ONCE in
// djinterp.h for both languages and inherited here -- not redefined, so there
// is no C vs. C++ drift. Gates come from qual_cfg.h (pulled in via djinterp.h).

// D_CONSTEXPR
//   qualifier: `constexpr` on C++11+. Expands to nothing when the config layer
// sets D_INTERNAL_QUAL_STRIP_CONSTEXPR (test instrumentation), or on pre-C++11.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR
        #if D_INTERNAL_QUAL_STRIP_CONSTEXPR
            #define D_CONSTEXPR             // stripped for test instrumentation
        #elif D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_CONSTEXPR             constexpr
        #else
            #define D_CONSTEXPR             // pre-C++11: no constexpr
        #endif
    #endif
#endif

// -- constexpr compound qualifiers (order: static constexpr inline) --
//   Composed from D_STATIC / D_INLINE (from djinterp.h) and D_CONSTEXPR above;
// each honors its own toggle. Valid in C++ because D_INLINE carries no
// `static` here. (The non-constexpr D_STATIC_INLINE lives in djinterp.h.)
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_STATIC_CONSTEXPR
        #define D_STATIC_CONSTEXPR          D_STATIC D_CONSTEXPR
    #endif
    #ifndef D_CONSTEXPR_INLINE
        #define D_CONSTEXPR_INLINE          D_CONSTEXPR D_INLINE
    #endif
    #ifndef D_STATIC_CONSTEXPR_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   D_STATIC D_CONSTEXPR D_INLINE
    #endif
#endif


// -- the VARIABLE half of the inline specifier ----------------------------
//   `inline` is two features wearing one keyword, and they do not share a
// floor:
//
//     inline FUNCTION  -- C++98. One merged definition of a function.
//     inline VARIABLE  -- C++17. One merged definition of an OBJECT.
//
//   D_INLINE is the FUNCTION spelling and says so; it also carries
// always_inline, which is a function attribute. Put it on a variable and both
// halves are wrong at once: below C++17 the `inline` declares an inline
// variable the tier cannot express (a -Wc++17-extensions error under
// -pedantic-errors), and at 17 and above the attribute is discarded with
// -Wattributes. Not a gap in D_INLINE -- the second feature simply had no
// spelling until now.

// D_INLINE_VAR
//   qualifier: one merged definition of a namespace-scope OBJECT across
// translation units. `inline` on C++17 and above; EMPTY below, and under a C
// compiler, so the spelling is safe to write unconditionally in a shared
// header exactly like D_EXTERN_C.
//
//   Empty is correct below 17 rather than a stand-in for one. A namespace-
// scope `const` / `constexpr` object already has INTERNAL linkage in C++, so
// dropping the `inline` yields one copy per TU that links cleanly and folds
// away. The only thing lost is address identity across TUs -- measured, not
// assumed: &k differs between two TUs at 11 and 14 and matches at 17.
//   A variable TEMPLATE never needed this; its instantiations merge under
// vague linkage on their own, which is why is_self_v below takes plain
// D_CONSTEXPR.
#if !defined(D_INLINE_VAR) && (D_INTERNAL_CFG_INLINE == 1)
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_INLINE_VAR                inline
    #else
        #define D_INLINE_VAR
    #endif
#endif

// D_CONSTEXPR_INLINE_VAR
//   qualifier: the variable counterpart of D_CONSTEXPR_INLINE -- `constexpr
// inline` on C++17 and above, `static constexpr` below. Component order
// follows the kit's rule that the NAME lists components in emission order, as
// D_CONSTEXPR_INLINE already does.
//
//   FOR NON-TEMPLATE header constants only -- option.hpp's arg_npos is the
// case. A variable TEMPLATE wants plain D_CONSTEXPR.
//
//   THE `static` BELOW 17 IS LOAD-BEARING, and is there because of a knob.
// The obvious spelling is `D_CONSTEXPR D_INLINE_VAR` at every tier, on the
// reasoning that a namespace-scope constexpr object already has internal
// linkage so the empty D_INLINE_VAR costs nothing. That reasoning holds only
// while the object is const -- and D_CFG_TESTING_STRIP_CONSTEXPR exists
// precisely to make D_CONSTEXPR expand to nothing. Under that knob, below 17,
// both halves vanish and `D_CONSTEXPR_INLINE_VAR int k = 7;` becomes a plain
// mutable `int k = 7;` at namespace scope in a header: external linkage, and
// a multiple-definition link error the moment a second translation unit
// includes it. Measured, not theorised -- two TUs at c++14 with
// -DD_CFG_TESTING_STRIP_CONSTEXPR=1 fail to link without this `static`.
//
//   It changes nothing when constexpr is NOT stripped: `static constexpr` and
// `constexpr` have identical linkage at namespace scope, so the address
// behaviour documented at D_INLINE_VAR is unaffected -- one copy per TU below
// 17, one merged object at 17 and above.
#if (D_INTERNAL_CFG_CONSTEXPR == 1) && (D_INTERNAL_CFG_INLINE == 1)
    #ifndef D_CONSTEXPR_INLINE_VAR
        #if D_ENV_LANG_IS_CPP17_OR_HIGHER
            #define D_CONSTEXPR_INLINE_VAR  D_CONSTEXPR D_INLINE_VAR
        #else
            #define D_CONSTEXPR_INLINE_VAR  D_STATIC D_CONSTEXPR
        #endif
    #endif
#endif


// -- constraining a template parameter ------------------------------------

// D_CONCEPT_PARAM(_Concept)
//   qualifier: writes a type-constraint on a template parameter where the
// tier supports one, and plain `typename` where it does not:
//
//     template<D_CONCEPT_PARAM(OverridePolicy) _Policy, typename _Set>
//
//   WHY THIS IS ADDITIVE AND NOT A SECOND IMPLEMENTATION. The body of the
// template is identical either way; only the constraint token changes. At
// C++20 the compiler rejects a bad argument at the point of use with the
// concept's name in the message; below, the same program compiles to the same
// thing and a bad argument is diagnosed later, from inside. That is a
// difference in DIAGNOSTIC QUALITY, which D1 permits a higher tier to add.
//
//   THE ONE CASE WHERE IT WOULD NOT BE. Constraints participate in overload
// resolution and partial ordering, so if two entry points were distinguished
// ONLY by their constraints, eliding one would silently change which is
// selected -- a behaviour change, and a D1 violation rather than an addition.
// Before using this macro, check that the constrained declarations are
// distinguished by their argument PATTERNS (as every current user is: alias
// templates, and class-template primary / partial-specialization pairs) and
// not by the constraint itself. Where they are not, the module states a floor
// instead.
//
//   Pair it with the trait behind the concept -- every concept in this
// framework is the PascalCase face of one -- so the contract can still be
// asserted below C++20 at whatever single point the parameter is actually
// consumed.
#ifndef D_CONCEPT_PARAM
    #if D_ENV_CPP_FEATURE_LANG_CONCEPTS
        #define D_CONCEPT_PARAM(_Concept)   _Concept
    #else
        #define D_CONCEPT_PARAM(_Concept)   typename
    #endif
#endif

// D_NOEXCEPT
//   Portable no-throw specifier.  In C++11 and later `noexcept` is a
//   first-class keyword; older C++ compilers accept the deprecated
//   `throw()` as a close equivalent.  In C mode the concept does not
//   exist, so the macro expands to nothing.
//
//   Resolution order:
//     1. C++11 - noexcept is standard.
//     2. Pre-C++11 C++ - throw() (deprecated but widely supported).
//     3. C / everything else - no-op fallback.
//
//   Pre-definable: users may #define D_NOEXCEPT before including this
//   header to override the detected value.
#ifndef D_NOEXCEPT
    #if defined(__cplusplus)
        #define D_NOEXCEPT noexcept      // C++11 floor; `throw()` is dead
    #else
        #define D_NOEXCEPT
    #endif
#endif  // D_NOEXCEPT

// D_NOEXCEPT_IF
//   conditional exception specification. Expands to noexcept(cond)
//   on C++11+. Pre-C++11 has no conditional spelling (throw() is
//   unconditional), so the condition is dropped to a no-op; in C
//   mode it is likewise a no-op.
//   Variadic so a condition containing a top-level comma (e.g. a
//   multi-arg trait) passes through intact.
#ifndef D_NOEXCEPT_IF
    #if defined(__cplusplus)
        #define D_NOEXCEPT_IF(...) noexcept(__VA_ARGS__)
    #else
        #define D_NOEXCEPT_IF(...)
    #endif
#endif  // D_NOEXCEPT_IF

// D_MOVE_ENABLED
//   move-support flag: 1 when the language has rvalue references, so a
//   move constructor and move assignment can be declared; 0 before
//   C++11 and in C mode, where a resource type is constructed in place
//   and passed by reference instead. Named for its use, so a resource
//   type reads `#if D_MOVE_ENABLED` rather than restating the standard
//   check at every ownership boundary.
//   Pre-definable: users may #define D_MOVE_ENABLED before including
//   this header to override the detected value.
#ifndef D_MOVE_ENABLED
    #if defined(__cplusplus)
        //   1 at the floor and above. It stays a KNOB rather than becoming a
        // constant because pre-defining it to 0 is still meaningful -- it asks
        // a type to be non-movable as well as non-copyable -- but it is no
        // longer a question about which standard is in use.
        #define D_MOVE_ENABLED 1
    #else
        #define D_MOVE_ENABLED 0
    #endif
#endif  // D_MOVE_ENABLED

// D_EXPLICIT_BOOL
//   explicit-conversion qualifier: `explicit` on a bool conversion
//   operator where the language allows it (C++11+), so an object does
//   not silently become an int in arithmetic. Empty before C++11 and in
//   C mode, where the named observer (is_open / valid / failed) is the
//   spelling to prefer for the same question.
//   Pre-definable.
#ifndef D_EXPLICIT_BOOL
    #if defined(__cplusplus)
        #define D_EXPLICIT_BOOL explicit  // C++11 floor; safe-bool is dead
    #else
        #define D_EXPLICIT_BOOL
    #endif
#endif  // D_EXPLICIT_BOOL

// D_DELETED_FN
//   deleted-function spelling: marks a member that must not exist,
//   taking the full declaration as its argument. On C++11+ it is
//   `= delete`, so a use is a compile error with a clear message; before
//   C++11 it is a bare declaration left undefined, which the caller
//   places in a private: section so a use fails at compile time (access)
//   or link time (missing symbol). Both make a type non-copyable; the
//   newer spelling just says so sooner and louder.
//
//   Usage (in a private: section):
//       D_DELETED_FN(my_type(const my_type&))
//       D_DELETED_FN(my_type& operator=(const my_type&))
//   Pre-definable.
#ifndef D_DELETED_FN
    #if defined(__cplusplus)
        //   `= delete` at the floor. The old fallback -- a private undefined
        // declaration -- diagnosed at LINK time instead of compile time, and
        // only if the program was linked at all. Not equivalent, and not kept.
        #define D_DELETED_FN(_decl) _decl = delete;
    #else
        #define D_DELETED_FN(_decl) _decl;
    #endif
#endif  // D_DELETED_FN


// III.  Core type utilities (namespace djinterp)

NS_DJINTERP

// void_t
//   type: maps any type sequence to void. Used as the SFINAE
// sink in detection idioms. Pre-C++17 replacement for
// std::void_t.
template<typename...>
using void_t = void;

// abs_value
//   trait: computes the absolute value of a compile-time
// integral constant.
template<typename _Type,
         _Type    _N>
struct abs_value
{
    static_assert(std::is_integral<_Type>::value,
                  "Type parameter `_Type` must be an integral type.");

    static constexpr _Type value = (_N < 0)
                                   ? -(_N)
                                   : _N;
};

// abs_value_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // abs_value_v
    //   value: convenience variable template for
    // abs_value<_Type, _N>::value.
    template<typename _Type,
             _Type    _N>
    constexpr _Type abs_value_v = abs_value<_Type, _N>::value;
#endif

// abs_value_to_size_t (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // abs_value_to_size_t
    //   value: convenience variable template that yields the
    // absolute value of _N as a std::size_t.
    template<typename _Type,
             _Type    _N>
    constexpr std::size_t abs_value_to_size_t =
        std::integral_constant<
            std::size_t,
            abs_value<_Type, _N>::value
        >::value;
#endif


// ================================================================
//  clean
// ================================================================

// clean
//   type modifier: strips cv-qualifiers and references.
//   Spelled through ::type rather than the _t aliases: std::remove_cv_t and
// std::remove_reference_t are C++14 library additions, and this header is
// included by every C++ module in the framework -- so using them here would
// make C++14 the floor for all of them.
template<typename _Type>
struct clean
{
    typedef typename std::remove_cv<
        typename std::remove_reference<_Type>::type>::type type;
};

// clean_t
//   type: convenience alias for clean<_Type>::type.
template<typename _Type>
//   The `typename` is required below C++20 -- P0634 made it optional in this
// position, so omitting it silently pins the header to C++20.
using clean_t = typename clean<_Type>::type;

// constexpr_swap
//   function: swaps two values in a constexpr context.
// std::swap is not constexpr until C++20, so this provides
// the same semantics for C++14 and C++17.
//
// C++20+  -- delegates to std::swap (already constexpr).
// C++14+  -- manual move-based swap under relaxed constexpr.
// C++11   -- identical body, but not constexpr (C++11 constexpr forbids local
//            variables and assignments).

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    template<typename _Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
        _Type& _a,
        _Type& _b
    )
    noexcept( std::is_nothrow_move_constructible<_Type>::value &&
              std::is_nothrow_move_assignable<_Type>::value )
    {
        std::swap(_a, _b);

        return;
    }

#elif D_ENV_LANG_IS_CPP14_OR_HIGHER

    template<typename _Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
        _Type& _a,
        _Type& _b
    )
    noexcept( std::is_nothrow_move_constructible<_Type>::value &&
              std::is_nothrow_move_assignable<_Type>::value )
    {
        _Type temp = static_cast<_Type&&>(_a);
        _a         = static_cast<_Type&&>(_b);
        _b         = static_cast<_Type&&>(temp);

        return;
    }

#else  // C++11

    template<typename _Type>
    D_INLINE void
    constexpr_swap(
        _Type& _a,
        _Type& _b
    )
    noexcept( std::is_nothrow_move_constructible<_Type>::value &&
              std::is_nothrow_move_assignable<_Type>::value )
    {
        _Type temp = static_cast<_Type&&>(_a);
        _a         = static_cast<_Type&&>(_b);
        _b         = static_cast<_Type&&>(temp);

        return;
    }

#endif

// ================================================================
//  repeat_type
// ================================================================

NS_INTERNAL
    // repeat_type_helper
    //   trait: internal recursive builder for repeat_type<>
    // (general case).
    template<typename    _Type,
             std::size_t _N,
             typename... _Types>
    struct repeat_type_helper
    {
        using type = typename repeat_type_helper<_Type,
                                                 (_N - 1),
                                                 _Type,
                                                 _Types...>::type;
    };

    // repeat_type_helper<_Type, 0, _Types...>
    //   trait: base case specialization producing the final
    // tuple.
    template<typename    _Type,
             typename... _Types>
    struct repeat_type_helper<_Type, 0, _Types...>
    {
        using type = std::tuple<_Types...>;
    };

NS_END  // internal

// repeat_type
//   trait: produces a std::tuple containing _Type repeated
// _NumTimes times.
//
//   NOT named `repeat`: functional/producer.hpp carries an infinite
// producer factory `repeat(_Value&&)` at this same namespace scope,
// and a class template and a function template of one name in one
// namespace is ill-formed - a class NAME may be hidden by a function
// name, a class TEMPLATE name may not.  This trait is the type-level
// one, so it takes the qualified spelling and the factory keeps the
// bare verb.
template<typename    _Type,
         std::size_t _NumTimes>
struct repeat_type
{
    // std::conditional_t is C++14; ::type is the C++11 spelling.
    typedef typename std::conditional<(_NumTimes > 0),
        typename internal::repeat_type_helper<_Type,
                                              _NumTimes>::type,
        std::tuple<>
    >::type type;
};

// repeat_type_t
//   type: convenience alias for repeat_type<_Type, _NumTimes>::type.
template<typename    _Type,
         std::size_t _NumTimes>
using repeat_type_t = typename repeat_type<_Type, _NumTimes>::type;


// ================================================================
//  self  /  resolve_self
// ================================================================

// self
//   type: self-reference marker for recursive type definitions.
struct self
{};

// is_self
//   trait: detects the self marker type (primary template).
template<typename _Type>
struct is_self : std::false_type
{};

// is_self<self>
//   trait: specialization recognizing the self marker.
template<>
struct is_self<self> : std::true_type
{};

// is_self_v
//   value: convenience alias for is_self<_Type>::value.
//
//   C++14 AND ABOVE. This is a variable TEMPLATE, and variable templates are a
// C++14 feature -- there is no C++11 spelling to fall back to, so the gate is
// D1 working as intended rather than a portability workaround: a higher tier
// ADDS the convenience and the floor keeps the trait it is a convenience for.
// Below 14, write is_self<T>::value, which is what this expands to anyway.
//
//   D_CONSTEXPR, not D_CONSTEXPR_INLINE. A variable template's instantiations
// already merge under vague linkage, so `inline` buys nothing -- and D_INLINE's
// always_inline is a FUNCTION attribute the compiler discards on a variable,
// warning on every TU that includes this header.
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
template<typename _Type>
D_CONSTEXPR bool is_self_v = is_self<_Type>::value;
#endif

// resolve_self
//   trait: resolves the self marker within a type to a concrete
// target type (primary template, passthrough).
template<typename _Type,
         typename _ResolveTo>
struct resolve_self
{
    using type = _Type;
};

// resolve_self<self, _ResolveTo>
//   trait: base case -- self resolves to _ResolveTo.
template<typename _ResolveTo>
struct resolve_self<self, _ResolveTo>
{
    using type = _ResolveTo;
};

// resolve_self<std::unique_ptr<self>, _ResolveTo>
//   trait: smart pointer specialization for unique_ptr.
template<typename _ResolveTo>
struct resolve_self<std::unique_ptr<self>, _ResolveTo>
{
    using type = std::unique_ptr<_ResolveTo>;
};

// resolve_self<std::shared_ptr<self>, _ResolveTo>
//   trait: smart pointer specialization for shared_ptr.
template<typename _ResolveTo>
struct resolve_self<std::shared_ptr<self>, _ResolveTo>
{
    using type = std::shared_ptr<_ResolveTo>;
};

// resolve_self<std::weak_ptr<self>, _ResolveTo>
//   trait: smart pointer specialization for weak_ptr.
template<typename _ResolveTo>
struct resolve_self<std::weak_ptr<self>, _ResolveTo>
{
    using type = std::weak_ptr<_ResolveTo>;
};

// resolve_self<self*, _ResolveTo>
//   trait: raw pointer specialization.
template<typename _ResolveTo>
struct resolve_self<self*, _ResolveTo>
{
    using type = _ResolveTo*;
};

// resolve_self_t
//   type: convenience alias for resolve_self<...>::type.
template<typename _Type,
         typename _ResolveTo>
using resolve_self_t = typename resolve_self<_Type, _ResolveTo>::type;

// resolve_self<_Template<_Args...>, _ResolveTo>
//   trait: variadic catch-all for any class template whose
// parameters are all types. Recursively resolves self within
// each template argument, enabling detection of self inside
// arbitrary std:: containers, wrappers, and user templates
// (e.g. std::vector<self>, std::optional<self>,
// std::pair<int, self>, etc.).
//
// Note: templates with non-type parameters (e.g. std::array)
// require their own dedicated specializations, as this partial
// specialization only matches template<typename...> forms.
template<template<typename...> class _Template,
         typename...                 _Args,
         typename                    _ResolveTo>
struct resolve_self<_Template<_Args...>, _ResolveTo>
{
    using type = _Template<resolve_self_t<_Args, _ResolveTo>...>;
};

// resolve_self<repeat_type<_Type, _NumTimes>, _ResolveTo>
//   trait: recursively resolves self within a repeated type.
template<typename    _Type,
         std::size_t _NumTimes,
         typename    _ResolveTo>
struct resolve_self<repeat_type<_Type, _NumTimes>, _ResolveTo>
{
    using resolved_inner = resolve_self_t<_Type, _ResolveTo>;
    using type           = repeat_type_t<resolved_inner, _NumTimes>;
};


NS_END  // djinterp


#endif  // DJINTERP_CPP_
