/******************************************************************************
* djinterp [option]                                                 option.hpp
*
*   The core option<> type, its detection trait, and the C++20 concept
* analogs - everything that speaks about a SINGLE option<>, in one header.
*
*     option<_Key, _Args...>
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    arg_not_found / arg_npos            (reserved arg-search sentinels)
II.   option                              (core type)
III.  is_option                           (option<> specialization detection)
IV.   Option / UnaryOption / ArgsOption   (C++20 concept analogs)
*/

#ifndef DJINTERP_OPTION_
#define DJINTERP_OPTION_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


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
inline constexpr std::size_t arg_npos = static_cast<std::size_t>(-1);


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
template<auto        _Key,
         typename... _Args>
struct option;

// unary form
template<auto _Key>
struct option<_Key>
{
    using key_type = decltype(_Key);

    static constexpr key_type    key       = _Key;
    static constexpr bool        has_args  = false;
    static constexpr std::size_t arg_count = 0;
};

// args form (1+ args)
//   Written as <_Key, _First, _Rest...> so it is strictly more
// specialized than the primary template.  <_Key, _Args...> would be
// identical to the primary's signature and rejected by the compiler
// as a non-specialization.
template<auto        _Key,
         typename    _First,
         typename... _Rest>
struct option<_Key, _First, _Rest...>
{
    using key_type  = decltype(_Key);
    using args_type = std::tuple<_First, _Rest...>;

    static constexpr key_type    key       = _Key;
    static constexpr bool        has_args  = true;
    static constexpr std::size_t arg_count = (sizeof...(_Rest) + 1);
};


// ===========================================================================
// III. is_option
// ===========================================================================

// is_option
//   trait: true iff _Type is some option<_Key, _Args...>
// specialization.  Catches both the unary form (option<K>)
// and the args form (option<K, A, B, ...>) via a single
// _Args... pack that may be empty.
template<typename _Type>
struct is_option : std::false_type
{};

template<auto        _Key,
         typename... _Args>
struct is_option<option<_Key, _Args...>> : std::true_type
{};

template<typename _Type>
D_CONSTEXPR_VAR bool is_option_v = is_option<clean_t<_Type>>::value;


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
//   concept: satisfied iff _Type is some option<...> specialization.
// Parallels is_option_v<_Type>.
template<typename _Type>
concept Option = is_option_v<_Type>;


// UnaryOption
//   concept: satisfied iff _Type is a unary option - option<K> with no
// args.  Composite over Option + ::has_args == false.  A SHAPE
// classifier only ("this option carries no extra storage"); carries no
// semantic about the option's role.
template<typename _Type>
concept UnaryOption =
    Option<_Type> &&
    requires
    {
        requires (_Type::has_args == false);
    };


// ArgsOption
//   concept: satisfied iff _Type is an option with at least one arg.
// Complementary to UnaryOption.  Reports only that there ARE args, not
// anything about their shape.
template<typename _Type>
concept ArgsOption =
    Option<_Type> &&
    requires
    {
        requires (_Type::has_args == true);
        typename _Type::args_type;
    };

#endif  // C++20 concepts available


NS_END  // djinterp


#endif  // DJINTERP_OPTION_
