/*******************************************************************************
* djinterp [core]                                                     option.hpp
*
*   The core option<> type, its detection trait, and the C++20 concept
* analogs - everything that speaks about a SINGLE option<>, in one header.
*
*     option<Key, Args...>
*
*   The key is a value (NTTP); everything after it is an "arg" type that is
* context-less by default - option<> itself imposes no meaning on what an
* arg represents.  Meaning is layered on top by the consumer (e.g. the
* context tags in option_tags.hpp), never by option<> itself.
*
*   This header provides, in order:
*     - arg_not_found / arg_npos : reserved arg-search sentinels.
*     - option<>                 : the core type (unary + args forms).
*     - is_option / is_option_v  : "is this some option<...>?" detection.
*     - Option / UnaryOption /
*       ArgsOption               : C++20 concept analogs, compiled only
*                                  where the toolchain supports concepts.
*
*   HISTORY:
*   The pre-2026.05.27 design shipped tag-driven args-search machinery
* (find_arg, option_find_arg, option_has_arg, value<>, option_from_tuple)
* in a separate option_traits.hpp.  All of that was retired; options now
* carry an NTTP key followed by an OPAQUE arg pack, and slot positioning
* is handled by the build pipeline (option_builder.hpp), not by per-slot
* tag search.  The surviving is_option trait (formerly option_traits.hpp)
* and the option concepts (formerly option_concepts.hpp) now live here,
* alongside the type they describe.
*
*
* path:      /inc/djinterp/core/option/option.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.24
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    arg_not_found / arg_npos            (reserved arg-search sentinels)
      -------------------------------------------------------------------

II.   option                              (core type)
      -----------------------------------------------

III.  is_option                           (option<> specialization detection)
      -----------------------------------------------------------------------

IV.   Option / UnaryOption / ArgsOption   (C++20 concept analogs)
      -----------------------------------------------------------
*/

#ifndef DJINTERP_OPTION_OPTION_HPP
#define DJINTERP_OPTION_OPTION_HPP 1


//   THE MODULE FLOOR.
//
//   C++17 rather than the framework floor of C++11, and it is not a
// preference. option<> is declared `template<auto Key, typename... Args>`
// (below, four times) and a deduced non-type template parameter is a C++17
// feature. There is no C++11 or C++14 spelling of this type to fall back to,
// so the floor is stated rather than worked around -- per D1, a fallback here
// would be a second implementation nobody compiles. Below C++17 the header
// degrades rather than errors (the owner's ruling of 2026.09.30): it
// compiles to nothing, like every header of the family, each of which
// gates itself the same way.
//
//   The C++20 concept analogs further down are an ADDITION on top of this
// floor, gated on D_ENV_LANG_IS_CPP20_OR_HIGHER, which is D1 working as
// intended: a higher tier adds capability, it does not change behaviour.
//
//   This is the option subframework's root -- option_set, option_diff,
// option_compose, option_factory, option_generator, option_override,
// option_set_compare, option_builder and optionator all reach it before any
// C++17 construct of their own, so stating it here covers the family.

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER


// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../meta/type_utility.hpp"  // clean_t


NS_DJINTERP


// ===========================================================================
// I.   arg_not_found / arg_npos sentinels
// ===========================================================================

// arg_not_found
//   tag: reserved result type for an arg-search miss, distinguishable
// from any real tag.  The framework's own tag-driven arg search was
// retired (2026.05.27); this remains as a sentinel for user-defined
// arg-search helpers.
struct arg_not_found
{};

// arg_npos
//   value: reserved sentinel index for an arg-search miss.  Mirrors
// std::string::npos in spirit.
D_CONSTEXPR_INLINE_VAR std::size_t arg_npos = static_cast<std::size_t>(-1);


// ===========================================================================
// II.  option
// ===========================================================================

// option
//   type: a key (NTTP) plus an opaque pack of "arg" types.  option<>
// itself imposes no meaning on the args; consumers attach meaning via
// their own context tags.
//
// Example:
//   option<window_opt::title>
//   option<window_opt::title, value<"Untitled">>
//   option<window_opt::title, value<"Untitled">, verifier<&fn>>
template<auto        Key,
         typename... Args>
struct option;

// unary form
template<auto Key>
struct option<Key>
{
    using key_type = decltype(Key);

    static constexpr key_type    key       = Key;
    static constexpr bool        has_args  = false;
    static constexpr std::size_t arg_count = 0;
};

// args form (1+ args)
//   Written as <Key, First, Rest...> so it is strictly more
// specialized than the primary template.  <Key, Args...> would be
// identical to the primary's signature and rejected by the compiler
// as a non-specialization.
template<auto        Key,
         typename    First,
         typename... Rest>
struct option<Key, First, Rest...>
{
    using key_type  = decltype(Key);
    using args_type = std::tuple<First, Rest...>;

    static constexpr key_type    key       = Key;
    static constexpr bool        has_args  = true;
    static constexpr std::size_t arg_count = (sizeof...(Rest) + 1);
};


// ===========================================================================
// III. is_option
// ===========================================================================

// is_option
//   trait: true iff Type is some option<Key, Args...>
// specialization.  Catches both the unary form (option<K>)
// and the args form (option<K, A, B, ...>) via a single
// Args... pack that may be empty.
template<typename Type>
struct is_option : std::false_type
{};

template<auto        Key,
         typename... Args>
struct is_option<option<Key, Args...>> : std::true_type
{};

template<typename Type>
D_CONSTEXPR bool is_option_v = is_option<clean_t<Type>>::value;


// ===========================================================================
// IV.  Option / UnaryOption / ArgsOption   (C++20 concept analogs)
// ===========================================================================
//
//   Concept analogs of is_option_v, compiled only where the toolchain
// provides concepts.  Pre-C++20 they are simply absent and the trait
// above remains the portable detection path.  Naming follows the
// project's capital-letter concept convention; each concept's shape
// mirrors the corresponding trait condition in `requires` form.  The
// concepts do NOT depend on the args an option carries - args are opaque
// to the subframework.

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// Option
//   concept: satisfied iff Type is some option<...> specialization.
// Parallels is_option_v<Type>.
template<typename Type>
concept Option = is_option_v<Type>;


// UnaryOption
//   concept: satisfied iff Type is a unary option - option<K> with no
// args.  Composite over Option + ::has_args == false.  A SHAPE
// classifier only ("this option carries no extra storage"); carries no
// semantic about the option's role.
template<typename Type>
concept UnaryOption =
    Option<Type> &&
    requires
    {
        requires (Type::has_args == false);
    };


// ArgsOption
//   concept: satisfied iff Type is an option with at least one arg.
// Complementary to UnaryOption.  Reports only that there ARE args, not
// anything about their shape.
template<typename Type>
concept ArgsOption =
    Option<Type> &&
    requires
    {
        requires (Type::has_args == true);
        typename Type::args_type;
    };

#endif  // C++20 concepts available


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTION_HPP
