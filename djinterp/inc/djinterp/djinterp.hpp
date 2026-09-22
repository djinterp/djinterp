/*******************************************************************************
* djinterp [djinterp]                                               djinterp.hpp
*
* C++ core header for the djinterp framework.
*   Extends the C core with namespace macros, constexpr support, and
* foundational type utilities including type cleaning, compile-time repetition,
* and self-referential type resolution.
*
* path:      /inc/djinterp/djinterp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.11.12
*                                                            revised: 2026.09.22
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  C++ KEYWORDS & NAMESPACE MACROS
    -------------------------------
    1.  C++ keywords
         1.  D_KEYWORD_CPP
         2.  D_KEYWORD_PARADIGM
         3.  D_KEYWORD_RE_STD
         4.  D_KEYWORD_STL
         5.  D_KEYWORD_TRAITS
         6.  D_KEYWORD_CONCEPTS
         7.  D_KEYWORD_CONTAINER
    2.  Namespace macros
         1.  D_NAMESPACE
         2.  NS_END
         3.  NS_CONCEPTS
         4.  NS_CONTAINER
         5.  NS_DATABASE
         6.  NS_DJINTERP
         7.  NS_ERROR
         8.  NS_EXCEPTION
         9.  NS_INTERNAL
         10. NS_MATH
         11. NS_MESSAGE
         12. NS_PARADIGM
         13. NS_RESTD
         14. NS_TEST
         15. NS_TESTING
         16. NS_TRAITS
2.  QUALIFIER SUPPORT
    -----------------
    1.  Constexpr family
         1.  D_CONSTEXPR
         2.  D_STATIC_CONSTEXPR
         3.  D_CONSTEXPR_INLINE
         4.  D_STATIC_CONSTEXPR_INLINE
    2.  Inline variables
         1.  D_INLINE_VAR
         2.  D_CONSTEXPR_INLINE_VAR
    3.  Template-parameter constraints
         1.  D_CONCEPT_PARAM
    4.  C++11 feature spellings
         1.  D_NOEXCEPT
         2.  D_NOEXCEPT_IF
         3.  D_MOVE_ENABLED
         4.  D_EXPLICIT_BOOL
         5.  D_DELETED_FN
3.  CORE TYPE UTILITIES
    -------------------
    1.  Detection idiom
         1.  void_t
    2.  Absolute value
         1.  abs_value
         2.  abs_value_v
         3.  abs_value_to_size_t
    3.  Type cleaning
         1.  clean
         2.  clean_t
    4.  Constexpr swap
         1.  constexpr_swap
    5.  Type repetition
         1.  repeat_type_helper (internal)
         2.  repeat_type
         3.  repeat_type_t
    6.  Self-reference resolution
         1.  self
         2.  is_self
         3.  is_self_v
         4.  resolve_self
         5.  resolve_self_t
         6.  resolve_self recursive specializations
*/

#ifndef DJINTERP_DJINTERP_HPP
#define DJINTERP_DJINTERP_HPP 1

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
#if ( !defined(__cplusplus) ||                                                \
      (__cplusplus < 201103L) )
    #error "djinterp requires C++11 or later. Compile with -std=c++11 "       \
           "(or newer), or /std:c++14 on MSVC."
#endif

// std
#include <cstddef>           // std::size_t
#include <initializer_list>  // std::initializer_list
#include <memory>            // std::unique_ptr, std::shared_ptr, std::weak_ptr
#include <tuple>             // std::tuple
#include <type_traits>       // std::conditional, std::integral_constant,
                             // std::remove_cv, std::remove_reference, std::is_*
#include <utility>           // std::swap
// djinterp
#include "./c/djinterp.h"                // framework root
#include "./config/cfg_qualifiers.h"     // qualifier configuration
#include "./env/env.h"                   // language/compiler/OS detection
#include "./env/cpp/env_cpp98.h"         // C++98 LIBRARY-header detection;
                                         // still relevant above the floor -- it
                                         // answers "does this implementation
                                         // ship <map>", not "is this C++98"
#include "./env/cpp/env_cpp_features.h"  // C++ feature detection


//==============================================================================
// 1.  C++ KEYWORDS & NAMESPACE MACROS
//==============================================================================


