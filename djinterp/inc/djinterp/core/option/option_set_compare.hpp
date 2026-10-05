/*******************************************************************************
* djinterp [core]                                         option_set_compare.hpp
*
*   Compile-time comparison/evaluation traits for option_set<>.  Three
* layers of congruity, from weakest to strongest, plus a parameterized
* value-equality trait:
*
*     1. KEY congruity      - same set of keys (order-insensitive).
*     2. TYPE congruity     - same OPTION TYPE at every key (full type
*                             equality on each, including args pack).
*     3. VALUE equality     - parameterized over a value extractor; two
*                             sets are value-equal under Extract iff
*                             they are key-congruent AND the extracted
*                             carriers compare equal at every key.
*
*   The value-equality trait is intentionally extractor-parameterized -
* the caller supplies a unary extractor that maps an option to a value
* carrier in the {value_absent | value_present<V>} interface (Section IV).
*   No extractor ships here.  The earlier actual<> / default_ / effective
* extractors were retired along with the actual<> option carrier, so
* Extract is a REQUIRED template parameter (there is no default).
*
*   All traits are flat-view aware (they go through option_set's normalized
* tuple) so opposing_unary_pair and any other multi-expander participate
* correctly.
*
*
* path:      /inc/djinterp/core/option/option_set_compare.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.25
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    key_list + option_set_keys     (key extraction)
      -----------------------------------------------

II.   key-list operations            (subset, equal)
      ----------------------------------------------

III.  congruity traits               (key, type)
      ------------------------------------------

IV.   value-carrier sentinels        (value_absent, value_present)
      ------------------------------------------------------------

V.    option_set_value_eq            (parameterized value equality)
      -------------------------------------------------------------
*/

#ifndef DJINTERP_OPTION_OPTION_SET_COMPARE_HPP
#define DJINTERP_OPTION_OPTION_SET_COMPARE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./option.hpp"             // option<>, is_option_v
#include "./option_set.hpp"         // option_set<> + queries (contains, find, key_type)


NS_DJINTERP


// ===========================================================================
// I.   key_list + option_set_keys
// ===========================================================================

// key_list
//   type: heterogeneous-ready compile-time pack of NTTP keys.
// Lives at the type level so it can be passed around like any other
// type and compared by partial-specialization machinery.
template<auto... Keys>
struct key_list
{
    static constexpr std::size_t size = sizeof...(Keys);
};

// option_set_keys
//   trait: yields key_list<...> of all keys in the FLAT view of Set.
template<typename Set>
struct option_set_keys;

template<typename... Options>
struct option_set_keys<option_set<Options...>>
{
private:
    using flat = typename option_set<Options...>::flat_options_t;

    template<typename Type>
    struct apply;

    template<typename... Opts>
    struct apply<std::tuple<Opts...>>
    {
        using type = key_list<Opts::key...>;
    };

public:
    using type = typename apply<flat>::type;
};

template<typename Set>
using option_set_keys_t = typename option_set_keys<Set>::type;


// ===========================================================================
// II.  key-list operations
// ===========================================================================

NS_INTERNAL

    // value_in_pack
    //   helper: true iff K equals any of Ks (NTTP-level OR).
    template<auto K, auto... Ks>
    struct value_in_pack
        : std::integral_constant<bool, ((K == Ks) || ...)>
    {};

NS_END  // internal

// key_list_subset
//   trait: true iff every key in Lhs appears in Rhs.
template<typename Lhs,
         typename Rhs>
struct key_list_subset;

template<auto... Ls, auto... Rs>
struct key_list_subset<key_list<Ls...>, key_list<Rs...>>
    : std::integral_constant<bool,
        ( internal::value_in_pack<Ls, Rs...>::value && ... )>
{};

template<typename Lhs,
         typename Rhs>
D_CONSTEXPR bool key_list_subset_v =
    key_list_subset<Lhs, Rhs>::value;


// key_list_equal
//   trait: true iff Lhs and Rhs contain the same set of keys
// (order-insensitive).
template<typename Lhs,
         typename Rhs>
