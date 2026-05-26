/******************************************************************************
* djinterp [options]                                        option_concepts.hpp
*
*   C++20 concept analogs of the option<> trait machinery in
* option_traits.hpp.
*
*   Concepts here are designed as a PARALLEL FACILITY to the traits, not
* a layer on top.  Callers may freely use either:
*     - requires clauses + concepts -> option_concepts.hpp
*     - SFINAE + traits             -> option_traits.hpp
*   Both speak about the same shapes.  The concept names mirror the
* corresponding traits with a `_c` suffix for type-classifier concepts
* and bare `has_*` for compositional ones.
*
*
* path:      /inc/djinterp/core/options/option_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    option detection concepts            (option_c, unary_option_c, ...)
II.   value-tag concepts                   (value_tag_c, option_has_value_c)
III.  arg-predicate composite concepts     (option_has_arg_c)
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
// I.   option detection concepts
// ===========================================================================

// option_c
//   concept: satisfied iff _Type is some option<...> specialization.
// Parallels is_option_v<_Type>.
template<typename _Type>
concept option_c = is_option_v<_Type>;

// unary_option_c
//   concept: satisfied iff _Type is a unary option (option<K> with no
// args).  Composite over option_c and `::has_args == false`.
template<typename _Type>
concept unary_option_c =
    option_c<_Type> &&
    requires
    {
        requires (_Type::has_args == false);
    };

// args_option_c
//   concept: satisfied iff _Type is an option with at least one arg.
// Complementary to unary_option_c.
template<typename _Type>
concept args_option_c =
    option_c<_Type> &&
    requires
    {
        requires (_Type::has_args == true);
        typename _Type::args_type;
    };


// ===========================================================================
// II.  value-tag concepts
// ===========================================================================

// value_tag_c
//   concept: satisfied iff _Type is an instantiation of the built-in
// value<> tag.  Parallels is_value_v.
template<typename _Type>
concept value_tag_c = is_value_v<_Type>;

// value_like_c
//   concept: structural shape match for "any tag that carries a
// value" - exposes ::value_type and ::the_value.  Useful when you
// want polymorphism over value-carrying tags (actual<>, default_<>,
// value<>, custom user tags) without nominal coupling to value<>.
template<typename _Type>
concept value_like_c = requires
{
    typename _Type::value_type;
    { _Type::the_value } -> std::convertible_to<typename _Type::value_type>;
};

// option_has_value_c
//   concept: satisfied iff _Opt is an option that carries a value<>
// tag among its args.  Parallels option_has_value_v.
template<typename _Opt>
concept option_has_value_c =
    option_c<_Opt> &&
    requires
    {
        requires option_has_value_v<_Opt>;
    };


// ===========================================================================
// III. arg-predicate composite concepts
// ===========================================================================

// option_has_arg_c
//   concept: satisfied iff _Opt has some arg matching _Predicate.
// Parameterized over the unary predicate template, parallel to
// option_has_arg_v.
template<typename                 _Opt,
         template<typename> class _Predicate>
concept option_has_arg_c =
    option_c<_Opt> &&
    requires
    {
        requires option_has_arg_v<_Opt, _Predicate>;
    };


NS_END  // djinterp


#endif  // DJINTERP_OPTION_CONCEPTS_
