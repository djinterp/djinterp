/******************************************************************************
* djinterp [cli]                                    cli_binding_set_traits.hpp
*
*   Trait machinery OVER an instantiated cli_binding_set<>:
*
*     1. is_cli_binding_set<T>             - detect a specialization
*     2. cli_binding_set_key_type<Set>     - the set's single key_type
*     3. cli_binding_set_contains_key      - boolean "set has this key"
*     4. cli_binding_set_find_by_key       - lookup by key NTTP
*     5. cli_binding_set_find_by_name      - lookup by long-form name,
*                                            ALIAS / NEGATION aware
*     6. cli_binding_set_find_by_short     - lookup by short-form char
*
*   The lookup-by-name trait walks each binding's primary name AND any
* cli_alias<> / cli_negate<> tags, so the SAME binding can be reached
* by any of its registered names.  This is what makes the bridge / parser
* able to dispatch "--no-verbose" to the verbose binding's negation arm.
*
*
* path:      /inc/djinterp/core/cli/cli_binding_set_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_cli_binding_set
II.   cli_binding_set_key_type
III.  per-binding name match    (primary / aliases / negation)
IV.   cli_binding_set_contains_key + find_by_key
V.    cli_binding_set_contains_name + find_by_name   (alias-aware)
VI.   cli_binding_set_contains_short + find_by_short
*/

#ifndef DJINTERP_CLI_BINDING_SET_TRAITS_
#define DJINTERP_CLI_BINDING_SET_TRAITS_ 1

// std
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "../meta/fixed_string.hpp"
#include "./cli.hpp"
#include "./cli_binding.hpp"
#include "./cli_binding_traits.hpp"
#include "./cli_binding_tags.hpp"       // alias / negate / short adapters
#include "./cli_binding_set.hpp"


NS_DJINTERP


// ===========================================================================
// I.   is_cli_binding_set
// ===========================================================================

// is_cli_binding_set
//   trait: true iff _T is some cli_binding_set<...> specialization.
template<typename _T>
struct is_cli_binding_set : std::false_type
{};

template<typename... _Bindings>
struct is_cli_binding_set<cli_binding_set<_Bindings...>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_cli_binding_set_v = is_cli_binding_set<_T>::value;


// ===========================================================================
// II.  cli_binding_set_key_type
// ===========================================================================

// cli_binding_set_key_type
//   trait: extracts the key_type of a non-empty cli_binding_set
// from the head of its bindings tuple.  All bindings in a set
// share the same key_type (enforced by run_binding_set_checks).
template<typename _Set>
struct cli_binding_set_key_type;

template<typename _First,
         typename... _Rest>
struct cli_binding_set_key_type<cli_binding_set<_First, _Rest...>>
{
    using type = typename _First::key_type;
};

template<typename _Set>
using cli_binding_set_key_type_t =
    typename cli_binding_set_key_type<_Set>::type;


// ===========================================================================
// III. per-binding name match
// ===========================================================================

NS_INTERNAL

    // binding_matches_name
    //   helper: true iff _Binding's primary name, or any of its
    // cli_alias<> args, or its cli_negate<> arg (if present),
    // equals _Query.
    //
    //   Done at compile time via constexpr string_view comparison;
    // works across heterogeneously-sized fixed_string carriers.
    template<typename _Binding,
             typename _Args>
    struct binding_matches_name_impl;

    template<typename _Binding>
    struct binding_matches_name_impl<_Binding, std::tuple<>>
    {
        static constexpr bool
        check(std::string_view) noexcept
        {
            return false;
        }
    };

    template<typename _Binding,
             typename... _Args>
    struct binding_matches_name_impl<_Binding, std::tuple<_Args...>>
    {
        // does this arg contribute a name (alias / negate)?  If so,
        // does it match the query?
        template<typename _Arg>
        static constexpr bool
        arg_matches(std::string_view _q) noexcept
        {
            if constexpr (is_cli_alias_v<_Arg>)
            {
                return (_Arg::view() == _q);
            }
            else if constexpr (is_cli_negate_v<_Arg>)
            {
                return (_Arg::view() == _q);
            }
            else
            {
                return false;
            }
        }

        static constexpr bool
        check(std::string_view _q) noexcept
        {
            return (arg_matches<_Args>(_q) || ... || false);
        }
    };

    template<typename _Binding>
    constexpr bool
    binding_matches_name(std::string_view _q) noexcept
    {
        if (_Binding::name_view() == _q)
        {
            return true;
        }

        return binding_matches_name_impl<
            _Binding,
            typename _Binding::args_type
        >::check(_q);
    }

NS_END  // internal


// ===========================================================================
// IV.  cli_binding_set_contains_key + find_by_key
// ===========================================================================

