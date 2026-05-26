/******************************************************************************
* djinterp [options]                                          option_traits.hpp
*
*   Trait machinery for option<>:
*
*     1. is_option<T>          - detect "T is an option<...> specialization"
*     2. find_arg              - generic predicate-based pack search
*     3. option_find_arg       - same, adapted to walk an option's args
*     4. option_has_arg        - boolean form of option_find_arg
*     5. value<>               - the built-in "this option carries a value"
*                                tag, kept alongside its detection traits
*                                so the canonical example is self-contained
*     6. option_from_tuple     - lift a tuple-shaped schema row into option<>
*
*   ADDING A NEW CONTEXT TAG (e.g. verifier, description, range, ...):
*
*     1. Define a tag struct:
*          template<auto _Fn>
*          struct verifier { static constexpr auto fn = _Fn; };
*
*     2. Define a predicate trait that detects it:
*          template<typename _T> struct is_verifier : std::false_type {};
*          template<auto _Fn>
*          struct is_verifier<verifier<_Fn>> : std::true_type {};
*
*     3. (Optional) Convenience extractor over an option:
*          template<typename _Option>
*          using option_verifier_tag =
*              option_find_arg_t<_Option, is_verifier>;
*
*   No central registration is required.  See option_tags.hpp for the
* full shipped tag library following this pattern.
*
*
* path:      /inc/djinterp/core/options/option_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_option                          (option<> specialization detection)
II.   find_arg                           (generic pack search by predicate)
III.  option_find_arg / option_has_arg   (option-adapted)
IV.   value<> tag + detection traits
V.    option_from_tuple
*/

#ifndef DJINTERP_OPTION_TRAITS_
#define DJINTERP_OPTION_TRAITS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./option.hpp"


NS_DJINTERP


// ===========================================================================
// I.   is_option
// ===========================================================================

// is_option
//   trait: true iff _T is some option<_Key, _Args...> specialization.
// Catches both the unary form (option<K>) and the args form
// (option<K, A, B, ...>) via a single _Args... pack that may be empty.
template<typename _T>
struct is_option : std::false_type
{};

template<auto        _Key,
         typename... _Args>
struct is_option<option<_Key, _Args...>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_option_v = is_option<_T>::value;


// ===========================================================================
// II.  find_arg
// ===========================================================================

// find_arg
//   trait: finds the first type in the pack for which
// _Predicate<T>::value is true.  Members mirror std::find:
//
//     ::type   - the matching type, or arg_not_found on miss.
//     ::found  - bool.
//     ::index  - position of the match, or arg_npos on miss.
//
//   _Predicate is a single-parameter trait template (the same shape
// as std::is_pointer, std::is_class, etc.).  This lets callers pass
// any predicate they want without coupling find_arg to a specific
// context.
template<template<typename> class _Predicate,
         typename...               _Args>
struct find_arg;

// base case: empty pack
template<template<typename> class _Predicate>
struct find_arg<_Predicate>
{
    using type = arg_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = arg_npos;
};

// recursive case
template<template<typename> class _Predicate,
         typename                  _Head,
         typename...               _Tail>
struct find_arg<_Predicate, _Head, _Tail...>
{
private:
    static constexpr bool head_matches = _Predicate<_Head>::value;

    using next_t = find_arg<_Predicate, _Tail...>;

public:
    using type =
        std::conditional_t<head_matches, _Head, typename next_t::type>;

    static constexpr bool found = (head_matches || next_t::found);

    static constexpr std::size_t index =
        head_matches
            ? 0
            : ( (next_t::index == arg_npos)
                  ? arg_npos
                  : (next_t::index + 1) );
};

template<template<typename> class _Predicate,
         typename...               _Args>
using find_arg_t = typename find_arg<_Predicate, _Args...>::type;


// ===========================================================================
// III. option_find_arg / option_has_arg
// ===========================================================================

// option_find_arg
//   adapter: applies find_arg to an option's args.  Yields
// arg_not_found / false / arg_npos when the option has no args at
// all (matching the empty-pack behavior of find_arg).
template<typename                 _Option,
         template<typename> class _Predicate>
struct option_find_arg;

// unary option: empty args -> miss
template<auto                     _Key,
         template<typename> class _Predicate>
struct option_find_arg<option<_Key>, _Predicate>
{
    using type = arg_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = arg_npos;
};

// option with args
template<auto                      _Key,
         typename                  _First,
         typename...               _Rest,
         template<typename> class  _Predicate>
struct option_find_arg<option<_Key, _First, _Rest...>, _Predicate>
{
private:
    using inner_t = find_arg<_Predicate, _First, _Rest...>;

public:
    using type = typename inner_t::type;

    static constexpr bool        found = inner_t::found;
    static constexpr std::size_t index = inner_t::index;
};

template<typename                 _Option,
         template<typename> class _Predicate>
using option_find_arg_t = typename option_find_arg<_Option, _Predicate>::type;


// option_has_arg
//   trait: true iff some arg in the option satisfies _Predicate.
template<typename                 _Option,
         template<typename> class _Predicate>
struct option_has_arg
    : std::integral_constant<bool,
        option_find_arg<_Option, _Predicate>::found>
{};

