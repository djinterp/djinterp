/******************************************************************************
* djinterp [cli]                                              cli_concepts.hpp
*
*   C++20 concept analogs of the cli_traits.hpp trait machinery.
*
*   Mirrors the option_concepts.hpp parallel-facility approach: callers
* may freely use either traits + SFINAE or concepts + requires-clauses.
* Both speak about the same shapes.
*
*   Adds one purely-structural concept (cli_descriptor_shape_c) that
* matches any type that "quacks like" a descriptor, regardless of
* whether it derives from the cli_descriptor base.  Useful for generic
* helpers that want to accept either form.
*
*
* path:      /inc/djinterp/core/cli/cli_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cli_name_c
II.   cli_short_c
III.  cli_descriptor_c          (nominal: derived from cli_descriptor)
IV.   cli_descriptor_shape_c    (structural: quacks like one)
*/

#ifndef DJINTERP_CLI_CONCEPTS_
#define DJINTERP_CLI_CONCEPTS_ 1

// std
#include <string_view>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./cli.hpp"
#include "./cli_traits.hpp"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "cli_concepts.hpp requires C++20 or later"
#endif

#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "cli_concepts.hpp requires compiler support for concepts"
#endif


NS_DJINTERP


// ===========================================================================
// I.   cli_name_c
// ===========================================================================

// cli_name_c
//   concept: satisfied iff _T is a cli_name<> specialization.
template<typename _T>
concept cli_name_c = is_cli_name_v<_T>;


// ===========================================================================
// II.  cli_short_c
// ===========================================================================

// cli_short_c
//   concept: satisfied iff _T is a cli_short<> specialization.
template<typename _T>
concept cli_short_c = is_cli_short_v<_T>;


// ===========================================================================
// III. cli_descriptor_c
// ===========================================================================

// cli_descriptor_c
//   concept: NOMINAL match - satisfied iff _T is (publicly)
// derived from cli_descriptor.  Pairs with the polymorphic
// runtime registry.
template<typename _T>
concept cli_descriptor_c = is_cli_descriptor_v<_T>;


// ===========================================================================
// IV.  cli_descriptor_shape_c
// ===========================================================================

// cli_descriptor_shape_c
//   concept: STRUCTURAL match - satisfied iff _T exposes the six
// descriptor accessors with appropriate return types, regardless
// of whether it derives from cli_descriptor.  Use this when a
// generic helper should accept either the polymorphic base or a
// pure-template descriptor with the same shape.
template<typename _T>
concept cli_descriptor_shape_c = requires (const _T& _t)
{
    { _t.name()       } -> std::convertible_to<std::string_view>;
    { _t.short_form() } -> std::convertible_to<char>;
    { _t.kind()       } -> std::convertible_to<cli_kind>;
    { _t.arity()      } -> std::convertible_to<cli_arity>;
    { _t.help()       } -> std::convertible_to<std::string_view>;
    { _t.hidden()     } -> std::convertible_to<bool>;
};


NS_END  // djinterp


#endif  // DJINTERP_CLI_CONCEPTS_