NS_INTERNAL

    // find_by_key_impl
    //   helper: scans a binding pack for one whose ::key == _Key.
    template<auto       _Key,
             typename...>
    struct find_by_key_impl
    {
        using type                          = cli_arg_not_found;
        static constexpr bool        found  = false;
        static constexpr std::size_t index  = cli_arg_npos;
    };

    template<auto       _Key,
             typename   _Head,
             typename...   _Tail>
    struct find_by_key_impl<_Key, _Head, _Tail...>
    {
    private:
        static constexpr bool head_matches = (_Head::key == _Key);

        using next_t = find_by_key_impl<_Key, _Tail...>;

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

NS_END  // internal


// cli_binding_set_find_by_key
//   trait: locate the binding whose key matches _Key.  Yields
// cli_arg_not_found / found=false / index=cli_arg_npos on miss.
template<typename _Set, auto _Key>
struct cli_binding_set_find_by_key;

template<typename... _Bindings, auto _Key>
struct cli_binding_set_find_by_key<cli_binding_set<_Bindings...>, _Key>
    : internal::find_by_key_impl<_Key, _Bindings...>
{};

template<typename _Set, auto _Key>
using cli_binding_set_find_by_key_t =
    typename cli_binding_set_find_by_key<_Set, _Key>::type;


// cli_binding_set_contains_key
//   trait: boolean form of cli_binding_set_find_by_key.
template<typename _Set, auto _Key>
struct cli_binding_set_contains_key
    : std::integral_constant<bool,
        cli_binding_set_find_by_key<_Set, _Key>::found>
{};

template<typename _Set, auto _Key>
inline constexpr bool cli_binding_set_contains_key_v =
    cli_binding_set_contains_key<_Set, _Key>::value;


// ===========================================================================
// V.   cli_binding_set_contains_name + find_by_name  (alias-aware)
// ===========================================================================

NS_INTERNAL

    // find_by_name_impl
    //   helper: scans a binding pack for one whose primary name,
    // alias, or negation matches _Name.  Compile-time string
    // equality via the binding_matches_name helper.
    template<fixed_string _Name,
             typename...>
    struct find_by_name_impl
    {
        using type                         = cli_arg_not_found;
        static constexpr bool        found = false;
        static constexpr std::size_t index = cli_arg_npos;
    };

    template<fixed_string _Name,
             typename     _Head,
             typename...  _Tail>
    struct find_by_name_impl<_Name, _Head, _Tail...>
    {
    private:
        static constexpr bool head_matches =
            binding_matches_name<_Head>(_Name.view());

        using next_t = find_by_name_impl<_Name, _Tail...>;

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

NS_END  // internal


// cli_binding_set_find_by_name
//   trait: locate the binding whose primary name, any cli_alias,
// or cli_negate matches _Name.  ALIAS / NEGATION AWARE - this is
// what lets parsing dispatch "--no-verbose" to the same binding
// that owns "--verbose".
template<typename _Set, fixed_string _Name>
struct cli_binding_set_find_by_name;

template<typename... _Bindings, fixed_string _Name>
struct cli_binding_set_find_by_name<cli_binding_set<_Bindings...>, _Name>
    : internal::find_by_name_impl<_Name, _Bindings...>
{};

template<typename _Set, fixed_string _Name>
using cli_binding_set_find_by_name_t =
    typename cli_binding_set_find_by_name<_Set, _Name>::type;


// cli_binding_set_contains_name
//   trait: boolean form of cli_binding_set_find_by_name.
template<typename _Set, fixed_string _Name>
struct cli_binding_set_contains_name
    : std::integral_constant<bool,
        cli_binding_set_find_by_name<_Set, _Name>::found>
{};

template<typename _Set, fixed_string _Name>
inline constexpr bool cli_binding_set_contains_name_v =
    cli_binding_set_contains_name<_Set, _Name>::value;


// ===========================================================================
// VI.  cli_binding_set_contains_short + find_by_short
// ===========================================================================

NS_INTERNAL

    // binding_short_matches
    //   helper: true iff _Binding has a cli_short<> arg whose
    // value equals _C.  False when the binding has no short form
    // at all (so cli_no_short queries never falsely match an
    // absent short).
    template<typename _Binding, char _C>
    constexpr bool
    binding_short_matches() noexcept
    {
        if constexpr (cli_binding_has_short_v<_Binding>)
        {
            return (cli_binding_short_tag_t<_Binding>::value == _C);
        }
        else
        {
            return false;
        }
    }

    // find_by_short_impl
    //   helper: scans a binding pack for one whose cli_short<>
    // value matches _C.
    template<char _C,
             typename...>
    struct find_by_short_impl
    {
        using type                         = cli_arg_not_found;
        static constexpr bool        found = false;
        static constexpr std::size_t index = cli_arg_npos;
    };

    template<char        _C,
             typename    _Head,
             typename... _Tail>
    struct find_by_short_impl<_C, _Head, _Tail...>
    {
    private:
        static constexpr bool head_matches =
            binding_short_matches<_Head, _C>();

        using next_t = find_by_short_impl<_C, _Tail...>;

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

NS_END  // internal


// cli_binding_set_find_by_short
//   trait: locate the binding whose cli_short<> matches _C.
template<typename _Set, char _C>
struct cli_binding_set_find_by_short;

template<typename... _Bindings, char _C>
struct cli_binding_set_find_by_short<cli_binding_set<_Bindings...>, _C>
    : internal::find_by_short_impl<_C, _Bindings...>
{};

template<typename _Set, char _C>
using cli_binding_set_find_by_short_t =
    typename cli_binding_set_find_by_short<_Set, _C>::type;


// cli_binding_set_contains_short
//   trait: boolean form of cli_binding_set_find_by_short.
template<typename _Set, char _C>
struct cli_binding_set_contains_short
    : std::integral_constant<bool,
        cli_binding_set_find_by_short<_Set, _C>::found>
{};

template<typename _Set, char _C>
inline constexpr bool cli_binding_set_contains_short_v =
    cli_binding_set_contains_short<_Set, _C>::value;


NS_END  // djinterp


#endif  // DJINTERP_CLI_BINDING_SET_TRAITS_
