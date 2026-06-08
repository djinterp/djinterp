/******************************************************************************
* djinterp [options]                                                option.hpp
*
*   The core option<> type and result sentinels.  Nothing else.
*
*     option<_Key, _Args...>
*
*   The key is a value (NTTP); everything after it is an "arg" type that
* is context-less by default - option<> itself imposes no meaning on
* what an arg represents.  Meaning is layered on top via CONTEXT TAGS
* (type wrappers that label the arg's role), introspected by the trait
* machinery in option_traits.hpp.
*
*   This header is intentionally minimal: depending only on it gets you
* the type without any of the trait apparatus.  See:
*     option_traits.hpp     - find_arg, option_find_arg, value<>,
*                             option_has_value, option_from_tuple, ...
*     option_concepts.hpp   - concept analogs of the above
*
*
* path:      /inc/djinterp/core/option/option.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    arg_not_found / arg_npos sentinels
II.   option core type
*/

#ifndef DJINTERP_OPTION_
#define DJINTERP_OPTION_ 1

// std
#include <cstddef>
#include <tuple>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
// I.   arg_not_found / arg_npos sentinels
// ===========================================================================

// arg_not_found
//   tag: result type emitted by find_arg / option_find_arg (defined
// in option_traits.hpp) when no arg in the pack matches the predicate.
// Distinguishable from any real tag.
struct arg_not_found
{};

// arg_npos
//   value: sentinel index returned when find_arg / option_find_arg
// misses.  Mirrors std::string::npos in spirit.
inline constexpr std::size_t arg_npos = static_cast<std::size_t>(-1);


// ===========================================================================
// II.  option
// ===========================================================================

// option
//   type: a key (NTTP) plus an opaque pack of "arg" types.  option<>
// itself imposes no meaning on the args; consumers extract structured
// information via the query traits in option_traits.hpp using
// per-context predicates.
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


NS_END  // djinterp


#endif  // DJINTERP_OPTION_