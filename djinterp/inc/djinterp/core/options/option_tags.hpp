/******************************************************************************
* djinterp [options]                                          option_tags.hpp
*
*   The starter library of built-in context tags for option<>.  Each tag
* is a type wrapper that gives one of an option's args a named role:
*
*     actual<_Value>          - the option's current/initial value
*     default_<_Value>        - the option's default value
*     val_type<_Type>         - explicit value-type marker
*     verifier<_Fn>           - verification function pointer
*     description<_Str>       - human-readable description string literal
*     opposes<_OtherKey>      - this option is the conceptual opposite of _OtherKey
*
*   Each tag follows the four-piece pattern:
*     1. the tag struct
*     2. is_<tag><T>  : std::false_type | std::true_type predicate
*     3. option_<tag>_tag<Opt>     - the find adapter alias
*     4. option_has_<tag>_v<Opt>   - the bool convenience
*
*   Adding a new tag follows the same pattern.  Nothing here is registered
* centrally - the option core in option.hpp has no idea any of these
* exist.
*
*   Also: fixed_string<N> for string-literal NTTPs (C++20 class-type NTTPs),
* unary_option<> as a stylistic alias of option<>, and
* opposing_unary_pair<> for declaring two opposing unary options in one
* statement.  opposing_unary_pair carries its own ::expanded_t alias, which
* option_set picks up STRUCTURALLY - no central expand_option specialization
* is required.
*
*
* path:      /inc/djinterp/core/options/option_tags.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    fixed_string
II.   nttp wrapper + trait detection
III.  actual / default_ / val_type / verifier / opposes
IV.   description (uses fixed_string)
V.    unary_option alias
VI.   opposing_unary_pair (carries ::expanded_t)
*/

#ifndef DJINTERP_OPTION_TAGS_
#define DJINTERP_OPTION_TAGS_ 1

// std
#include <cstddef>
#include <tuple>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./option.hpp"


NS_DJINTERP


// ===========================================================================
// I.   fixed_string
// ===========================================================================

// fixed_string
//   helper: structural string-literal carrier suitable for use
// as a C++20 class-type NTTP.  Required because raw const char[N]
// doesn't satisfy the structural-type constraints for NTTPs.
//
//   N includes the terminating null, matching the underlying
// array length of a string literal.
template<std::size_t _N>
struct fixed_string
{
    char data[_N];

    constexpr fixed_string(
        const char (&_s)[_N]
    )
    {
        for (std::size_t i = 0; i < _N; ++i)
        {
            data[i] = _s[i];
        }
    }

    constexpr const char*
    c_str() const noexcept
    {
        return data;
    }

    constexpr std::size_t
    size() const noexcept
    {
        return _N - 1;  // exclude null terminator
    }
};

// deduction guide
template<std::size_t _N>
fixed_string(const char (&)[_N]) -> fixed_string<_N>;


// ===========================================================================
// II.  nttp wrapper + trait detection
// ===========================================================================

// remove_cvref_t
//   alias: convenience for stripping const / volatile / reference.
template<typename _Type>
using remove_cvref_t =
    std::remove_cv_t<std::remove_reference_t<_Type>>;


// nttp
//   trait: explicit NTTP carrier.  Useful when you want to forward
// an NTTP-like value through a typename slot.
template<typename _Type,
         _Type    _Value>
struct nttp
{
    using self_type = nttp<_Type, _Value>;
    using type      = _Type;

    static constexpr type value = _Value;

    constexpr operator type() const noexcept
    {
        return value;
    }
};

// nttp partial specialization for "deduce the type from the NTTP"
template<auto _Value>
struct nttp<decltype(_Value), _Value>
{
    using self_type = nttp<decltype(_Value), _Value>;
    using type      = decltype(_Value);

    static constexpr type value = _Value;

    constexpr operator type() const noexcept
    {
        return value;
    }
};

// is_nttp
//   trait: detects an nttp<> instantiation.
template<typename _Type,
         typename = void>
struct is_nttp : std::false_type
{};

template<typename _Type,
         _Type    _Value>
struct is_nttp<nttp<_Type, _Value>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_nttp_v = is_nttp<remove_cvref_t<_Type>>::value;


// nttp_traits
//   trait: value / type extractor for nttp<>.
template<typename _Type,
         typename = void>
