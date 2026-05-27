/******************************************************************************
* djinterp [cli]                                     cli_binding_concepts.hpp
*
*   C++20 concept analogs of the cli_binding trait machinery.
*
*   Mirrors how option_concepts.hpp parallels option_traits.hpp.  Speaks
* about the SAME shapes as the corresponding traits, just in
* requires-clause form.
*
*
* path:      /inc/djinterp/core/cli/cli_binding_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cli_binding_c                       (cli_binding<> detection)
II.   args_cli_binding_c                  (with at least one arg)
III.  unary_cli_binding_c                 (no args)
IV.   cli_binding_has_arg_c               (predicate-parameterized)
*/

#ifndef DJINTERP_CLI_BINDING_CONCEPTS_
#define DJINTERP_CLI_BINDING_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./cli_binding.hpp"
#include "./cli_binding_traits.hpp"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "cli_binding_concepts.hpp requires C++20 or later"
#endif

#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "cli_binding_concepts.hpp requires compiler support for concepts"
#endif


NS_DJINTERP


// ===========================================================================
// I.   cli_binding_c
// ===========================================================================

// cli_binding_c
//   concept: satisfied iff _Type is some cli_binding<...>
// specialization.  Parallels is_cli_binding_v.
template<typename _Type>
concept cli_binding_c = is_cli_binding_v<_Type>;


// ===========================================================================
// II.  args_cli_binding_c
// ===========================================================================

// args_cli_binding_c
//   concept: satisfied iff _Type is a cli_binding with at least
// one arg.  Composite over cli_binding_c and `::has_args == true`.
template<typename _Type>
concept args_cli_binding_c =
    cli_binding_c<_Type> &&
    requires
    {
        requires (_Type::has_args == true);
        typename _Type::args_type;
    };


// ===========================================================================
// III. unary_cli_binding_c
// ===========================================================================

// unary_cli_binding_c
//   concept: satisfied iff _Type is a cli_binding with no args
// (name-only binding).  Complementary to args_cli_binding_c.
template<typename _Type>
concept unary_cli_binding_c =
    cli_binding_c<_Type> &&
    requires
    {
        requires (_Type::has_args == false);
    };


// ===========================================================================
// IV.  cli_binding_has_arg_c
// ===========================================================================

// cli_binding_has_arg_c
//   concept: satisfied iff _Binding has some arg matching
// _Predicate.  Parameterized over the unary predicate template,
// parallel to cli_binding_has_arg_v.
template<typename                 _Binding,
         template<typename> class _Predicate>
concept cli_binding_has_arg_c =
    cli_binding_c<_Binding> &&
    requires
    {
        requires cli_binding_has_arg_v<_Binding, _Predicate>;
    };


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_CONCEPTS_