// 1.1    C++ keywords
//------------------------------------------------------------------------------
// 1.1.1
// D_KEYWORD_CPP
//   keyword: resolves to `cpp`.
// Used to specify that a unit of code pertains to the C++
// standard.
#define D_KEYWORD_CPP               cpp

// 1.1.2
// D_KEYWORD_PARADIGM
//   keyword: resolves to `paradigm`.
// Used to specify that a unit of code pertains to a pattern or software
// paradigm.
#define D_KEYWORD_PARADIGM          paradigm

// 1.1.3
// D_KEYWORD_RE_STD
//   keyword: resolves to `re_std`.
// Used to specify the re-std, a more-portable, backwards-compatible version
// of the std library.
#define D_KEYWORD_RE_STD            re_std

// 1.1.4
// D_KEYWORD_STL
//   keyword: resolves to `stl`.
// Used to specify that a unit of code pertains to the STL
// (Standard Template Library) part of the C++ standard.
#define D_KEYWORD_STL               stl

// 1.1.5
// D_KEYWORD_TRAITS
//   keyword: resolves to `traits`.
// Used to specify that a unit of code uses template
// metaprogramming and SFINAE for compile-time logic.
#define D_KEYWORD_TRAITS            traits

// 1.1.6
// D_KEYWORD_CONCEPTS
//   keyword: resolves to `concepts`.
// Used to specify that a unit of code provides C++20
// concept definitions.
#define D_KEYWORD_CONCEPTS          concepts

// 1.1.7
// D_KEYWORD_CONTAINER
//   keyword: resolves to `container`.
// Used to specify that a unit of code pertains to the
// container subsystem.
#define D_KEYWORD_CONTAINER         container

// 1.2    Namespace macros
//------------------------------------------------------------------------------
// 1.2.1
// D_NAMESPACE
//   macro: wraps a block of code in a namespace with the name
// specified by parameter `NAME`.
#define D_NAMESPACE(NAME)           namespace NAME {

// 1.2.2
// NS_END
//   macro: namespace idiom; used to close any namespace.
#define NS_END                      };

// 1.2.3
// NS_CONCEPTS
//   namespace: the `concepts` namespace for C++20 concept
// definitions layered over a subsystem's trait surface.
#define NS_CONCEPTS                 D_NAMESPACE(D_KEYWORD_CONCEPTS)

// 1.2.4
// NS_CONTAINER
//   namespace: the `container` namespace for the container
// subsystem and its trait/concept surface.
#define NS_CONTAINER                D_NAMESPACE(D_KEYWORD_CONTAINER)

// 1.2.5
// NS_DATABASE
//   namespace: database namespace containing functionality pertaining to
// databases and database systems.
#define NS_DATABASE                 D_NAMESPACE(D_KEYWORD_DATABASE)

// 1.2.6
// NS_DJINTERP
//   namespace: the `djinterp` top-level namespace. Should be
// used as the top-level namespace of any and all modules
// within the djinterp tool-chain.
#define NS_DJINTERP                 D_NAMESPACE(D_KEYWORD_FRAMEWORK_NAME)

// 1.2.7
// NS_ERROR
//   namespace: the `error` namespace for error-handling types
// and utilities.
#define NS_ERROR                    D_NAMESPACE(D_KEYWORD_ERROR)

// 1.2.8
// NS_EXCEPTION
//   namespace: the `exception` namespace for severe
// error-handling types and utilities.
#define NS_EXCEPTION                D_NAMESPACE(D_KEYWORD_EXCEPTION)

// 1.2.9
// NS_INTERNAL
//   namespace: declares an `internal` namespace. Used to hide
// implementation details from regular use, such as "helper"
// types, structs and functions. Should be closed with `NS_END`.
#define NS_INTERNAL                 D_NAMESPACE(D_KEYWORD_INTERNAL)

// 1.2.10
// NS_MATH
//   namespace: the `math` namespace for mathematical
// utilities.
#define NS_MATH                     D_NAMESPACE(D_KEYWORD_MATH)

// 1.2.11
// NS_MESSAGE
//   namespace: the `message` namespace for variables, macros,
// and types that convey (usually string-based) human-readable
// information. Often (but not limited to) debugging and
// error-handling.
#define NS_MESSAGE                  D_NAMESPACE(D_KEYWORD_MESSAGE)

