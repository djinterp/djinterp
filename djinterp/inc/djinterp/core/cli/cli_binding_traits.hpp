/******************************************************************************
* djinterp [cli]                                       cli_binding_traits.hpp
*
*   Trait machinery for cli_binding<>:
*
*     1. is_cli_binding<T>            - detect a cli_binding<...> specialization
*     2. cli_find_arg                 - generic predicate-based pack search
*     3. cli_binding_find_arg         - same, adapted to a binding's args
*     4. cli_binding_has_arg          - boolean form
*
*   Mirrors option_traits.hpp shape-for-shape; the find vocabulary is
* deliberately CLI-prefixed (cli_find_arg, cli_arg_not_found, cli_arg_npos)
* so the CLI module stays type-decoupled from the option module.  When /
* if a shared "find-by-predicate" facility lands in /meta, both modules
* can re-export it - until then, the duplication is explicit and OK.
*
*   ADDING A NEW CLI TAG follows the same four-piece pattern as
* option tags:
*
*     1. Define a tag struct
*     2. Define a predicate trait (is_<tag>)
*     3. Convenience extractor over a binding (cli_binding_<tag>_tag)
*     4. Convenience bool (cli_binding_has_<tag>_v)
*
*   See cli_binding_tags.hpp for the shipped tag library following this
* pattern.
*
*
* path:      /inc/djinterp/core/cli/cli_binding_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_cli_binding
II.   cli_find_arg                       (generic pack search)
III.  cli_binding_find_arg / has_arg     (binding-adapted)
*/

#ifndef DJINTERP_CLI_BINDING_TRAITS_
#define DJINTERP_CLI_BINDING_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"
#include "./cli_binding.hpp"


NS_DJINTERP


// ===========================================================================
// I.   is_cli_binding
// ===========================================================================

// is_cli_binding
//   trait: true iff _T is some cli_binding<_Key, _Name, _Args...>
// specialization.  Catches both the args-less form and the
// args form via a single _Args... pack that may be empty.
template<typename _T>
struct is_cli_binding : std::false_type
{};

template<auto         _Key,
         fixed_string _Name,
         typename...  _Args>
struct is_cli_binding<cli_binding<_Key, _Name, _Args...>>
    : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_binding_v = is_cli_binding<_T>::value;


// ===========================================================================
// II.  cli_find_arg
// ===========================================================================

// cli_find_arg
//   trait: finds the first type in the pack for which
// _Predicate<T>::value is true.  Members mirror std::find:
//
//     ::type   - the matching type, or cli_arg_not_found on miss.
//     ::found  - bool.
//     ::index  - position of the match, or cli_arg_npos on miss.
//
//   _Predicate is a single-parameter trait template (the same
// shape as std::is_pointer, std::is_class, etc.).  This lets
// callers pass any predicate they want without coupling
// cli_find_arg to a specific context.
template<template<typename> class _Predicate,
         typename...               _Args>
struct cli_find_arg;

// base case: empty pack
template<template<typename> class _Predicate>
struct cli_find_arg<_Predicate>
{
    using type = cli_arg_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = cli_arg_npos;
};

// recursive case
template<template<typename> class _Predicate,
         typename                  _Head,
         typename...               _Tail>
struct cli_find_arg<_Predicate, _Head, _Tail...>
{
private:
    static constexpr bool head_matches = _Predicate<_Head>::value;

    using next_t = cli_find_arg<_Predicate, _Tail...>;

public:
    using type =
        std::conditional_t<head_matches, _Head, typename next_t::type>;

    static constexpr bool found = (head_matches || next_t::found);

    static constexpr std::size_t index =
        head_matches
            ? std::size_t{0}
            : ( (next_t::index == cli_arg_npos)
                  ? cli_arg_npos
                  : (next_t::index + 1) );
};

template<template<typename> class _Predicate,
         typename...               _Args>
using cli_find_arg_t = typename cli_find_arg<_Predicate, _Args...>::type;


// ===========================================================================
// III. cli_binding_find_arg / cli_binding_has_arg
// ===========================================================================

// cli_binding_find_arg
//   adapter: applies cli_find_arg to a cli_binding's args.  Yields
// cli_arg_not_found / false / cli_arg_npos when the binding has
// no args at all (matching the empty-pack behavior of cli_find_arg).
template<typename                 _Binding,
         template<typename> class _Predicate>
struct cli_binding_find_arg;

// args-less binding: empty args -> miss
template<auto                     _Key,
         fixed_string             _Name,
         template<typename> class _Predicate>
struct cli_binding_find_arg<cli_binding<_Key, _Name>, _Predicate>
{
    using type = cli_arg_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = cli_arg_npos;
};

// binding with args
template<auto                      _Key,
         fixed_string              _Name,
         typename                  _First,
         typename...               _Rest,
         template<typename> class  _Predicate>
struct cli_binding_find_arg<cli_binding<_Key, _Name, _First, _Rest...>,
                            _Predicate>
{
private:
    using inner_t = cli_find_arg<_Predicate, _First, _Rest...>;

public:
    using type = typename inner_t::type;

    static constexpr bool        found = inner_t::found;
    static constexpr std::size_t index = inner_t::index;
};

template<typename                 _Binding,
         template<typename> class _Predicate>
using cli_binding_find_arg_t =
    typename cli_binding_find_arg<_Binding, _Predicate>::type;


// cli_binding_has_arg
//   trait: true iff some arg in the binding satisfies _Predicate.
template<typename                 _Binding,
         template<typename> class _Predicate>
struct cli_binding_has_arg
    : std::integral_constant<bool,
        cli_binding_find_arg<_Binding, _Predicate>::found>
{};

template<typename                 _Binding,
         template<typename> class _Predicate>
inline constexpr bool cli_binding_has_arg_v =
    cli_binding_has_arg<_Binding, _Predicate>::value;


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_TRAITS_