struct nttp_traits
{};

template<typename _Type,
         _Type    _Value>
struct nttp_traits<nttp<_Type, _Value>, void>
{
    using type = _Type;

    static constexpr type value = _Value;
};

template<typename _Nttp>
using nttp_t = typename nttp_traits<remove_cvref_t<_Nttp>>::type;

template<typename _Nttp>
inline constexpr nttp_t<_Nttp> nttp_v =
    nttp_traits<remove_cvref_t<_Nttp>>::value;


#if defined(__cpp_concepts)
    // nttp_instance
    //   concept: satisfied iff the (decayed) type is an nttp<>.
    template<typename _Type>
    concept nttp_instance = is_nttp_v<_Type>;
#endif


// ===========================================================================
// III. Simple NTTP / typename context tags
// ===========================================================================

// ----- actual<> --------------------------------------------------------------

// actual
//   tag: the option's actual/current value.  value_type is
// deduced from the NTTP.
template<auto _Value>
struct actual
{
    using value_type = decltype(_Value);

    static constexpr value_type the_value = _Value;
};

template<typename _Type>
struct is_actual : std::false_type
{};

template<auto _Value>
struct is_actual<actual<_Value>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_actual_v = is_actual<_Type>::value;

template<typename _Opt>
using option_actual_tag   = option_find_arg<_Opt, is_actual>;

template<typename _Opt>
using option_actual_tag_t = typename option_actual_tag<_Opt>::type;

template<typename _Opt>
inline constexpr bool option_has_actual_v = option_actual_tag<_Opt>::found;


// ----- default_<> ------------------------------------------------------------
// (trailing underscore avoids the `default` keyword)

template<auto _Value>
struct default_
{
    using value_type = decltype(_Value);

    static constexpr value_type the_value = _Value;
};

template<typename _Type>
struct is_default : std::false_type
{};

template<auto _Value>
struct is_default<default_<_Value>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_default_v = is_default<_Type>::value;

template<typename _Opt>
using option_default_tag   = option_find_arg<_Opt, is_default>;

template<typename _Opt>
using option_default_tag_t = typename option_default_tag<_Opt>::type;

template<typename _Opt>
inline constexpr bool option_has_default_v =
    option_default_tag<_Opt>::found;


// ----- val_type<> ------------------------------------------------------------

// val_type
//   tag: explicit value-type marker.  Useful when the type
// should be stated in the schema (documentation, disambiguation)
// even though `actual<>` or `default_<>` would imply it.
template<typename _Type>
struct val_type
{
    using type = _Type;
};

template<typename _Type>
struct is_val_type : std::false_type
{};

template<typename _Type>
struct is_val_type<val_type<_Type>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_val_type_v = is_val_type<_Type>::value;

template<typename _Opt>
using option_val_type_tag   = option_find_arg<_Opt, is_val_type>;

template<typename _Opt>
using option_val_type_tag_t = typename option_val_type_tag<_Opt>::type;

template<typename _Opt>
inline constexpr bool option_has_val_type_v =
    option_val_type_tag<_Opt>::found;


// ----- verifier<> ------------------------------------------------------------

// verifier
//   tag: verification function pointer.  The function's
// signature is up to the consumer; verifier<> just carries it.
template<auto _Fn>
struct verifier
{
    static constexpr auto fn = _Fn;
};

template<typename _Type>
struct is_verifier : std::false_type
{};

template<auto _Fn>
struct is_verifier<verifier<_Fn>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_verifier_v = is_verifier<_Type>::value;

template<typename _Opt>
using option_verifier_tag   = option_find_arg<_Opt, is_verifier>;

template<typename _Opt>
using option_verifier_tag_t = typename option_verifier_tag<_Opt>::type;

template<typename _Opt>
inline constexpr bool option_has_verifier_v =
    option_verifier_tag<_Opt>::found;


// ----- opposes<> -------------------------------------------------------------

// opposes
//   tag: marks this option as the conceptual opposite of
// another option in the same set.  Used by opposing_unary_pair
// (and, optionally, by hand-written entries that want to record
// the relationship without going through the pair helper).
template<auto _OtherKey>
struct opposes
{
    using key_type = decltype(_OtherKey);

    static constexpr key_type opposed_key = _OtherKey;
};