// 1.2.12
// NS_PARADIGM
//   namespace: resolves to `namespace paradigm {`
// This namespace contains common software patterns or idioms.
#define NS_PARADIGM                 D_NAMESPACE(D_KEYWORD_PARADIGM)

// 1.2.13
// NS_RESTD
//   namespace: the re-std namespace, containing more-portable,
// backwards-compatible versions of std library types, functions,
// and more.
#define NS_RESTD                    D_NAMESPACE(D_KEYWORD_RE_STD)

// 1.2.14
// NS_TEST
//   namespace: the `test` namespace for unit testing
// utilities.
#define NS_TEST                     D_NAMESPACE(D_KEYWORD_TEST)

// 1.2.15
// NS_TESTING
//   namespace: the `testing` namespace for holding unit tests.
#define NS_TESTING                  D_NAMESPACE(D_KEYWORD_TESTING)

// 1.2.16
// NS_TRAITS
//   namespace: the `traits` namespace for the compile-time
// trait surface of a subsystem (e.g. djinterp::container::traits).
#define NS_TRAITS                   D_NAMESPACE(D_KEYWORD_TRAITS)


//==============================================================================
// 2.  QUALIFIER SUPPORT
//==============================================================================
// C++-only qualifiers and feature spellings. The storage / inlining qualifiers
// (D_STATIC, D_INLINE, D_STATIC_INLINE) are defined ONCE in djinterp.h for
// both languages and inherited here -- not redefined, so there is no C vs. C++
// drift. Gates come from cfg_qualifiers.h.


// 2.1    Constexpr family
//------------------------------------------------------------------------------
// the compounds (order: static constexpr inline) are composed from D_STATIC /
// D_INLINE (from djinterp.h) and D_CONSTEXPR below; each honors its own
// toggle. Valid in C++ because D_INLINE carries no `static` here. (The
// non-constexpr D_STATIC_INLINE lives in djinterp.h.)

// 2.1.1
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
    #endif  // D_CONSTEXPR
#endif

// 2.1.2
// D_STATIC_CONSTEXPR
//   qualifier: `static constexpr`, composed as D_STATIC D_CONSTEXPR.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_STATIC_CONSTEXPR
        #define D_STATIC_CONSTEXPR          D_STATIC D_CONSTEXPR
    #endif  // D_STATIC_CONSTEXPR
#endif

// 2.1.3
// D_CONSTEXPR_INLINE
//   qualifier: `constexpr inline`, composed as D_CONSTEXPR D_INLINE.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR_INLINE
        #define D_CONSTEXPR_INLINE          D_CONSTEXPR D_INLINE
    #endif  // D_CONSTEXPR_INLINE
#endif

// 2.1.4
// D_STATIC_CONSTEXPR_INLINE
//   qualifier: `static constexpr inline`, composed as D_STATIC D_CONSTEXPR
// D_INLINE.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_STATIC_CONSTEXPR_INLINE
        #define D_STATIC_CONSTEXPR_INLINE   D_STATIC D_CONSTEXPR D_INLINE
    #endif  // D_STATIC_CONSTEXPR_INLINE
#endif

// 2.2    Inline variables
//------------------------------------------------------------------------------
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

// 2.2.1
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
#if ( !defined(D_INLINE_VAR) &&                                               \
      (D_INTERNAL_CFG_INLINE == 1) )
    #if D_ENV_LANG_IS_CPP17_OR_HIGHER
        #define D_INLINE_VAR                inline
    #else
        #define D_INLINE_VAR
    #endif
#endif

// 2.2.2
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
#if ( (D_INTERNAL_CFG_CONSTEXPR == 1) &&                                      \
      (D_INTERNAL_CFG_INLINE == 1) )
    #ifndef D_CONSTEXPR_INLINE_VAR
        #if D_ENV_LANG_IS_CPP17_OR_HIGHER
            #define D_CONSTEXPR_INLINE_VAR  D_CONSTEXPR D_INLINE_VAR
        #else
            #define D_CONSTEXPR_INLINE_VAR  D_STATIC D_CONSTEXPR
        #endif
    #endif  // D_CONSTEXPR_INLINE_VAR
#endif

