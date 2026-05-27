/******************************************************************************
* djinterp [cli]                                               cli_binding.hpp
*
*   The core cli_binding<> type and its result sentinels.  Nothing else.
*
*     cli_binding<_Key, _Name, _Args...>
*
*   The key is a value (NTTP) matching the option-side key universe;
* the name is a validated long-form CLI name (fixed_string), enforced
* at compile time; everything after is an opaque arg pack of "tag"
* types in the same open-world spirit as option<>.  cli_binding itself
* imposes no meaning on what an arg represents.  Meaning is layered on
* top via CONTEXT TAGS (e.g. cli_alias, cli_negate, cli_hidden,
* cli_short, cli_metavar, cli_help_override - see cli_binding_tags.hpp)
* introspected by the trait machinery in cli_binding_traits.hpp.
*
*   This header is intentionally minimal: depending only on it gets you
* the type and its sentinels without any trait apparatus.  See:
*     cli_binding_traits.hpp    - cli_binding_find_arg, has_arg, ...
*     cli_binding_concepts.hpp  - concept analogs
*     cli_binding_tags.hpp      - shipped tag library
*     cli_binding_set.hpp       - the set container
*
*
* path:      /inc/djinterp/core/cli/cli_binding.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cli_arg_not_found / cli_arg_npos sentinels
II.   cli_binding core type
*/

#ifndef DJINTERP_CLI_BINDING_
#define DJINTERP_CLI_BINDING_ 1

// std
#include <cstddef>
#include <string_view>
#include <tuple>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"
#include "./cli.hpp"


NS_DJINTERP


// ===========================================================================
// I.   cli_arg_not_found / cli_arg_npos sentinels
// ===========================================================================

// cli_arg_not_found
//   tag: result type emitted by the cli_binding_find_arg trait
// (defined in cli_binding_traits.hpp) when no arg in the binding's
// pack matches the predicate.  Distinguishable from any real tag.
//
//   Intentionally separate from option.hpp's arg_not_found - the
// CLI module is fully decoupled from the option module, and the
// two find-result vocabularies must not be coupled at the type
// level just because they happen to play the same structural role.
struct cli_arg_not_found
{};

// cli_arg_npos
//   value: sentinel index returned when cli_binding_find_arg misses.
// Mirrors std::string::npos in spirit.
inline constexpr std::size_t cli_arg_npos = static_cast<std::size_t>(-1);


// ===========================================================================
// II.  cli_binding
// ===========================================================================

// cli_binding
//   type: a key (NTTP) + a long-form CLI name (fixed_string) +
// an opaque pack of "arg" tag types.  cli_binding itself imposes
// no meaning on the args; consumers extract structured information
// via the query traits in cli_binding_traits.hpp using per-context
// predicates.
//
//   The primary name is validated against is_valid_cli_name at
// compile time.  Aliases, negations, short forms, help overrides
// and similar metadata live in the args pack as tags.
//
// Example:
//   cli_binding<window_opt::title, "title">
//   cli_binding<window_opt::title, "title", cli_short<'t'>>
//   cli_binding<window_opt::title, "title", cli_short<'t'>,
//               cli_alias<"window-title">,
//               cli_metavar<"TEXT">>
template<auto         _Key,
         fixed_string _Name,
         typename...  _Args>
struct cli_binding
{
    static_assert(is_valid_cli_name(_Name),
                  "cli_binding: invalid long-form CLI name.  Must "
                  "be non-empty, begin with an ASCII letter, and "
                  "contain only alphanumerics, '-', or '_'.");

    using key_type = decltype(_Key);

    static constexpr key_type    key       = _Key;
    static constexpr auto        name      = _Name;
    static constexpr std::size_t name_size = _Name.size();
    static constexpr bool        has_args  = (sizeof...(_Args) > 0);
    static constexpr std::size_t arg_count = sizeof...(_Args);

    // args_type
    //   alias: tuple of the binding's args, useful for tuple-walking
    // helpers.  An empty pack yields std::tuple<>.
    using args_type = std::tuple<_Args...>;

    // name_view
    //   accessor: runtime-friendly string_view over the primary
    // long-form CLI name (without leading dashes).
    static constexpr std::string_view
    name_view() noexcept
    {
        return _Name.view();
    }
};


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_