template<typename                 _Option,
         template<typename> class _Predicate>
inline constexpr bool option_has_arg_v =
    option_has_arg<_Option, _Predicate>::value;


// ===========================================================================
// IV.  value<> tag + detection traits
// ===========================================================================

// value
//   tag: "this option carries a value".  Mirrors the
// std::integral_constant interface (::value_type, ::value), so it
// composes with anything that already speaks that vocabulary.
//
//   The type is deduced from the NTTP via `auto`:
//
//     value<false>        ->  value_type = bool,    value = false
//     value<42>           ->  value_type = int,     value = 42
//     value<some_enum::x> ->  value_type = some_enum
//
//   The "user-explicit" form value<bool, false> is intentionally not
// supported: type deduction from the NTTP is sufficient.  If you
// genuinely need to override the deduced type, cast the NTTP at the
// call site (e.g. `value<static_cast<long>(0)>`).
template<auto _V>
struct value
{
    using value_type = decltype(_V);

    static constexpr value_type the_value = _V;
};

// is_value
//   trait: true iff _T is an instantiation of value<>.
//   Closed over value<_V> by design - users add their own contexts
// by defining their own predicates, not by extending is_value.
template<typename _T>
struct is_value : std::false_type
{};

template<auto _V>
struct is_value<value<_V>> : std::true_type
{};

template<typename _T>
inline constexpr bool is_value_v = is_value<_T>::value;


// option_value_tag
//   trait: yields the option's value<> tag if present, or
// arg_not_found otherwise.
template<typename _Option>
using option_value_tag = option_find_arg<_Option, is_value>;

template<typename _Option>
using option_value_tag_t = typename option_value_tag<_Option>::type;

// option_has_value
//   trait: true iff the option carries a value<> tag among its args.
template<typename _Option>
struct option_has_value
    : std::integral_constant<bool, option_value_tag<_Option>::found>
{};

template<typename _Option>
inline constexpr bool option_has_value_v = option_has_value<_Option>::value;


// ===========================================================================
// V.   option_from_tuple
// ===========================================================================

// option_from_tuple
//   trait: lifts a std::tuple-shaped schema into an option<>.  The
// element at _KeyIndex is treated as the key carrier - it must
// expose a static constexpr ::value member, which becomes the
// option's key NTTP.  All other elements (in their original
// left-to-right order) become the option's args.
//
// Example:
//   template<auto _V>
//   struct key_v { static constexpr auto value = _V; };
//
//   using row = std::tuple<key_v<window_opt::title>,
//                          value<"Untitled">,
//                          verifier<&fn>>;
//   using opt = option_from_tuple_t<row, 0>;
//   // opt == option<window_opt::title,
//   //               value<"Untitled">,
//   //               verifier<&fn>>
template<typename    _Tuple,
         std::size_t _KeyIndex>
struct option_from_tuple;

NS_INTERNAL

    // option_from_tuple_helper
    //   helper: given an index_sequence covering the args (length
    // tuple_size - 1, since the key element is skipped), maps each
    // sequence index i to either tuple_element_t<i, _Tuple> (when
    // i < _KeyIndex) or tuple_element_t<i + 1, _Tuple> (when
    // i >= _KeyIndex).  Classic skip-the-i-th trick.
    template<typename    _Tuple,
             std::size_t _KeyIndex,
             typename    _IndexSequence>
    struct option_from_tuple_helper;

    template<typename       _Tuple,
             std::size_t    _KeyIndex,
             std::size_t... _Indexes>
    struct option_from_tuple_helper<_Tuple,
                                    _KeyIndex,
                                    std::index_sequence<_Indexes...>>
    {
        using key_carrier_type =
            std::tuple_element_t<_KeyIndex, _Tuple>;

        static_assert(
            requires { key_carrier_type::value; },
            "option_from_tuple: the element at _KeyIndex must "
            "expose a static constexpr ::value member.");

        template<std::size_t _Index>
        using arg_at =
            std::conditional_t<
                (_Index < _KeyIndex),
                std::tuple_element_t<_Index,     _Tuple>,
                std::tuple_element_t<_Index + 1, _Tuple>>;

        using type = option<key_carrier_type::value, arg_at<_Indexes>...>;
    };

NS_END  // internal

template<typename    _Tuple,
         std::size_t _KeyIndex>
struct option_from_tuple
{
private:
    static constexpr std::size_t tuple_size = std::tuple_size_v<_Tuple>;

    static_assert(tuple_size >= 1,
                  "option_from_tuple: tuple must contain at least the key.");
    static_assert(_KeyIndex < tuple_size,
                  "option_from_tuple: key index is out of range.");

    // Compute args sequence length safely: 0 for key-only tuples.
    // Prevents ever forming make_index_sequence<-1>.
    static constexpr std::size_t args_count =
        (tuple_size > 0 ? tuple_size - 1 : 0);

public:
    using type =
        typename internal::option_from_tuple_helper<
            _Tuple,
            _KeyIndex,
            std::make_index_sequence<args_count>>::type;
};

template<typename    _Tuple,
         std::size_t _KeyIndex>
using option_from_tuple_t = typename option_from_tuple<_Tuple, _KeyIndex>::type;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_TRAITS_
