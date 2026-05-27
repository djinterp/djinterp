/******************************************************************************
* djinterp [cli]                                          cli_binding_tags.hpp
*
*   The starter library of built-in context tags for cli_binding<>.  Each
* tag is a type wrapper that gives one of a binding's args a named role:
*
*     cli_alias<_Name>          - an additional long-form name
*     cli_negate<_Name>         - an explicit negation form (e.g. --no-foo)
*     cli_help_override<_Text>  - override the help text the bridge would
*                                 otherwise pull from the option's
*                                 description<> tag
*     cli_metavar<_Name>        - placeholder name in --help output
*                                 (e.g. --output FILE -> FILE is the metavar)
*     cli_hidden                - omit from --help / usage (unary tag)
*     cli_positional            - force cli_kind::positional in the bridge,
*                                 even when the option's shape would
*                                 otherwise imply flag/value (unary tag)
*
*   cli_short<> (defined in cli.hpp) is ALSO a binding tag - its
*   detection trait is_cli_short_v (in cli_traits.hpp) and the binding
*   adapter / has-bool live here for consistency with the rest of the
*   tag library.
*
*   Each tag follows the four-piece pattern (matching option_tags.hpp):
*     1. the tag struct
*     2. is_<tag><T>  : std::false_type | std::true_type predicate
*     3. cli_binding_<tag>_tag<B>      - find-adapter alias
*     4. cli_binding_has_<tag>_v<B>    - bool convenience
*
*   Adding a new tag follows the same pattern.  Nothing here is registered
* centrally - the binding core in cli_binding.hpp has no idea any of
* these exist.
*
*
* path:      /inc/djinterp/core/cli/cli_binding_tags.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    cli_short<>            adapter (tag itself lives in cli.hpp)
II.   cli_alias<>
III.  cli_negate<>
IV.   cli_help_override<>
V.    cli_metavar<>
VI.   cli_hidden              (unary tag)
VII.  cli_positional          (unary tag)
*/

#ifndef DJINTERP_CLI_BINDING_TAGS_
#define DJINTERP_CLI_BINDING_TAGS_ 1

// std
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"
#include "./cli.hpp"
#include "./cli_traits.hpp"           // is_cli_short
#include "./cli_binding.hpp"
#include "./cli_binding_traits.hpp"   // cli_binding_find_arg


NS_DJINTERP


// ===========================================================================
// I.   cli_short<> binding adapter
// ===========================================================================

//   cli_short<> itself is defined in cli.hpp; is_cli_short / is_cli_short_v
// live in cli_traits.hpp.  The binding-adapter trio is here so all
// per-tag accessors stay in one place.

template<typename _Binding>
using cli_binding_short_tag =
    cli_binding_find_arg<_Binding, is_cli_short>;

template<typename _Binding>
using cli_binding_short_tag_t =
    typename cli_binding_short_tag<_Binding>::type;

template<typename _Binding>
inline constexpr bool cli_binding_has_short_v =
    cli_binding_short_tag<_Binding>::found;


// ===========================================================================
// II.  cli_alias<>
// ===========================================================================

// cli_alias
//   tag: an ADDITIONAL long-form CLI name the binding also
// matches against.  Multiple cli_alias<> tags per binding are
// fine; each adds one more recognized name.  The primary name
// remains the binding's _Name template parameter.
template<fixed_string _Name>
struct cli_alias
{
    static_assert(is_valid_cli_name(_Name),
                  "cli_alias: invalid CLI name (must be non-empty, "
                  "begin with an ASCII letter, and contain only "
                  "alphanumerics, '-', or '_').");

    static constexpr auto value = _Name;

    static constexpr std::string_view
    view() noexcept
    {
        return _Name.view();
    }
};

template<typename _T>
struct is_cli_alias : std::false_type
{};

template<fixed_string _N>
struct is_cli_alias<cli_alias<_N>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_alias_v = is_cli_alias<_T>::value;

template<typename _Binding>
using cli_binding_alias_tag =
    cli_binding_find_arg<_Binding, is_cli_alias>;

template<typename _Binding>
using cli_binding_alias_tag_t =
    typename cli_binding_alias_tag<_Binding>::type;

template<typename _Binding>
inline constexpr bool cli_binding_has_alias_v =
    cli_binding_alias_tag<_Binding>::found;


// ===========================================================================
// III. cli_negate<>
// ===========================================================================

// cli_negate
//   tag: explicit negation name for this binding.  When parsing
// sees this name, it sets the bound option's effective value to
// the negation of its actual<> (boolean options) or to a per-
// option-defined "off" state.  Typically used on flag-shaped
// options to provide the conventional --no-X form.
//
//   The bridge synthesizes the relationship for opposing_unary_pair
// expansions automatically (each side becomes a binding with
// implicit negation pointing at the other), but cli_negate is the
// hand-rolled form for non-pair bindings.
template<fixed_string _Name>
struct cli_negate
{
    static_assert(is_valid_cli_name(_Name),
                  "cli_negate: invalid CLI name.");

