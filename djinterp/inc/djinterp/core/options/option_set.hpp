/******************************************************************************
* djinterp [options]                                            option_set.hpp
*
*   The option_set<> core type plus the structural machinery needed to
* CONSTRUCT one safely:
*
*     - is_keyed         : the contract every option must satisfy.
*     - expand_option    : the per-option ::expanded_t customization point.
*     - flatten_tuples_t : type-level tuple_cat.
*     - run_set_checks   : the uniformity + uniqueness static_asserts.
*
*   Queries OVER an instantiated set (key_type, contains, find) live in
* option_set_traits.hpp.  Comparison / equality traits live in
* option_set_compare.hpp.  Concept analogs live in option_set_concepts.hpp.
*
*   This header has NO dependency on option.hpp or option_tags.hpp.  It
* works against the SHAPE (any keyed type, optionally with an ::expanded_t
* alias) - not against named types.  Adding a new option type, or a new
* multi-expander, requires NO edit here.
*
*
* path:      /inc/djinterp/core/options/option_set.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    is_keyed             (structural option contract)
II.   expand_option        (structural per-option expansion)
III.  flatten helpers      (tuple_cat at the type level)
IV.   set checks           (keyed + uniformity + uniqueness)
V.    option_set
*/

#ifndef DJINTERP_OPTION_SET_
#define DJINTERP_OPTION_SET_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "../util/lookup.hpp"   // value_pack_unique


NS_DJINTERP


// ===========================================================================
// I.   is_keyed
// ===========================================================================

// is_keyed
//   trait: structural detection of the option contract.  True iff
// _Type exposes both a static ::key member and a nested ::key_type
// alias.  This is the entire shape option_set requires of any type
// it accepts (after expansion).
template<typename _Type,
         typename = void>
struct is_keyed : std::false_type
{};

template<typename _Type>
struct is_keyed<_Type, std::void_t<
        decltype(_Type::key),
        typename _Type::key_type>>
    : std::true_type
{};

template<typename _Type>
inline constexpr bool is_keyed_v = is_keyed<_Type>::value;


// ===========================================================================
// II.  expand_option
// ===========================================================================

// expand_option
//   trait: yields std::tuple<...> for ONE user-supplied option.
//   Default specialization: an option is its own expansion (wrapped
// in a single-element tuple).  An option opts into multi-expansion
// by exposing a nested `::expanded_t = std::tuple<...>` alias.
// This is the ONLY customization point - detection is structural,
// so adding a new multi-expanding option type requires NO
// specialization of expand_option here.
//
// Example multi-expander (lives in option_tags.hpp):
//   template<auto _A, auto _B, typename... _Shared>
//   struct opposing_unary_pair
//   {
//       using expanded_t = std::tuple<
//           option<_A, _Shared..., opposes<_B>>,
//           option<_B, _Shared..., opposes<_A>>>;
//   };
template<typename _Option,
         typename = void>
struct expand_option
{
    using type = std::tuple<_Option>;
};

template<typename _Option>
struct expand_option<_Option, std::void_t<typename _Option::expanded_t>>
{
    using type = typename _Option::expanded_t;
};

template<typename _Option>
using expand_option_t = typename expand_option<_Option>::type;


// ===========================================================================
// III. flatten helpers
// ===========================================================================

// flatten_tuples_t
//   trait: tuple_cat-style flattening at the type level.
template<typename... _Tuples>
using flatten_tuples_t = decltype(std::tuple_cat(std::declval<_Tuples>()...));


// ===========================================================================
// IV.  set checks
// ===========================================================================

NS_INTERNAL

    // all_same_type
    //   helper: every type in the pack is identical.
    template<typename...>
    struct all_same_type : std::true_type
    {};

    template<typename _First>
    struct all_same_type<_First> : std::true_type
    {};

    template<typename    _First,
             typename    _Second,
             typename... _Rest>
    struct all_same_type<_First, _Second, _Rest...>
        : std::integral_constant<bool,
            ( std::is_same<_First, _Second>::value &&
              all_same_type<_Second, _Rest...>::value )>
    {};

    // all_keyed
    //   helper: every type in the pack satisfies the structural
    // is_keyed contract.
    template<typename...>
    struct all_keyed : std::true_type
    {};

    template<typename    _First,
             typename... _Rest>
    struct all_keyed<_First, _Rest...>
        : std::integral_constant<bool,
            ( is_keyed_v<_First> &&
              all_keyed<_Rest...>::value )>
    {};

    // run_set_checks
    //   helper: instantiated by option_set on its flat (post-
    // expansion) option tuple to fire the structural + uniformity +
    // uniqueness static_asserts.  Exposes ::value so callers can
    // depend on it and force instantiation.
    template<typename _Tuple>
    struct run_set_checks;

    template<>
    struct run_set_checks<std::tuple<>>
    {
        static constexpr bool value = true;
    };

    template<typename    _First,
             typename... _Rest>
    struct run_set_checks<std::tuple<_First, _Rest...>>
    {
        static_assert(
            all_keyed<_First, _Rest...>::value,
            "option_set: every option (after expansion) must be "
            "KEYED - i.e., expose a static ::key member and a "
            "nested ::key_type alias.  This is the open-world "
            "contract; option_set itself imposes nothing else.");

        static_assert(
            all_same_type<
                typename _First::key_type,
                typename _Rest::key_type...
            >::value,
            "option_set: all options (after expansion) must share "
            "the same key_type.  Use a single enum / class / scope "
            "for every key in the set.");

        static_assert(
            value_pack_unique<
                _First::key,
                _Rest::key...
            >::value,
            "option_set: all keys (after expansion) must be unique. "
            "Note that multi-expanding options (those exposing "
            "::expanded_t) emit ALL of their inner keys - if you "
            "also declare a colliding key directly, the duplicate "
            "is caught here.");

        static constexpr bool value = true;
    };

NS_END  // internal


// ===========================================================================
// V.   option_set
// ===========================================================================

template<typename... _Options>
struct option_set
{
private:
    // 1. expand each user-supplied option per the structural
    //    ::expanded_t convention (no specializations required).
    // 2. flatten the per-option tuples into one normalized tuple.
    using flat_tuple = flatten_tuples_t<expand_option_t<_Options>...>;

    // 3. force the checks to fire by depending on ::value.  Accessing
    //    the value member instantiates run_set_checks, which runs the
    //    nested static_asserts.
    static_assert(internal::run_set_checks<flat_tuple>::value,
                  "internal: run_set_checks did not return true "
                  "(see preceding assertion for the real diagnostic).");

public:
    // size
    //   value: number of options AFTER expansion.
    static constexpr std::size_t size = std::tuple_size_v<flat_tuple>;

    // empty
    static constexpr bool empty = (size == 0);

    // flat_options_t
    //   type: the normalized std::tuple<...> of expanded options.
    using flat_options_t = flat_tuple;

    // option_at
    //   type: positional access into the flat list.
    template<std::size_t _I>
    using option_at = std::tuple_element_t<_I, flat_tuple>;
};


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_
