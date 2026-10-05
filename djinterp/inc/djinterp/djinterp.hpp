/*******************************************************************************
* djinterp [djinterp]                                               djinterp.hpp
*
* C++ core header for the djinterp framework.
*   Extends the C core with the namespace macros and the C++ qualifier kit,
* each spelled for the language level in use, from ISO C++98 to C++23. The
* type utilities it once held (void_t, clean, repeat_type, resolve_self, ...)
* are in core/meta/type_utility.hpp.
*
*
* path:      /inc/djinterp/djinterp.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.11.12
*                                                            revised: 2026.10.03
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  C++ KEYWORDS & NAMESPACE MACROS
    -------------------------------
    1.  C++ keywords
         1.  D_KEYWORD_CPP
         2.  D_KEYWORD_PARADIGM
         3.  D_KEYWORD_STL
         4.  D_KEYWORD_TRAITS
         5.  D_KEYWORD_CONCEPTS
         6.  D_KEYWORD_CONTAINER
         7.  D_KEYWORD_EXCEPTION
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
         13. NS_TEST
         14. NS_TESTING
         15. NS_TRAITS
2.  QUALIFIER SUPPORT
    -----------------
    1.  Constexpr family
         1.  D_CONSTEXPR
         2.  D_STATIC_CONSTEXPR
         3.  D_CONSTEXPR_INLINE
         4.  D_STATIC_CONSTEXPR_INLINE
         5.  D_CONSTEXPR_CPP14
         6.  D_CONSTEXPR_CPP17
         7.  D_CONSTEXPR_CPP20
         8.  D_CONSTEXPR_REQ
    2.  Variable qualifiers
         1.  D_INLINE_VAR
         2.  D_CONSTEXPR_INLINE_VAR
         3.  D_CONSTEXPR_VAR
    3.  Template-parameter constraints
         1.  D_CONCEPT_PARAM
    4.  C++11 feature spellings
         1.  D_NOEXCEPT
         2.  D_NOEXCEPT_IF
         3.  D_MOVE_ENABLED
         4.  D_EXPLICIT_BOOL
         5.  D_DELETED_FN
         6.  D_NULLPTR
         7.  D_OVERRIDE
3.  NAMESPACE ALIASES
    -----------------
    1.  Framework aliases
         1.  functional
*/

#ifndef DJINTERP_DJINTERP_HPP
#define DJINTERP_DJINTERP_HPP 1

// djinterp
#include "./c/djinterp.h"  // framework root
#include "./env/env.h"     // D_ENV_LANG_*, read by the floor and the kit

//   THE LANGUAGE FLOOR is C++98 (decision 3.6, with levels above it): this
// header and everything it includes compile at every level from ISO strict
// C++98 (-DD_CFG_ENV_ISO_STRICT=1) to C++23, and include no standard C++
// header. The kit below is the authority on what each level spells; a
// facility that needs a later level is gated on the env layer's
// D_ENV_LANG_IS_CPPnn_OR_HIGHER and absent below it -- never an error, and
// never a second meaning for the same name. The env layer is read rather than
// __cplusplus, so the level compared is the one env detection settled on,
// MSVC included, whose __cplusplus stays at 199711L unless /Zc:__cplusplus is
// given.
//   What it still refuses is C: the C framework's root is c/djinterp.h.
#if !D_ENV_LANG_USING_CPP
    #error "djinterp.hpp is C++; C code includes djinterp/c/djinterp.h."
#endif

// djinterp
#include "./config/cfg_qualifiers.h"     // qualifier configuration
#include "./env/cpp/env_cpp_features.h"  // D_ENV_CPP_FEATURE_*


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
// D_KEYWORD_STL
//   keyword: resolves to `stl`.
// Used to specify that a unit of code pertains to the STL
// (Standard Template Library) part of the C++ standard.
#define D_KEYWORD_STL               stl

// 1.1.4
// D_KEYWORD_TRAITS
//   keyword: resolves to `traits`.
// Used to specify that a unit of code uses template
// metaprogramming and SFINAE for compile-time logic.
#define D_KEYWORD_TRAITS            traits

// D_KEYWORD_USER_INTERFACE
//   constant: keyword naming the user-interface subsystem's namespace, used
// by NS_UI.
#define D_KEYWORD_USER_INTERFACE    ui