// 2.3    Template-parameter constraints
//------------------------------------------------------------------------------
// 2.3.1
// D_CONCEPT_PARAM
//   qualifier: writes a type-constraint on a template parameter where the
// tier supports one, and plain `typename` where it does not:
//
//     template<D_CONCEPT_PARAM(OverridePolicy) Policy,
//              typename                        Set>
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
        #define D_CONCEPT_PARAM(Concept)    Concept
    #else
        #define D_CONCEPT_PARAM(Concept)    typename
    #endif
#endif  // D_CONCEPT_PARAM

// 2.4    C++11 feature spellings
//------------------------------------------------------------------------------
// 2.4.1
// D_NOEXCEPT
//   qualifier: portable no-throw specifier. `noexcept` is a first-class
// keyword at the C++11 floor; in C mode the concept does not exist, so the
// macro expands to nothing.
//   pre-definable: users may #define D_NOEXCEPT before including this header
// to override the detected value.
#ifndef D_NOEXCEPT
    #if defined(__cplusplus)
        #define D_NOEXCEPT noexcept      // C++11 floor; `throw()` is dead
    #else
        #define D_NOEXCEPT
    #endif
#endif  // D_NOEXCEPT

// 2.4.2
// D_NOEXCEPT_IF
//   qualifier: conditional exception specification; expands to
// noexcept(cond) at the C++11 floor, and to nothing in C mode.
//   variadic so a condition containing a top-level comma (e.g. a multi-arg
// trait) passes through intact.
#ifndef D_NOEXCEPT_IF
    #if defined(__cplusplus)
        #define D_NOEXCEPT_IF(...) noexcept(__VA_ARGS__)
    #else
        #define D_NOEXCEPT_IF(...)
    #endif
#endif  // D_NOEXCEPT_IF

// 2.4.3
// D_MOVE_ENABLED
//   constant: move-support flag. 1 when the language has rvalue references,
// so a move constructor and move assignment can be declared; 0 in C mode,
// where a resource type is constructed in place and passed by reference
// instead. Named for its use, so a resource type reads `#if D_MOVE_ENABLED`
// rather than restating the standard check at every ownership boundary.
//   pre-definable: users may #define D_MOVE_ENABLED before including this
// header to override the detected value.
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

// 2.4.4
// D_EXPLICIT_BOOL
//   qualifier: `explicit` on a bool conversion operator, so an object does
// not silently become an int in arithmetic. Empty in C mode, where the named
// observer (is_open / valid / failed) is the spelling to prefer for the same
// question.
//   pre-definable.
#ifndef D_EXPLICIT_BOOL
    #if defined(__cplusplus)
        #define D_EXPLICIT_BOOL explicit  // C++11 floor; safe-bool is dead
    #else
        #define D_EXPLICIT_BOOL
    #endif
#endif  // D_EXPLICIT_BOOL

// 2.4.5
// D_DELETED_FN
//   macro: deleted-function spelling. Marks a member that must not exist,
// taking the full declaration as its argument; it expands to `= delete`, so a
// use is a compile error with a clear message.
//
//   usage (in a private: section):
//       D_DELETED_FN(my_type(const my_type&))
//       D_DELETED_FN(my_type& operator=(const my_type&))
//   pre-definable.
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


//==============================================================================
// 3.  CORE TYPE UTILITIES
//==============================================================================
// Foundational type utilities, all declared in namespace djinterp.


NS_DJINTERP

// 3.1    Detection idiom
//------------------------------------------------------------------------------
// 3.1.1
// void_t
//   type: maps any type sequence to void. Used as the SFINAE
// sink in detection idioms. Pre-C++17 replacement for
// std::void_t.
template<typename...>
using void_t = void;

// 3.2    Absolute value
//------------------------------------------------------------------------------
// 3.2.1
// abs_value
//   trait: computes the absolute value of a compile-time
// integral constant.
template<typename Type,
         Type     N>
struct abs_value
{
    static_assert(std::is_integral<Type>::value,
                  "Type parameter `Type` must be an integral type.");

    static constexpr Type value = (N < 0)
                                  ? -(N)
                                  : N;
};

// 3.2.2
// abs_value_v
//   value: convenience variable template for abs_value<Type, N>::value.
// Requires variable templates (C++14).
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type,
             Type     N>
    constexpr Type abs_value_v = abs_value<Type, N>::value;
#endif

