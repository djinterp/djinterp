/******************************************************************************
* djinterp [options]                                        option_concepts.hpp
*
*   C++20 concept counterparts for the option<> trait machinery in
* option_traits.hpp.
*
*   Naming: Capital-letter concepts (Option, UnaryOption, ArgsOption)
* per project convention.  Concept names parallel the corresponding
* traits without a suffix; the concept's shape mirrors the trait's
* condition, in `requires` form.
*
*   The framework's concepts do NOT depend on the args carried by an
* option<> - args are opaque to the subframework.  Custom option
* types satisfy Option iff they are some `option<...>` instantiation,
* full stop.  Users defining their own option-like types (e.g. a
* hypothetical `valued_option`) must EITHER specialize is_option for
* their type or provide a parallel concept under their own name.
*
*   Pre-2026.05.27 this header also exposed value-tag concepts
* (value_tag_c, value_like_c, option_has_value_c, option_has_arg_c).
* Those have been retired alongside the option_tag tag library.
*
*
* path:      /inc/djinterp/core/option/option_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
*                                                          revised: 2026.05.27
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Option                      (option<> detection)
II.   UnaryOption / ArgsOption    (composite: option with / without args)
*/

#ifndef DJINTERP_OPTION_CONCEPTS_
#define DJINTERP_OPTION_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./option.hpp"
#include "./option_traits.hpp"


// require C++20 for concepts
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    #error "option_concepts.hpp requires C++20 or later"
#endif

#if !D_ENV_CPP_FEATURE_LANG_CONCEPTS
    #error "option_concepts.hpp requires compiler support for concepts"
#endif


NS_DJINTERP


// ===========================================================================
// I.   Option
// ===========================================================================

// Option
//   concept: satisfied iff _Type is some option<...>
// specialization.  Parallels is_option_v<_Type>.
template<typename _Type>
concept Option = is_option_v<_Type>;


// ===========================================================================
// II.  UnaryOption / ArgsOption
// ===========================================================================

// UnaryOption
//   concept: satisfied iff _Type is a unary option - option<K>
// with no args.  Composite over Option + ::has_args == false.
//
//   Useful only as a SHAPE classifier (e.g. "this option carries
// no extra storage").  Carries no semantic about the option's
// role.
template<typename _Type>
concept UnaryOption =
    Option<_Type> &&
    requires
    {
        requires (_Type::has_args == false);
    };


// ArgsOption
//   concept: satisfied iff _Type is an option with at least one
// arg.  Complementary to UnaryOption.
//
//   What the args MEAN is outside the framework's scope - this
// concept reports only that there ARE args, not anything about
// their shape.
template<typename _Type>
concept ArgsOption =
    Option<_Type> &&
    requires
    {
        requires (_Type::has_args == true);
        typename _Type::args_type;
    };


NS_END  // djinterp


#endif  // DJINTERP_OPTION_CONCEPTS_