// 1.1.5
// D_KEYWORD_CONCEPTS
//   keyword: resolves to `concepts`.
// Used to specify that a unit of code provides C++20
// concept definitions.
#define D_KEYWORD_CONCEPTS          concepts

// 1.1.6
// D_KEYWORD_CONTAINER
//   keyword: resolves to `container`.
// Used to specify that a unit of code pertains to the
// container subsystem.
#define D_KEYWORD_CONTAINER         container

// 1.1.7
// D_KEYWORD_EXCEPTION
//   keyword: resolves to `exception`.
// Used to specify that a unit of code pertains to severe error-handling types;
// NS_EXCEPTION opens a namespace of this name.
#define D_KEYWORD_EXCEPTION         exception

// 1.2    Namespace macros
//------------------------------------------------------------------------------
// 1.2.1
// D_NAMESPACE
//   macro: wraps a block of code in a namespace with the name
// specified by parameter `NAME`.
#define D_NAMESPACE(NAME)           namespace NAME {

// 1.2.2
// NS_END
//   macro: namespace idiom; used to close any namespace. A bare `}`: the
// `;` it once carried is an empty declaration, which C++98 does not have
// (-pedantic rejects it there; C++11 added it).
#define NS_END                      }

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
#define NS_DJINTERP                 D_NAMESPACE(D_FRAMEWORK_NAME)

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
// NS_TEST
//   namespace: the `test` namespace for unit testing
// utilities.
#define NS_TEST                     D_NAMESPACE(D_KEYWORD_TEST)

// 1.2.14
// NS_TESTING
//   namespace: the `testing` namespace for holding unit tests.
#define NS_TESTING                  D_NAMESPACE(D_KEYWORD_TESTING)

// 1.2.15
// NS_TRAITS
//   namespace: the `traits` namespace for the compile-time
// trait surface of a subsystem (e.g. djinterp::container::traits).
#define NS_TRAITS                   D_NAMESPACE(D_KEYWORD_TRAITS)

// NS_UI
//   macro: opens the user-interface subsystem's namespace.
#define NS_UI                       D_NAMESPACE(D_KEYWORD_USER_INTERFACE)


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
// toggle. The level forms (D_CONSTEXPR_CPP14, _CPP17, _CPP20) are D_CONSTEXPR
// from their level up, so D_CFG_TESTING_STRIP_CONSTEXPR strips them too. This
// is the kit's only definition of each: a module does not keep a private copy
// (decision 4.2). Valid in C++ because D_INLINE carries no `static` here. (The
// non-constexpr D_STATIC_INLINE lives in djinterp.h.)

// 2.1.1
// D_CONSTEXPR
//   qualifier: `constexpr` from C++11, where the keyword exists; empty below,
// and when the config layer sets D_INTERNAL_QUAL_STRIP_CONSTEXPR (test
// instrumentation).
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR
        #if D_INTERNAL_QUAL_STRIP_CONSTEXPR
            #define D_CONSTEXPR             // stripped for test instrumentation
        #elif D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_CONSTEXPR             constexpr
        #else
            #define D_CONSTEXPR
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

// 2.1.5
// D_CONSTEXPR_CPP14
//   qualifier: D_CONSTEXPR from C++14, empty below: for a function whose body
// needs C++14's relaxed constexpr (local variables, loops, more than one
// statement), which a C++11 constexpr function may not have.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR_CPP14
        #if D_ENV_LANG_IS_CPP14_OR_HIGHER
            #define D_CONSTEXPR_CPP14       D_CONSTEXPR
        #else
            #define D_CONSTEXPR_CPP14
        #endif
    #endif  // D_CONSTEXPR_CPP14
#endif

// 2.1.6
// D_CONSTEXPR_CPP17
//   qualifier: D_CONSTEXPR from C++17, empty below: for a function that is
// constant-evaluable only with C++17's additions (constexpr lambdas, the
// constexpr standard-library members it calls).
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR_CPP17
        #if D_ENV_LANG_IS_CPP17_OR_HIGHER
            #define D_CONSTEXPR_CPP17       D_CONSTEXPR
        #else
            #define D_CONSTEXPR_CPP17
        #endif
    #endif  // D_CONSTEXPR_CPP17