// 3.2.3
// abs_value_to_size_t
//   value: convenience variable template that yields the absolute value of N
// as a std::size_t. Requires variable templates (C++14).
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type,
             Type     N>
    constexpr std::size_t abs_value_to_size_t =
        std::integral_constant<std::size_t,
                               abs_value<Type, N>::value>::value;
#endif

// 3.3    Type cleaning
//------------------------------------------------------------------------------
// 3.3.1
// clean
//   trait: strips cv-qualifiers and references.
//   Spelled through ::type rather than the _t aliases: std::remove_cv_t and
// std::remove_reference_t are C++14 library additions, and this header is
// included by every C++ module in the framework -- so using them here would
// make C++14 the floor for all of them.
template<typename Type>
struct clean
{
    typedef typename std::remove_cv<
        typename std::remove_reference<Type>::type>::type type;
};

// 3.3.2
// clean_t
//   type: convenience alias for clean<Type>::type.
//   The `typename` is required below C++20 -- P0634 made it optional in this
// position, so omitting it silently pins the header to C++20.
template<typename Type>
using clean_t = typename clean<Type>::type;

// 3.4    Constexpr swap
//------------------------------------------------------------------------------
// 3.4.1
// constexpr_swap
//   function: swaps two values in a constexpr context. std::swap is not
// constexpr until C++20, so this provides the same semantics for C++14 and
// C++17; C++11 gets the same body without constexpr, since C++11 constexpr
// forbids local variables and assignment.
//   each tier's declaration carries its own contract: clang attaches a doc
// comment only to a declaration that directly follows it, never across an
// #if.
#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    /**
     * @brief Swaps two values in a constant expression, via `std::swap`.
     *
     * @param[in,out] _a  the first value; receives the second's value.
     * @param[in,out] _b  the second value; receives the first's value.
     */
    template<typename Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
        Type& _a,
        Type& _b
    ) noexcept( std::is_nothrow_move_constructible<Type>::value &&
                std::is_nothrow_move_assignable<Type>::value )
    {
        std::swap(_a,
                  _b);

        return;
    }

#elif D_ENV_LANG_IS_CPP14_OR_HIGHER

    /**
     * @brief Swaps two values in a constant expression, via a move-based
     *        swap under relaxed constexpr.
     *
     * @param[in,out] _a  the first value; receives the second's value.
     * @param[in,out] _b  the second value; receives the first's value.
     */
    template<typename Type>
    D_CONSTEXPR_INLINE void
    constexpr_swap(
        Type& _a,
        Type& _b
    ) noexcept( std::is_nothrow_move_constructible<Type>::value &&
                std::is_nothrow_move_assignable<Type>::value )
    {
        Type temp = static_cast<Type&&>(_a);
        _a        = static_cast<Type&&>(_b);
        _b        = static_cast<Type&&>(temp);

        return;
    }

#else  // C++11

    /**
     * @brief Swaps two values via a move-based swap; not constexpr, since
     *        C++11 constexpr forbids local variables and assignment.
     *
     * @param[in,out] _a  the first value; receives the second's value.
     * @param[in,out] _b  the second value; receives the first's value.
     */
    template<typename Type>
    D_INLINE void
    constexpr_swap(
        Type& _a,
        Type& _b
    ) noexcept( std::is_nothrow_move_constructible<Type>::value &&
                std::is_nothrow_move_assignable<Type>::value )
    {
        Type temp = static_cast<Type&&>(_a);
        _a        = static_cast<Type&&>(_b);
        _b        = static_cast<Type&&>(temp);

        return;
    }

#endif

// 3.5    Type repetition
//------------------------------------------------------------------------------
// 3.5.1
// repeat_type_helper (internal)
NS_INTERNAL

    // repeat_type_helper
    //   trait: internal recursive builder for repeat_type<>
    // (general case).
    template<typename    Type,
             std::size_t N,
             typename... Types>
    struct repeat_type_helper
    {
        using type = typename repeat_type_helper<Type,
                                                 (N - 1),
                                                 Type,
                                                 Types...>::type;
    };

    // repeat_type_helper<Type, 0, Types...>
    //   trait: base case specialization producing the final
    // tuple.
    template<typename    Type,
             typename... Types>
    struct repeat_type_helper<Type, 0, Types...>
    {
        using type = std::tuple<Types...>;
    };

NS_END  // internal