struct key_list_equal
    : std::integral_constant<bool,
        ( key_list_subset<Lhs, Rhs>::value &&
          key_list_subset<Rhs, Lhs>::value )>
{};

template<typename Lhs,
         typename Rhs>
D_CONSTEXPR bool key_list_equal_v =
    key_list_equal<Lhs, Rhs>::value;


// ===========================================================================
// III. congruity traits
// ===========================================================================

// option_set_key_congruent
//   trait: true iff A and B have the same key set (order-insensitive).
template<typename A,
         typename B>
struct option_set_key_congruent
    : std::integral_constant<bool,
        key_list_equal<
            option_set_keys_t<A>,
            option_set_keys_t<B>
        >::value>
{};

template<typename A,
         typename B>
D_CONSTEXPR bool option_set_key_congruent_v =
    option_set_key_congruent<A, B>::value;


// option_set_type_congruent
//   trait: stronger than key-congruent.  True iff the two sets are
// key-congruent AND each option in A is exactly the same type as
// the option at the same key in B (full type equality, including
// args pack).
template<typename A,
         typename B>
struct option_set_type_congruent;

template<typename... AOpts,
         typename     B>
struct option_set_type_congruent<option_set<AOpts...>, B>
{
private:
    template<typename Opt>
    static constexpr bool matches_in_b =
        ( option_set_contains_v<B, Opt::key> &&
          std::is_same_v<Opt, option_set_find_t<B, Opt::key>> );

public:
    static constexpr bool value =
        ( option_set_key_congruent_v<option_set<AOpts...>, B> &&
          (matches_in_b<AOpts> && ...) );
};

template<typename A,
         typename B>
D_CONSTEXPR bool option_set_type_congruent_v =
    option_set_type_congruent<A, B>::value;


// ===========================================================================
// IV.  value-carrier sentinels
// ===========================================================================

// value_absent
//   type: extractor result for "this option carries no value of the
// requested kind".
struct value_absent
{
    static constexpr bool has_value = false;
};

// value_present
//   type: extractor result carrying the extracted NTTP.
template<auto V>
struct value_present
{
    using value_type = decltype(V);

    static constexpr bool       has_value = true;
    static constexpr value_type value     = V;
};

NS_INTERNAL

    // carrier_eq
    //   helper: structural equality for value_absent / value_present<V>.
    template<typename L,
             typename R>
    struct carrier_eq : std::false_type
    {};

    template<>
    struct carrier_eq<value_absent, value_absent> : std::true_type
    {};

    template<auto LV,
             auto RV>
    struct carrier_eq<value_present<LV>, value_present<RV>>
        : std::integral_constant<bool, (LV == RV)>
    {};

NS_END  // internal


// ===========================================================================
// V.   option_set_value_eq
// ===========================================================================

// option_set_value_eq
//   trait: parameterized over a single-arg extractor.  Two sets are
// value-equal under Extract iff they are key-congruent and the
// extracted carriers compare equal at every key.
//
//   Extract is any unary trait-style template that yields a type
// satisfying the {value_absent | value_present<V>} interface.
template<typename                 A,
         typename                 B,
         template<typename> typename Extract>
struct option_set_value_eq;

template<typename... AOpts,
         typename     B,
         template<typename> typename Extract>
struct option_set_value_eq<option_set<AOpts...>, B, Extract>
{
private:
    template<typename AOpt>
    static constexpr bool at_key =
        ( option_set_contains_v<B, AOpt::key> &&
          internal::carrier_eq<
              typename Extract<AOpt>::type,
              typename Extract<
                  option_set_find_t<B, AOpt::key>>::type
          >::value );

public:
    static constexpr bool value =
        ( option_set_key_congruent_v<option_set<AOpts...>, B> &&
          (at_key<AOpts> && ...) );
};

template<typename                 A,
         typename                 B,
         template<typename> typename Extract>
D_CONSTEXPR bool option_set_value_eq_v =
    option_set_value_eq<A, B, Extract>::value;


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_OPTION_OPTION_SET_COMPARE_HPP