#endif

// 2.1.7
// D_CONSTEXPR_CPP20
//   qualifier: D_CONSTEXPR from C++20, empty below: for a function that needs
// C++20's constexpr (virtual calls, try blocks, transient allocation,
// trivial default initialization, a changed union member).
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR_CPP20
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            #define D_CONSTEXPR_CPP20       D_CONSTEXPR
        #else
            #define D_CONSTEXPR_CPP20
        #endif
    #endif  // D_CONSTEXPR_CPP20
#endif

// 2.1.8
// D_CONSTEXPR_REQ
//   qualifier: `constexpr` from C++11 in every build -- the testing switch
// that strips D_CONSTEXPR (D_CFG_TESTING_STRIP_CONSTEXPR) leaves it alone --
// and `inline` below C++11. Only for a function a constant expression needs (a
// template argument, a static assertion, an array bound); everything else
// uses D_CONSTEXPR, so testing builds still run it at run time. The owner's
// ruling of 2026.10.03.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR_REQ
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            #define D_CONSTEXPR_REQ         constexpr
        #else
            #define D_CONSTEXPR_REQ         inline
        #endif
    #endif  // D_CONSTEXPR_REQ
#endif

// 2.2    Variable qualifiers
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

// 2.2.3
// D_CONSTEXPR_VAR
//   qualifier: a compile-time constant OBJECT: `constexpr` from C++11, `const`
// below, where an integral static member or a namespace-scope constant with a
// constant initializer is still a constant expression. Never empty: under
// D_CFG_TESTING_STRIP_CONSTEXPR it is `const` too, because an initialized
// static data member must be const and a namespace-scope constant must keep
// its internal linkage.
//       static D_CONSTEXPR_VAR std::size_t capacity = 16;
//   It adds no linkage of its own: a member writes `static`, and a header
// constant that should be one object from C++17 takes D_CONSTEXPR_INLINE_VAR.
#if (D_INTERNAL_CFG_CONSTEXPR == 1)
    #ifndef D_CONSTEXPR_VAR
        #if ( (D_ENV_LANG_IS_CPP11_OR_HIGHER) &&                              \
              (!D_INTERNAL_QUAL_STRIP_CONSTEXPR) )
            #define D_CONSTEXPR_VAR         constexpr
        #else
            #define D_CONSTEXPR_VAR         const
        #endif
    #endif  // D_CONSTEXPR_VAR
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
// each names a C++11 feature and spells it from C++11 up. Below C++11 it is
// empty, or a declaration with the same access and meaning, or -- where no
// spelling keeps the meaning -- absent, so a use is diagnosed at the level
// that cannot express it. The choices are the owner's decisions 3.1 to 3.3.

// 2.4.1
// D_NOEXCEPT
//   qualifier: the no-throw exception specification: `noexcept` from C++11,
// empty below (decision 3.1). `throw()` is not a spelling of it: it adds a
// run-time path to std::unexpected, can pessimise code, and is gone from
// C++20, while a C++98 build, without move semantics, gains nothing from the
// guarantee.
//   pre-definable: users may #define D_NOEXCEPT before including this header
// to override it.
#ifndef D_NOEXCEPT
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NOEXCEPT                  noexcept
    #else
        #define D_NOEXCEPT
    #endif
#endif  // D_NOEXCEPT

// 2.4.2
// D_NOEXCEPT_IF
//   qualifier: the conditional exception specification, `noexcept(cond)`
// from C++11, empty below. One argument, so the macro needs no variadic macro
// and stays valid under ISO C++98 (decision 4.4). A condition with a
// top-level comma takes a second pair of parentheses, which `noexcept`
// accepts as they stand:
//       D_NOEXCEPT_IF((is_nothrow_pair<A, B>::value))
//   pre-definable.
#ifndef D_NOEXCEPT_IF
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NOEXCEPT_IF(_cond)        noexcept(_cond)
    #else
        #define D_NOEXCEPT_IF(_cond)
    #endif
#endif  // D_NOEXCEPT_IF

