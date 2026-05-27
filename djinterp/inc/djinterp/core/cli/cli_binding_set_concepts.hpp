/******************************************************************************
* djinterp [cli]                                  cli_binding_set_concepts.hpp
*
*   C++20 concept analogs of the cli_binding_set trait machinery.
*
*   Parallels option_set_concepts.hpp.  Concepts here speak about the
* SAME shapes as the corresponding traits, just in requires-clause form.
*
*
* path:      /inc/djinterp/core/cli/cli_binding_set_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cli_binding_set_c                    (specialization detection)
II.   cli_binding_set_contains_key_c       (NTTP-parameterized)
III.  cli_binding_set_contains_name_c      (alias-aware)
IV.   cli_binding_set_contains_short_c     (char-parameterized)
V.    cli_binding_set_nonempty_c           (composite shape)
*/

#ifndef DJINTERP_CLI_BINDING_SET_CONCEPTS_
#define DJINTERP_CLI_BINDING_SET_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"
#include "./cli_binding_set.hpp"
#include "./cli_binding_set_traits.hpp"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "cli_binding_set_concepts.hpp requires C++20 or later"
#endif

#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "cli_binding_set_concepts.hpp requires compiler support for concepts"
#endif


NS_DJINTERP


// ===========================================================================
// I.   cli_binding_set_c
// ===========================================================================

// cli_binding_set_c
//   concept: satisfied iff _Type is some cli_binding_set<...>
// specialization.  Parallels is_cli_binding_set_v.
template<typename _Type>
concept cli_binding_set_c = is_cli_binding_set_v<_Type>;


// ===========================================================================
// II.  cli_binding_set_contains_key_c
// ===========================================================================

// cli_binding_set_contains_key_c
//   concept: satisfied iff _Set is a cli_binding_set that
// contains a binding with the key _Key.  Parameterized over
// the NTTP key for use in requires-clauses.
template<typename _Set, auto _Key>
concept cli_binding_set_contains_key_c =
    cli_binding_set_c<_Set> &&
    requires
    {
        requires cli_binding_set_contains_key_v<_Set, _Key>;
    };


// ===========================================================================
// III. cli_binding_set_contains_name_c
// ===========================================================================

// cli_binding_set_contains_name_c
//   concept: satisfied iff _Set contains a binding whose primary
// name, any cli_alias<>, or cli_negate<> matches _Name.  ALIAS /
// NEGATION AWARE.
template<typename _Set, fixed_string _Name>
concept cli_binding_set_contains_name_c =
    cli_binding_set_c<_Set> &&
    requires
    {
        requires cli_binding_set_contains_name_v<_Set, _Name>;
    };


// ===========================================================================
// IV.  cli_binding_set_contains_short_c
// ===========================================================================

// cli_binding_set_contains_short_c
//   concept: satisfied iff _Set contains a binding whose
// cli_short<> matches _C.
template<typename _Set, char _C>
concept cli_binding_set_contains_short_c =
    cli_binding_set_c<_Set> &&
    requires
    {
        requires cli_binding_set_contains_short_v<_Set, _C>;
    };


// ===========================================================================
// V.   cli_binding_set_nonempty_c
// ===========================================================================

// cli_binding_set_nonempty_c
//   concept: satisfied iff _Set is a non-empty cli_binding_set.
// Pairs naturally with cli_binding_set_key_type_t.
template<typename _Set>
concept cli_binding_set_nonempty_c =
    cli_binding_set_c<_Set> &&
    requires
    {
        requires (_Set::size > 0);
    };


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_SET_CONCEPTS_
