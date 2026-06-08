/******************************************************************************
* djinterp [options]                                      option_set_traits.hpp
*
*   Trait machinery OVER an instantiated option_set<>:
*
*     1. is_option_set<T>          - detect "T is an option_set<...>"
*     2. option_set_key_type<Set>  - the set's single key_type
*     3. option_set_contains       - boolean "set has this key"
*     4. option_set_find           - lookup the option with key K
*
*   The set-CONSTRUCTION traits (is_keyed, expand_option, internals) live
* in option_set.hpp because they are part of the contract option_set
* enforces - not part of querying an existing one.
*
*   Comparison / equality / value-extraction traits live in
* option_set_compare.hpp.
*
*   Concept analogs in option_set_concepts.hpp.
*
*
* path:      /inc/djinterp/core/option/option_set_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_option_set        (option_set<> specialization detection)
II.   option_set_key_type  (single key_type extraction)
III.  option_set_contains  (boolean key presence)
IV.   option_set_find      (option lookup by key)
*/

#ifndef DJINTERP_OPTION_SET_TRAITS_
#define DJINTERP_OPTION_SET_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../util/lookup.hpp"      // contains_key, find_by_key
#include "./option_set.hpp"


NS_DJINTERP


// ===========================================================================
// I.   is_option_set
// ===========================================================================

// is_option_set
//   trait: true iff _Type is some option_set<...> specialization.
template<typename _Type>
struct is_option_set : std::false_type
{};

template<typename... _Options>
struct is_option_set<option_set<_Options...>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_option_set_v = is_option_set<clean_t<_Type>>::value;


// ===========================================================================
// II.  option_set_key_type
// ===========================================================================

// option_set_key_type
//   trait: extracts the key_type of a non-empty option_set from the
// head of its normalized option tuple.
template<typename _Set>
struct option_set_key_type;

template<typename... _Options>
    requires (option_set<_Options...>::size > 0)
struct option_set_key_type<option_set<_Options...>>
{
private:
    using head_option =
        typename option_set<_Options...>::template option_at<0>;
public:
    using type = typename head_option::key_type;
};

template<typename _Set>
using option_set_key_type_t = typename option_set_key_type<_Set>::type;


// ===========================================================================
// III. option_set_contains
// ===========================================================================

// option_set_contains
//   trait: true iff the set has an option with key _Key.  Walks the
// flat (post-expansion) tuple.
template<typename _Set, auto _Key>
struct option_set_contains;

template<typename... _Options, auto _Key>
struct option_set_contains<option_set<_Options...>, _Key>
{
private:
    using flat = typename option_set<_Options...>::flat_options_t;

    template<typename _Tuple>
    struct apply;

    template<typename... _Opts>
    struct apply<std::tuple<_Opts...>>
        : std::integral_constant<bool,
            contains_key<_Key, _Opts...>::value>
    {};

public:
    static constexpr bool value = apply<flat>::value;
};

template<typename _Set, auto _Key>
inline constexpr bool option_set_contains_v =
    option_set_contains<_Set, _Key>::value;


// ===========================================================================
// IV.  option_set_find
// ===========================================================================

// option_set_find
//   trait: yields the option with key _Key, or lookup_not_found.
template<typename _Set, auto _Key>
struct option_set_find;

template<typename... _Options, auto _Key>
struct option_set_find<option_set<_Options...>, _Key>
{
private:
    using flat = typename option_set<_Options...>::flat_options_t;

    template<typename _Tuple>
    struct apply;

    template<typename... _Opts>
    struct apply<std::tuple<_Opts...>>
    {
        using type  = find_by_key_t<_Key, _Opts...>;
        static constexpr bool        found = find_by_key<_Key, _Opts...>::found;
        static constexpr std::size_t index = find_by_key<_Key, _Opts...>::index;
    };

public:
    using type = typename apply<flat>::type;

    static constexpr bool        found = apply<flat>::found;
    static constexpr std::size_t index = apply<flat>::index;
};

template<typename _Set, auto _Key>
using option_set_find_t = typename option_set_find<_Set, _Key>::type;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_TRAITS_