template<typename _Type>
struct is_opposes : std::false_type
{};

template<auto _K>
struct is_opposes<opposes<_K>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_opposes_v = is_opposes<_Type>::value;

template<typename _Opt>
using option_opposes_tag   = option_find_arg<_Opt, is_opposes>;

template<typename _Opt>
using option_opposes_tag_t = typename option_opposes_tag<_Opt>::type;

template<typename _Opt>
inline constexpr bool option_has_opposes_v =
    option_opposes_tag<_Opt>::found;


// ===========================================================================
// IV.  description (uses fixed_string)
// ===========================================================================

// description
//   tag: a human-readable description string literal.  Stored
// as a fixed_string<N> NTTP so the string content lives at the
// type level (no runtime allocation, no global pointer chase).
//
// Usage:
//   description<"does foo stuff">
template<fixed_string _S>
struct description
{
    static constexpr auto text = _S;
};

template<typename _Type>
struct is_description : std::false_type
{};

template<fixed_string _S>
struct is_description<description<_S>> : std::true_type
{};

template<typename _Type>
inline constexpr bool is_description_v = is_description<_Type>::value;

template<typename _Opt>
using option_description_tag   = option_find_arg<_Opt, is_description>;

template<typename _Opt>
using option_description_tag_t = typename option_description_tag<_Opt>::type;

template<typename _Opt>
inline constexpr bool option_has_description_v =
    option_description_tag<_Opt>::found;


// ===========================================================================
// V.   unary_option alias
// ===========================================================================

// unary_option
//   alias: stylistic shorthand for option<>, communicating
// at the call site that the option carries no value (just a
// key, optionally plus description / opposes / verifier).
//   No semantic constraint - the user is trusted to keep
// value-carrying tags off unary options.  If you want
// enforcement, add a static_assert layer on top.
template<auto        _Key,
         typename... _Args>
using unary_option = option<_Key, _Args...>;


// ===========================================================================
// VI.  opposing_unary_pair
// ===========================================================================

// opposing_unary_pair
//   marker: declares TWO unary options in a single statement,
// each conceptually opposing the other.  When this appears as
// an entry in an option_set, the set picks up the nested
// ::expanded_t alias structurally and fans it out into two
// distinct option<> entries, each carrying an opposes<>
// reference to the other plus any shared args.
//
//   The expansion logic lives HERE, with the type that owns the
// semantic - option_set.hpp imposes only "do you have an
// ::expanded_t?" and never names this type.
//
// Example:
//   opposing_unary_pair<cli::verbose, cli::quiet,
//                       description<"verbosity toggle">>
//
//   expands inside an option_set to:
//
//     option<cli::verbose, description<"verbosity toggle">, opposes<cli::quiet>>,
//     option<cli::quiet,   description<"verbosity toggle">, opposes<cli::verbose>>
template<auto        _KeyA,
         auto        _KeyB,
         typename... _SharedArgs>
struct opposing_unary_pair
{
    using key_type = decltype(_KeyA);

    static_assert(
        std::is_same<decltype(_KeyA), decltype(_KeyB)>::value,
        "opposing_unary_pair: both keys must share the same key type.");

    static constexpr key_type key_a = _KeyA;
    static constexpr key_type key_b = _KeyB;

    // expanded_t
    //   alias: the per-pair expansion.  option_set detects this
    // structurally (no specialization needed there) and uses it
    // to flatten the pair into the normalized option list.
    using expanded_t = std::tuple<
        option<_KeyA, _SharedArgs..., opposes<_KeyB>>,
        option<_KeyB, _SharedArgs..., opposes<_KeyA>>>;
};

// is_opposing_unary_pair
//   predicate: true iff _Type is an opposing_unary_pair<>
// instantiation.  Kept for callers that want to dispatch on the
// pair shape; the expansion machinery itself no longer needs it.
template<typename _Type>
struct is_opposing_unary_pair : std::false_type
{};

template<auto _A, auto _B, typename... _Args>
struct is_opposing_unary_pair<opposing_unary_pair<_A, _B, _Args...>>
    : std::true_type
{};

template<typename _Type>
inline constexpr bool is_opposing_unary_pair_v =
    is_opposing_unary_pair<_Type>::value;


NS_END  // djinterp


#endif  // DJINTERP_OPTION_TAGS_