// 2.4.3
// D_MOVE_ENABLED
//   constant: move-support flag: 1 from C++11, so a resource type can declare
// a move constructor and move assignment; 0 below, where rvalue references do
// not exist. Named for its use, so a resource type reads `#if D_MOVE_ENABLED`
// rather than restating a standard check at every ownership boundary.
//   pre-definable: 0 still asks a C++11 type to be non-movable as well as
// non-copyable.
#ifndef D_MOVE_ENABLED
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_MOVE_ENABLED              1
    #else
        #define D_MOVE_ENABLED              0
    #endif
#endif  // D_MOVE_ENABLED

// 2.4.4
// D_EXPLICIT_BOOL
//   qualifier: `explicit` on a bool conversion operator, so an object does
// not silently become an int in arithmetic. C++11 and up only: explicit
// conversion functions do not exist below, and an implicit `operator bool`
// is the very conversion this guards against, so below C++11 the macro is
// absent, and the operator with it (decision 3.2). A type gives its validity
// a named predicate (`is_valid()` style) at every level, and adds the
// operator from C++11:
//       #if D_ENV_LANG_IS_CPP11_OR_HIGHER
//           D_EXPLICIT_BOOL operator bool() const;
//       #endif
//   pre-definable.
#if ( (!defined(D_EXPLICIT_BOOL)) &&                                          \
      (D_ENV_LANG_IS_CPP11_OR_HIGHER) )
    #define D_EXPLICIT_BOOL                 explicit
#endif

// 2.4.5
// D_DELETED_FN
//   macro: marks a member that must not exist, taking the full declaration
// as its argument: `= delete` from C++11; below, the declaration alone, which
// the class never defines (decision 3.3). Write it in a private: section at
// every level, so access is the same on every tier:
//       private:
//           D_DELETED_FN(my_type(const my_type&))
//           D_DELETED_FN(my_type& operator=(const my_type&))
// From C++11 any use is a compile error naming the deleted function. Below,
// a use from outside the class is an access error at compile time, and one
// from inside the class or a friend a link error -- the one difference, and
// why the C++11 spelling is used wherever it exists.
//   pre-definable.
#ifndef D_DELETED_FN
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_DELETED_FN(_decl)         _decl = delete;
    #else
        #define D_DELETED_FN(_decl)         _decl;
    #endif
#endif  // D_DELETED_FN

// 2.4.6
// D_NULLPTR
//   constant: the null pointer constant: `nullptr` from C++11; `0` below,
// which is C++98's. Write it only where a pointer is expected: `0` is an int,
// so an overload set taking both an integer and a pointer resolves
// differently below C++11.
//   pre-definable.
#ifndef D_NULLPTR
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_NULLPTR                   nullptr
    #else
        #define D_NULLPTR                   0
    #endif
#endif  // D_NULLPTR

// 2.4.7
// D_OVERRIDE
//   qualifier: `override` on a virtual function that must override one in a
// base, from C++11; empty below, where the override is unchecked but the same.
//   pre-definable.
#ifndef D_OVERRIDE
    #if D_ENV_LANG_IS_CPP11_OR_HIGHER
        #define D_OVERRIDE                  override
    #else
        #define D_OVERRIDE
    #endif
#endif  // D_OVERRIDE


//==============================================================================
// 3.  NAMESPACE ALIASES
//==============================================================================
// The type utilities that used to follow here (void_t, abs_value, clean,
// constexpr_swap, repeat_type, self, resolve_self) are metaprogramming, and
// live in core/meta/type_utility.hpp (decision 4.4); the root keeps the macro
// kit and the namespace vocabulary.


NS_DJINTERP

// 3.1    Framework aliases
//------------------------------------------------------------------------------
// 3.1.1
// functional
//   alias: `functional` as a second name for the framework namespace,
// because the core/functional and core/event headers refer to their own
// entities as functional::result and so on. Declared inside djinterp, so
// it adds no name to the global namespace.
namespace functional = ::djinterp;


NS_END  // djinterp


//   TRANSITIONAL. Every unit that uses the type utilities reached them
// through this header until they moved, so the root still includes their new
// home, at its END: type_utility.hpp includes this header first and needs the
// kit above, so a unit may include either header first. Once every such unit
// includes core/meta/type_utility.hpp itself, this include goes, and the root
// no longer depends on core/ at all (results_root.md lists the units).
#include "./core/meta/type_utility.hpp"  // void_t, clean, resolve_self, ...


#endif  // DJINTERP_DJINTERP_HPP