    static constexpr auto value = _Name;

    static constexpr std::string_view
    view() noexcept
    {
        return _Name.view();
    }
};

template<typename _T>
struct is_cli_negate : std::false_type
{};

template<fixed_string _N>
struct is_cli_negate<cli_negate<_N>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_negate_v = is_cli_negate<_T>::value;

template<typename _Binding>
using cli_binding_negate_tag =
    cli_binding_find_arg<_Binding, is_cli_negate>;

template<typename _Binding>
using cli_binding_negate_tag_t =
    typename cli_binding_negate_tag<_Binding>::type;

template<typename _Binding>
inline constexpr bool cli_binding_has_negate_v =
    cli_binding_negate_tag<_Binding>::found;


// ===========================================================================
// IV.  cli_help_override<>
// ===========================================================================

// cli_help_override
//   tag: replaces, FOR THIS CLI SURFACE ONLY, the help text the
// bridge would otherwise extract from the option's description<>.
// Useful when the option's description is internal-oriented and
// the CLI help needs different (or localized) wording without
// touching the option schema.
template<fixed_string _Text>
struct cli_help_override
{
    static constexpr auto value = _Text;

    static constexpr std::string_view
    view() noexcept
    {
        return _Text.view();
    }
};

template<typename _T>
struct is_cli_help_override : std::false_type
{};

template<fixed_string _S>
struct is_cli_help_override<cli_help_override<_S>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_help_override_v =
    is_cli_help_override<_T>::value;

template<typename _Binding>
using cli_binding_help_override_tag =
    cli_binding_find_arg<_Binding, is_cli_help_override>;

template<typename _Binding>
using cli_binding_help_override_tag_t =
    typename cli_binding_help_override_tag<_Binding>::type;

template<typename _Binding>
inline constexpr bool cli_binding_has_help_override_v =
    cli_binding_help_override_tag<_Binding>::found;


// ===========================================================================
// V.   cli_metavar<>
// ===========================================================================

// cli_metavar
//   tag: placeholder name shown in --help output to indicate where
// a value belongs.  For example, `--output FILE` has metavar
// "FILE".  Has no parsing semantics; purely cosmetic.
//
//   No validation beyond non-emptiness - metavars are display
// strings, not CLI names, so they may contain uppercase, spaces,
// or punctuation.
template<fixed_string _Name>
struct cli_metavar
{
    static_assert(_Name.size() > 0,
                  "cli_metavar: must be non-empty.");

    static constexpr auto value = _Name;

    static constexpr std::string_view
    view() noexcept
    {
        return _Name.view();
    }
};

template<typename _T>
struct is_cli_metavar : std::false_type
{};

template<fixed_string _N>
struct is_cli_metavar<cli_metavar<_N>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_metavar_v = is_cli_metavar<_T>::value;

template<typename _Binding>
using cli_binding_metavar_tag =
    cli_binding_find_arg<_Binding, is_cli_metavar>;

template<typename _Binding>
using cli_binding_metavar_tag_t =
    typename cli_binding_metavar_tag<_Binding>::type;

template<typename _Binding>
inline constexpr bool cli_binding_has_metavar_v =
    cli_binding_metavar_tag<_Binding>::found;


// ===========================================================================
// VI.  cli_hidden  (unary tag)
// ===========================================================================

// cli_hidden
//   tag: presence-only.  Marks a binding as hidden from --help /
// usage output.  The option remains parseable; it just doesn't
// appear in documentation.
struct cli_hidden
{};

template<typename _T>
struct is_cli_hidden
    : std::integral_constant<bool, std::is_same<_T, cli_hidden>::value>
{};

template<typename _T>
inline constexpr bool is_cli_hidden_v = is_cli_hidden<_T>::value;

template<typename _Binding>
using cli_binding_hidden_tag =
    cli_binding_find_arg<_Binding, is_cli_hidden>;

template<typename _Binding>
inline constexpr bool cli_binding_has_hidden_v =
    cli_binding_hidden_tag<_Binding>::found;


// ===========================================================================
// VII. cli_positional  (unary tag)
// ===========================================================================

// cli_positional
//   tag: presence-only.  Forces the bridge to report cli_kind::positional
// for this binding, regardless of what the underlying option's shape
// (val_type<>, actual<>, default_<>, ...) would otherwise imply.
//
//   Use sparingly: most positionals also need cli_metavar<> to look
// sensible in --help output.
struct cli_positional
{};

template<typename _T>
struct is_cli_positional
    : std::integral_constant<bool, std::is_same<_T, cli_positional>::value>
{};

template<typename _T>
inline constexpr bool is_cli_positional_v = is_cli_positional<_T>::value;

template<typename _Binding>
using cli_binding_positional_tag =
    cli_binding_find_arg<_Binding, is_cli_positional>;

template<typename _Binding>
inline constexpr bool cli_binding_has_positional_v =
    cli_binding_positional_tag<_Binding>::found;


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_TAGS_
