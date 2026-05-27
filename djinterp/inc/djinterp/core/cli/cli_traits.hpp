/******************************************************************************
* djinterp [cli]                                                cli_traits.hpp
*
*   Trait machinery for the cli.hpp vocabulary types:
*
*     1. is_cli_name<T>        - detect "T is cli_name<...>"
*     2. is_cli_short<T>       - detect "T is cli_short<...>"
*     3. is_cli_descriptor<T>  - detect "T derives from cli_descriptor"
*
*   Concept analogs of these live in cli_concepts.hpp.  Higher-level
* binding traits live in cli_binding_traits.hpp.
*
*
* path:      /inc/djinterp/core/cli/cli_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_cli_name
II.   is_cli_short
III.  is_cli_descriptor
*/

#ifndef DJINTERP_CLI_TRAITS_
#define DJINTERP_CLI_TRAITS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"
#include "./cli.hpp"


NS_DJINTERP


// ===========================================================================
// I.   is_cli_name
// ===========================================================================

// is_cli_name
//   trait: true iff _T is some cli_name<...> specialization.
template<typename _T>
struct is_cli_name : std::false_type
{};

template<fixed_string _S>
struct is_cli_name<cli_name<_S>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_name_v = is_cli_name<_T>::value;


// ===========================================================================
// II.  is_cli_short
// ===========================================================================

// is_cli_short
//   trait: true iff _T is some cli_short<...> specialization.
template<typename _T>
struct is_cli_short : std::false_type
{};

template<char _C>
struct is_cli_short<cli_short<_C>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_short_v = is_cli_short<_T>::value;


// ===========================================================================
// III. is_cli_descriptor
// ===========================================================================

// is_cli_descriptor
//   trait: true iff _T (publicly) derives from the runtime
// cli_descriptor base.  Useful in SFINAE for registry helpers
// that require the polymorphic form.
template<typename _T>
struct is_cli_descriptor
    : std::integral_constant<bool,
        std::is_base_of<cli_descriptor, _T>::value>
{};

template<typename _T>
inline constexpr bool is_cli_descriptor_v = is_cli_descriptor<_T>::value;


NS_END  // djinterp


#endif  // DJINTERP_CLI_TRAITS_