// 3.5.2
// repeat_type
//   trait: produces a std::tuple containing Type repeated
// NumTimes times.
//
//   NOT named `repeat`: functional/producer.hpp carries an infinite
// producer factory `repeat(_Value&&)` at this same namespace scope,
// and a class template and a function template of one name in one
// namespace is ill-formed - a class NAME may be hidden by a function
// name, a class TEMPLATE name may not.  This trait is the type-level
// one, so it takes the qualified spelling and the factory keeps the
// bare verb.
template<typename    Type,
         std::size_t NumTimes>
struct repeat_type
{
    // std::conditional_t is C++14; ::type is the C++11 spelling.
    typedef typename std::conditional<(NumTimes > 0),
        typename internal::repeat_type_helper<Type,
                                              NumTimes>::type,
        std::tuple<>
    >::type type;
};

// 3.5.3
// repeat_type_t
//   type: convenience alias for repeat_type<Type, NumTimes>::type.
template<typename    Type,
         std::size_t NumTimes>
using repeat_type_t = typename repeat_type<Type, NumTimes>::type;

// 3.6    Self-reference resolution
//------------------------------------------------------------------------------
// 3.6.1
// self
//   type: self-reference marker for recursive type definitions.
struct self
{};

// 3.6.2
// is_self
//   trait: detects the self marker type (primary template).
template<typename Type>
struct is_self : std::false_type
{};

// is_self<self>
//   trait: specialization recognizing the self marker.
template<>
struct is_self<self> : std::true_type
{};

// 3.6.3
// is_self_v
//   value: convenience alias for is_self<Type>::value.
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
    template<typename Type>
    D_CONSTEXPR bool is_self_v = is_self<Type>::value;
#endif

// 3.6.4
// resolve_self
//   trait: resolves the self marker within a type to a concrete
// target type (primary template, passthrough).
template<typename Type,
         typename ResolveTo>
struct resolve_self
{
    using type = Type;
};

// resolve_self<self, ResolveTo>
//   trait: base case -- self resolves to ResolveTo.
template<typename ResolveTo>
struct resolve_self<self, ResolveTo>
{
    using type = ResolveTo;
};

// resolve_self<std::unique_ptr<self>, ResolveTo>
//   trait: smart pointer specialization for unique_ptr.
template<typename ResolveTo>
struct resolve_self<std::unique_ptr<self>, ResolveTo>
{
    using type = std::unique_ptr<ResolveTo>;
};

// resolve_self<std::shared_ptr<self>, ResolveTo>
//   trait: smart pointer specialization for shared_ptr.
template<typename ResolveTo>
struct resolve_self<std::shared_ptr<self>, ResolveTo>
{
    using type = std::shared_ptr<ResolveTo>;
};

// resolve_self<std::weak_ptr<self>, ResolveTo>
//   trait: smart pointer specialization for weak_ptr.
template<typename ResolveTo>
struct resolve_self<std::weak_ptr<self>, ResolveTo>
{
    using type = std::weak_ptr<ResolveTo>;
};

// resolve_self<self*, ResolveTo>
//   trait: raw pointer specialization.
template<typename ResolveTo>
struct resolve_self<self*, ResolveTo>
{
    using type = ResolveTo*;
};

// 3.6.5
// resolve_self_t
//   type: convenience alias for resolve_self<...>::type.
template<typename Type,
         typename ResolveTo>
using resolve_self_t = typename resolve_self<Type, ResolveTo>::type;

// 3.6.6
// resolve_self recursive specializations
// resolve_self<Template<Args...>, ResolveTo>
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
template<template<typename...> class Template,
         typename...                 Args,
         typename                    ResolveTo>
struct resolve_self<Template<Args...>, ResolveTo>
{
    using type = Template<resolve_self_t<Args, ResolveTo>...>;
};

// resolve_self<repeat_type<Type, NumTimes>, ResolveTo>
//   trait: recursively resolves self within a repeated type.
template<typename    Type,
         std::size_t NumTimes,
         typename    ResolveTo>
struct resolve_self<repeat_type<Type, NumTimes>, ResolveTo>
{
    using resolved_inner = resolve_self_t<Type, ResolveTo>;
    using type           = repeat_type_t<resolved_inner, NumTimes>;
};

NS_END  // djinterp


#endif  // DJINTERP_DJINTERP_HPP
