/******************************************************************************
* djinterp [option]                                     option_set_compare.hpp
*
*   Compile-time comparison/evaluation traits for option_set<>.  Three
* layers of congruity, from weakest to strongest, plus a parameterized
* value-equality trait:
*
*     1. KEY congruity      - same set of keys (order-insensitive).
*     2. TYPE congruity     - same OPTION TYPE at every key (full type
*                             equality on each, including args pack).
*     3. VALUE equality     - parameterized over a value extractor; two
*                             sets are value-equal under _Extract iff
*                             they are key-congruent AND the extracted
*                             carriers compare equal at every key.
*
*   The value-equality trait is intentionally extractor-parameterized -
* the caller supplies a unary extractor that maps an option to a value
* carrier in the {value_absent | value_present<V>} interface (Section IV).
*   No extractor ships here.  The earlier actual<> / default_ / effective
* extractors were retired along with the actual<> option carrier, so
* _Extract is a REQUIRED template parameter (there is no default).
*
*   All traits are flat-view aware (they go through option_set's normalized
* tuple) so opposing_unary_pair and any other multi-expander participate
* correctly.
*
*
* path:      /inc/djinterp/core/option/option_set_compare.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    key_list + option_set_keys     (key extraction)
II.   key-list operations            (subset, equal)
III.  congruity traits               (key, type)
IV.   value-carrier sentinels        (value_absent, value_present)
V.    option_set_value_eq            (parameterized value equality)
*/

#ifndef DJINTERP_OPTION_SET_COMPARE_
#define DJINTERP_OPTION_SET_COMPARE_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
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
template<auto... _Keys>
struct key_list
{
    static constexpr std::size_t size = sizeof...(_Keys);
};

// option_set_keys
//   trait: yields key_list<...> of all keys in the FLAT view of _Set.
template<typename _Set>
struct option_set_keys;

template<typename... _Options>
struct option_set_keys<option_set<_Options...>>
{
private:
    using flat = typename option_set<_Options...>::flat_options_t;

    template<typename _Type>
    struct apply;

    template<typename... _Opts>
    struct apply<std::tuple<_Opts...>>
    {
        using type = key_list<_Opts::key...>;
    };

public:
    using type = typename apply<flat>::type;
};

template<typename _Set>
using option_set_keys_t = typename option_set_keys<_Set>::type;


// ===========================================================================
// II.  key-list operations
// ===========================================================================

NS_INTERNAL

    // value_in_pack
    //   helper: true iff _K equals any of _Ks (NTTP-level OR).
    template<auto _K, auto... _Ks>
    struct value_in_pack
        : std::integral_constant<bool, ((_K == _Ks) || ...)>
    {};

NS_END  // internal

// key_list_subset
//   trait: true iff every key in _Lhs appears in _Rhs.
template<typename _Lhs,
         typename _Rhs>
struct key_list_subset;

template<auto... _Ls, auto... _Rs>
struct key_list_subset<key_list<_Ls...>, key_list<_Rs...>>
    : std::integral_constant<bool,
        ( internal::value_in_pack<_Ls, _Rs...>::value && ... )>
{};

template<typename _Lhs,
         typename _Rhs>
inline constexpr bool key_list_subset_v =
    key_list_subset<_Lhs, _Rhs>::value;


// key_list_equal
//   trait: true iff _Lhs and _Rhs contain the same set of keys
// (order-insensitive).
template<typename _Lhs,
         typename _Rhs>
struct key_list_equal
    : std::integral_constant<bool,
        ( key_list_subset<_Lhs, _Rhs>::value &&
          key_list_subset<_Rhs, _Lhs>::value )>
{};

template<typename _Lhs,
         typename _Rhs>
inline constexpr bool key_list_equal_v =
    key_list_equal<_Lhs, _Rhs>::value;


// ===========================================================================
// III. congruity traits
// ===========================================================================

// option_set_key_congruent
//   trait: true iff _A and _B have the same key set (order-insensitive).
template<typename _A,
         typename _B>
struct option_set_key_congruent
    : std::integral_constant<bool,
        key_list_equal<
            option_set_keys_t<_A>,
            option_set_keys_t<_B>
        >::value>
{};

template<typename _A,
         typename _B>
inline constexpr bool option_set_key_congruent_v =
    option_set_key_congruent<_A, _B>::value;


// option_set_type_congruent
//   trait: stronger than key-congruent.  True iff the two sets are
// key-congruent AND each option in _A is exactly the same type as
// the option at the same key in _B (full type equality, including
// args pack).
template<typename _A,
         typename _B>
struct option_set_type_congruent;

template<typename... _AOpts,
         typename     _B>
struct option_set_type_congruent<option_set<_AOpts...>, _B>
{
private:
    template<typename _Opt>
    static constexpr bool matches_in_b =
        ( option_set_contains_v<_B, _Opt::key> &&
          std::is_same_v<_Opt, option_set_find_t<_B, _Opt::key>> );

public:
    static constexpr bool value =
        ( option_set_key_congruent_v<option_set<_AOpts...>, _B> &&
          (matches_in_b<_AOpts> && ...) );
};

template<typename _A,
         typename _B>
inline constexpr bool option_set_type_congruent_v =
    option_set_type_congruent<_A, _B>::value;


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
template<auto _V>
struct value_present
{
    using value_type = decltype(_V);

    static constexpr bool       has_value = true;
    static constexpr value_type value     = _V;
};

NS_INTERNAL

    // carrier_eq
    //   helper: structural equality for value_absent / value_present<V>.
    template<typename _L,
             typename _R>
    struct carrier_eq : std::false_type
    {};

    template<>
    struct carrier_eq<value_absent, value_absent> : std::true_type
    {};

    template<auto _LV,
             auto _RV>
    struct carrier_eq<value_present<_LV>, value_present<_RV>>
        : std::integral_constant<bool, (_LV == _RV)>
    {};

NS_END  // internal


// ===========================================================================
// V.   option_set_value_eq
// ===========================================================================

// option_set_value_eq
//   trait: parameterized over a single-arg extractor.  Two sets are
// value-equal under _Extract iff they are key-congruent and the
// extracted carriers compare equal at every key.
//
//   _Extract is any unary trait-style template that yields a type
// satisfying the {value_absent | value_present<V>} interface.
template<typename                 _A,
         typename                 _B,
         template<typename> typename _Extract>
struct option_set_value_eq;

template<typename... _AOpts,
         typename     _B,
         template<typename> typename _Extract>
struct option_set_value_eq<option_set<_AOpts...>, _B, _Extract>
{
private:
    template<typename _AOpt>
    static constexpr bool at_key =
        ( option_set_contains_v<_B, _AOpt::key> &&
          internal::carrier_eq<
              typename _Extract<_AOpt>::type,
              typename _Extract<
                  option_set_find_t<_B, _AOpt::key>>::type
          >::value );

public:
    static constexpr bool value =
        ( option_set_key_congruent_v<option_set<_AOpts...>, _B> &&
          (at_key<_AOpts> && ...) );
};

template<typename                 _A,
         typename                 _B,
         template<typename> typename _Extract>
inline constexpr bool option_set_value_eq_v =
    option_set_value_eq<_A, _B, _Extract>::value;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_SET_COMPARE_