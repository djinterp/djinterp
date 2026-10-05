/*******************************************************************************
* djinterp [core]                                                     lookup.hpp
*
*   Compile-time lookup-by-key idioms over type packs.  Domain-agnostic:
* knows nothing about options, settings, commands, configs, or any other
* downstream concept.  Whatever your domain is, if its entries expose
* either a value-key (a static `::key` member) or a type-key (a `::key_type`
* alias), the traits here can search, count, locate, and project them.
*   Four families:
*     Value-keyed entries (entry::key is an NTTP value):
*       find_by_key<Key, Entries...>          - first match or sentinel
*       contains_key<Key, Entries...>         - bool trait
*       key_index_of<Key, Entries...>         - size_t (or lookup_npos)
*       keys_of<Entries...>                    - value_pack of every key
*     Type-keyed entries (entry::key_type is a type):
*       find_by_type_key<Type, Entries...>    - first match or sentinel
*       contains_type_key<Type, Entries...>   - bool trait
*       type_key_index_of<Type, Entries...>   - size_t (or lookup_npos)
*       type_keys_of<Entries...>               - type_pack of every key
*     Predicate-driven (no key convention; caller supplies the match):
*       find_by_pred<Predicate, Entries...>   - first match or sentinel
*       contains_pred<Predicate, Entries...>  - bool trait
*       pred_index_of<Predicate, Entries...>  - size_t (or lookup_npos)
*     Sorted-key binary search (value-/type-keyed; opt-in fast path):
*       find_by_key_bsearch<Key, Entries...>          - O(log N) depth
*       find_by_type_key_bsearch<Cmp, Type, E...>    - comparator-ordered
*       find_by_key_auto<Key, Entries...>             - dispatch on sorted
*   Plus two carrier shapes:
*
*     value_pack<auto...>     - results carrier for NTTP packs
*     type_pack<typename...>  - results carrier for type packs
*
*   And two NTTP-pack predicates (also useful in their own right):
*
*     value_pack_contains<Needle, Haystack...>    - bool trait
*     value_pack_unique<Haystack...>               - bool trait
*
*   Search policy is first-match-wins on a left-to-right walk, matching
* the intuition from std::find / std::ranges::find.  Misses yield the
* `lookup_not_found` sentinel and the `lookup_npos` index, both
* well-typed and inspectable.  Linear recursion is O(N) per lookup; the
* boolean predicates short-circuit via the recursive ||.  The optional
* sorted family (VIII) trades the O(N) walk for O(log N) recursion
* DEPTH via the binary-search engine in bsearch.hpp - see that header
* and section VIII for the depth-vs-instantiation-count caveat.
*
*
* path:      /inc/djinterp/core/util/lookup/lookup.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.24
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Sentinels and constants
      -----------------------
      1.    lookup_not_found
      2.    lookup_npos

II.   Result carriers
      ---------------
      1.    value_pack
      2.    type_pack

III.  NTTP-pack predicates
      --------------------
      1.    value_pack_contains
      2.    value_pack_unique

IV.   Type-pack predicates
      --------------------
      1.    type_pack_contains
      2.    type_pack_unique

V.    Value-keyed lookup (convention: entry::key is an NTTP)
      ------------------------------------------------------
      1.    find_by_key
      2.    contains_key
      3.    key_index_of
      4.    keys_of

VI.   Type-keyed lookup (convention: entry::key_type is a type)
      ---------------------------------------------------------
      1.    find_by_type_key
      2.    contains_type_key
      3.    type_key_index_of
      4.    type_keys_of

VII.  Predicate-driven lookup (key-convention-agnostic)
      -------------------------------------------------
      1.    find_by_pred
      2.    contains_pred
      3.    pred_index_of

VIII. Sorted value-/type-key lookup (binary search; opt-in)
      -----------------------------------------------------
      1.    is_key_sorted
      2.    find_by_key_bsearch
      3.    find_by_type_key_bsearch
      4.    find_by_key_auto
*/

#ifndef DJINTERP_UTIL_LOOKUP_LOOKUP_HPP
#define DJINTERP_UTIL_LOOKUP_LOOKUP_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/bsearch.hpp"  // bsearch_by (engine for section VIII)
#include "./lookup_sentinels.hpp"  // lookup_not_found, lookup_npos (shared)


NS_DJINTERP


// ===========================================================================
// I.   Sentinels and constants
// ===========================================================================
//   lookup_not_found and lookup_npos are defined in lookup_sentinels.hpp
// (included below, before NS_DJINTERP) so that bsearch.hpp can share
// them without a cyclic include back into this header.  They remain
// part of lookup.hpp's public surface - consumers including lookup.hpp
// see both names exactly as before.


// ===========================================================================
// II.  Result carriers
// ===========================================================================

// type_pack
//   trait: a type-level carrier for a pack of types.  Used as
// the result carrier for type_keys_of<...> and any trait that
// emits a type pack.
//
// Example:
//   using ts = type_pack<int, double, char>;
//   static_assert(ts::size == 3, "");
template<typename... Types>
struct type_pack
{
    static constexpr std::size_t size = sizeof...(Types);
};

// value_pack
//   trait: a type-level carrier for a pack of NTTP values.
// Used as the result carrier for keys_of<...> and as a return
// shape for any trait that emits a value pack.  Carries no
// instance state; only the pack and its size.
//
// Example:
//   using ks = value_pack<1, 2, 3>;
//   static_assert(ks::size == 3, "");
template<auto... Values>
struct value_pack
{
    static constexpr std::size_t size = sizeof...(Values);
};


// ===========================================================================
// III. NTTP-pack predicates
// ===========================================================================

// value_pack_contains
//   trait: true iff some value in the NTTP pack equals Needle.
// Short-circuits on the first match via the recursive ||.
template<auto    Needle,
         auto... Haystack>
struct value_pack_contains;

template<auto Needle>
struct value_pack_contains<Needle>
    : std::false_type
{};

template<auto    Needle,
         auto    First,
         auto... Rest>
struct value_pack_contains<Needle, First, Rest...>
    : std::integral_constant<bool,  ( (Needle == First) || value_pack_contains<Needle, Rest...>::value )>
{};

// value_pack_contains_v
//   value: convenience alias.
template<auto    Needle,
         auto... Haystack>
inline constexpr bool value_pack_contains_v = value_pack_contains<Needle, Haystack...>::value;

// value_pack_unique
//   trait: true iff every value in the NTTP pack is distinct.
// O(N^2): each step checks whether the head appears in the
// tail, then recurses on the tail.
template<auto... Haystack>
struct value_pack_unique;

template<>
struct value_pack_unique<> : std::true_type
{};

template<auto    First,
         auto... Rest>
struct value_pack_unique<First, Rest...>
    : std::integral_constant<bool, ((!value_pack_contains<First, Rest...>::value) &&
                                      value_pack_unique<Rest...>::value )>
{};

// value_pack_unique_v
//   value: convenience alias.
template<auto... Haystack>
inline constexpr bool value_pack_unique_v = value_pack_unique<Haystack...>::value;


// ===========================================================================
// IV.  Type-pack predicates
// ===========================================================================

// type_pack_contains
//   trait: true iff some type in the pack is the same as
// Needle.  Short-circuits on the first match.
template<typename    Needle,
         typename... Haystack>
struct type_pack_contains;

template<typename Needle>
struct type_pack_contains<Needle> : std::false_type
{};

template<typename    Needle,
         typename    First,
         typename... Rest>
struct type_pack_contains<Needle, First, Rest...>
    : std::integral_constant<bool,
        ( std::is_same<Needle, First>::value ||
          type_pack_contains<Needle, Rest...>::value )>
{};

// type_pack_contains_v
//   value: convenience alias.
template<typename    Needle,
         typename... Haystack>
inline constexpr bool type_pack_contains_v =
    type_pack_contains<Needle, Haystack...>::value;


// type_pack_unique
//   trait: true iff every type in the pack is distinct.
template<typename... Haystack>
struct type_pack_unique;

template<>
struct type_pack_unique<> : std::true_type
{};

template<typename    First,
         typename... Rest>
struct type_pack_unique<First, Rest...>
    : std::integral_constant<bool,
        ( !type_pack_contains<First, Rest...>::value &&
          type_pack_unique<Rest...>::value )>
{};

// type_pack_unique_v
//   value: convenience alias.
template<typename... Haystack>
inline constexpr bool type_pack_unique_v = type_pack_unique<Haystack...>::value;


// ===========================================================================
// V.   Value-keyed lookup
// ===========================================================================
//   Convention: each entry exposes a static NTTP member named
// ::key, whose value is the entry's lookup key.  Lookup matches
// via `==` between the needle and entry::key.

// find_by_key
//   trait: finds the first entry whose ::key equals Needle.
//
//   On match:  ::type    = the matching entry
//              ::found   = true
//              ::index   = position of the match
//   On miss:   ::type    = lookup_not_found
//              ::found   = false
//              ::index   = lookup_npos
//
// Example:
//   struct e1 { static constexpr int key = 10; };
//   struct e2 { static constexpr int key = 20; };
//   using f = find_by_key<20, e1, e2>;
//   // f::type   == e2
//   // f::found  == true
//   // f::index  == 1
template<auto        Needle,
         typename... Entries>
struct find_by_key;

// base case: empty pack - miss
template<auto Needle>
struct find_by_key<Needle>
{
    using type = lookup_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = lookup_npos;
};

// recursive case
template<auto        Needle,
         typename    Head,
         typename... Tail>
struct find_by_key<Needle, Head, Tail...>
{
private:
    static constexpr bool head_matches = (Head::key == Needle);

    using next_t = find_by_key<Needle, Tail...>;

public:
    using type = std::conditional_t<head_matches, Head, typename next_t::type>;

    static constexpr bool found = (head_matches || next_t::found);

    static constexpr std::size_t index =
        head_matches
            ? 0
            : ( (next_t::index == lookup_npos)
                  ? lookup_npos
                  : (next_t::index + 1) );
};

// find_by_key_t
//   type: convenience alias for find_by_key<...>::type.
template<auto       Needle,
         typename... Entries>
using find_by_key_t = typename find_by_key<Needle, Entries...>::type;


// contains_key
//   trait: true iff some entry's ::key equals Needle.  Equivalent
// to find_by_key<...>::found, but written standalone so it
// short-circuits via the recursive || without computing ::type.
template<auto        Needle,
         typename... Entries>
struct contains_key;

template<auto Needle>
struct contains_key<Needle> : std::false_type
{};

template<auto        Needle,
         typename    Head,
         typename... Tail>
struct contains_key<Needle, Head, Tail...>
    : std::integral_constant<bool,
        ( (Head::key == Needle) ||
          contains_key<Needle, Tail...>::value )>
{};

// contains_key_v
//   value: convenience alias.
template<auto        Needle,
         typename... Entries>
inline constexpr bool contains_key_v = contains_key<Needle, Entries...>::value;


// key_index_of
//   trait: returns the index of the first entry whose ::key
// equals Needle, or lookup_npos if no such entry exists.
template<auto       Needle,
         typename... Entries>
struct key_index_of;

template<auto Needle>
struct key_index_of<Needle>
    : std::integral_constant<std::size_t, lookup_npos>
{};

template<auto        Needle,
         typename    Head,
         typename... Tail>
struct key_index_of<Needle, Head, Tail...>
{
private:
    static constexpr bool head_matches = (Head::key == Needle);

    static constexpr std::size_t tail_index =
        key_index_of<Needle, Tail...>::value;

public:
    static constexpr std::size_t value =
        head_matches
            ? 0
            : ( (tail_index == lookup_npos)
                  ? lookup_npos
                  : (tail_index + 1) );
};

// key_index_of_v
//   value: convenience alias.
template<auto        Needle,
         typename... Entries>
inline constexpr std::size_t
key_index_of_v = key_index_of<Needle, Entries...>::value;

// keys_of
//   trait: emits a value_pack containing every entry's ::key
// value, in left-to-right order.  Useful for diagnostics, for
// passing to further metaprograms, or for emitting all keys at
// once.
//
// Example:
//   using ks = keys_of<e1, e2, e3>::type;
//   // ks == value_pack<e1::key, e2::key, e3::key>
template<typename... Entries>
struct keys_of
{
    using type = value_pack<Entries::key...>;
};

// keys_of_t
//   type: convenience alias for keys_of<...>::type.
template<typename... Entries>
using keys_of_t = typename keys_of<Entries...>::type;


// ===========================================================================
// VI.  Type-keyed lookup
// ===========================================================================
//   Convention: each entry exposes a `::key_type` alias whose
// value is the entry's lookup key (a type).  Lookup matches via
// std::is_same between the needle type and entry::key_type.

// find_by_type_key
//   trait: finds the first entry whose ::key_type is the same
// type as Needle.  Result members mirror find_by_key.
//
// Example:
//   struct e1 { using key_type = int;    };
//   struct e2 { using key_type = double; };
//   using f = find_by_type_key<double, e1, e2>;
//   // f::type  == e2
//   // f::found == true
//   // f::index == 1
template<typename    Needle,
         typename... Entries>
struct find_by_type_key;

// base case: empty pack - miss
template<typename Needle>
struct find_by_type_key<Needle>
{
    using type = lookup_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = lookup_npos;
};

// recursive case
template<typename    Needle,
         typename    Head,
         typename... Tail>
struct find_by_type_key<Needle, Head, Tail...>
{
private:
    static constexpr bool head_matches =
        std::is_same<typename Head::key_type, Needle>::value;

    using next_t = find_by_type_key<Needle, Tail...>;

public:
    using type =
        std::conditional_t<head_matches, Head, typename next_t::type>;

    static constexpr bool found = (head_matches || next_t::found);

    static constexpr std::size_t index =
        head_matches
            ? 0
            : ( (next_t::index == lookup_npos)
                  ? lookup_npos
                  : (next_t::index + 1) );
};

// find_by_type_key_t
//   type: convenience alias for find_by_type_key<...>::type.
template<typename    Needle,
         typename... Entries>
using find_by_type_key_t = typename find_by_type_key<Needle, Entries...>::type;

// contains_type_key
//   trait: true iff some entry's ::key_type is the same type
// as Needle.  Short-circuits via the recursive ||.
template<typename    Needle,
         typename... Entries>
struct contains_type_key;

template<typename Needle>
struct contains_type_key<Needle> : std::false_type
{};

template<typename    Needle,
         typename    Head,
         typename... Tail>
struct contains_type_key<Needle, Head, Tail...>
    : std::integral_constant<bool,
        ( std::is_same<typename Head::key_type, Needle>::value ||
          contains_type_key<Needle, Tail...>::value )>
{};

// contains_type_key_v
//   value: convenience alias.
template<typename    Needle,
         typename... Entries>
inline constexpr bool contains_type_key_v = contains_type_key<Needle, Entries...>::value;

// type_key_index_of
//   trait: returns the index of the first entry whose
// ::key_type is the same as Needle, or lookup_npos otherwise.
template<typename    Needle,
         typename... Entries>
struct type_key_index_of;

template<typename Needle>
struct type_key_index_of<Needle>
    : std::integral_constant<std::size_t, lookup_npos>
{};

template<typename    Needle,
         typename    Head,
         typename... Tail>
struct type_key_index_of<Needle, Head, Tail...>
{
private:
    static constexpr bool head_matches =
        std::is_same<typename Head::key_type, Needle>::value;

    static constexpr std::size_t tail_index =
        type_key_index_of<Needle, Tail...>::value;

public:
    static constexpr std::size_t value =
        head_matches
            ? 0
            : ( (tail_index == lookup_npos)
                  ? lookup_npos
                  : (tail_index + 1) );
};

// type_key_index_of_v
//   value: convenience alias.
template<typename    Needle,
         typename... Entries>
inline constexpr std::size_t type_key_index_of_v =
    type_key_index_of<Needle, Entries...>::value;


// type_keys_of
//   trait: emits a type_pack containing every entry's
// ::key_type, in left-to-right order.
//
// Example:
//   using ts = type_keys_of<e1, e2, e3>::type;
//   // ts == type_pack<e1::key_type, e2::key_type, e3::key_type>
template<typename... Entries>
struct type_keys_of
{
    using type = type_pack<typename Entries::key_type...>;
};

// type_keys_of_t
//   type: convenience alias for type_keys_of<...>::type.
template<typename... Entries>
using type_keys_of_t = typename type_keys_of<Entries...>::type;


// ===========================================================================
// VII. Predicate-driven lookup  (key-convention-agnostic)
// ===========================================================================
//   Unlike the value-keyed (V) and type-keyed (VI) families, these
// impose NO ::key or ::key_type convention on entries.  Matching is
// delegated to a caller-supplied predicate: a template<typename> class
// exposing a `static constexpr bool ::value`.  This is the most
// general locate primitive in this header - the keyed families are
// effectively special cases with a fixed predicate baked in.

// find_by_pred
//   trait: finds the first entry satisfying Predicate, scanning
// left to right.  Result members mirror find_by_key exactly:
//
//   On match:  ::type    = the matching entry
//              ::found   = true
//              ::index   = position of the match
//   On miss:   ::type    = lookup_not_found
//              ::found   = false
//              ::index   = lookup_npos
//
// Example:
//   template<typename Type> struct is_big
//       : std::integral_constant<bool, (sizeof(Type) >= 4)> {};
//   using f = find_by_pred<is_big, char, short, int>;
//   // f::type  == int
//   // f::found == true
//   // f::index == 2
template<template<typename> class Predicate,
         typename...               Entries>
struct find_by_pred;

// base case: empty pack - miss
template<template<typename> class Predicate>
struct find_by_pred<Predicate>
{
    using type = lookup_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = lookup_npos;
};

// recursive case
template<template<typename> class Predicate,
         typename                  Head,
         typename...               Tail>
struct find_by_pred<Predicate, Head, Tail...>
{
private:
    static constexpr bool head_matches = Predicate<Head>::value;

    using next_t = find_by_pred<Predicate, Tail...>;

public:
    using type =
        std::conditional_t<head_matches, Head, typename next_t::type>;

    static constexpr bool found = (head_matches || next_t::found);

    static constexpr std::size_t index =
        head_matches
            ? 0
            : ( (next_t::index == lookup_npos)
                  ? lookup_npos
                  : (next_t::index + 1) );
};

// find_by_pred_t
//   type: convenience alias for find_by_pred<...>::type.
template<template<typename> class Predicate,
         typename...               Entries>
using find_by_pred_t = typename find_by_pred<Predicate, Entries...>::type;


// contains_pred
//   trait: true iff some entry satisfies Predicate.  Equivalent to
// find_by_pred<...>::found, but standalone so it short-circuits via
// the recursive || without computing ::type.
template<template<typename> class Predicate,
         typename...               Entries>
struct contains_pred;

template<template<typename> class Predicate>
struct contains_pred<Predicate> : std::false_type
{};

template<template<typename> class Predicate,
         typename                  Head,
         typename...               Tail>
struct contains_pred<Predicate, Head, Tail...>
    : std::integral_constant<bool,
        ( Predicate<Head>::value ||
          contains_pred<Predicate, Tail...>::value )>
{};

// contains_pred_v
//   value: convenience alias.
template<template<typename> class Predicate,
         typename...               Entries>
inline constexpr bool contains_pred_v =
    contains_pred<Predicate, Entries...>::value;


// pred_index_of
//   trait: index of the first entry satisfying Predicate, or
// lookup_npos if none does.
template<template<typename> class Predicate,
         typename...               Entries>
struct pred_index_of;

template<template<typename> class Predicate>
struct pred_index_of<Predicate>
    : std::integral_constant<std::size_t, lookup_npos>
{};

template<template<typename> class Predicate,
         typename                  Head,
         typename...               Tail>
struct pred_index_of<Predicate, Head, Tail...>
{
private:
    static constexpr bool head_matches = Predicate<Head>::value;

    static constexpr std::size_t tail_index =
        pred_index_of<Predicate, Tail...>::value;

public:
    static constexpr std::size_t value =
        head_matches
            ? 0
            : ( (tail_index == lookup_npos)
                  ? lookup_npos
                  : (tail_index + 1) );
};

// pred_index_of_v
//   value: convenience alias.
template<template<typename> class Predicate,
         typename...               Entries>
inline constexpr std::size_t pred_index_of_v =
    pred_index_of<Predicate, Entries...>::value;


// ===========================================================================
// VIII. Sorted value-key lookup  (is_key_sorted + binary-search shorthands)
// ===========================================================================
//   Binary search for the value-keyed (::key) and type-keyed
// (::key_type) conventions.  The SEARCH ENGINE itself lives in
// bsearch.hpp (bsearch_by); the traits here are thin convention
// adapters - they form the engine's two needle predicates from a key
// needle and hand off.  Nothing here re-implements the search.
//
//   Preconditions are convention-specific and checked (in debug) by
// the matching sortedness trait below; define DJINTERP_NO_SORTED_ASSERT
// to compile those checks out where the O(N) scan is itself too costly.

// ---------------------------------------------------------------------------
// VIII.1  is_key_sorted  (value-key precondition predicate)
// ---------------------------------------------------------------------------

// is_key_sorted
//   trait: true iff the entries' ::key values are non-decreasing left
// to right (adjacent-pair check, `<=`).  Guards the value-keyed
// binary search and drives find_by_key_auto.  O(N) - cheap, not free.
template<typename... Entries>
struct is_key_sorted;

template<>
struct is_key_sorted<> : std::true_type
{};

template<typename Only>
struct is_key_sorted<Only> : std::true_type
{};

template<typename    A,
         typename    B,
         typename... Rest>
struct is_key_sorted<A, B, Rest...>
    : std::integral_constant<bool,
        ( (A::key <= B::key) &&
          is_key_sorted<B, Rest...>::value )>
{};

// is_key_sorted_v
//   value: convenience alias.
template<typename... Entries>
inline constexpr bool is_key_sorted_v = is_key_sorted<Entries...>::value;


// ---------------------------------------------------------------------------
// VIII.2  find_by_key_bsearch  (value-key shorthand)
// ---------------------------------------------------------------------------
//   Convention: entry::key is an NTTP, ordered by `<`.  Forms the
// engine's Below / Above predicates from Needle and calls
// bsearch_by.  Same result members and semantics as find_by_key (V.1),
// but O(log N) depth; precondition is_key_sorted_v<Entries...>.

NS_INTERNAL

    // nttp_key_preds
    //   helper: bakes an NTTP needle into the engine's predicate shape,
    // comparing entry::key with `<`.
    template<auto Needle>
    struct nttp_key_preds
    {
        template<typename Entry>
        struct below
            : std::integral_constant<bool, (Entry::key < Needle)>
        {};

        template<typename Entry>
        struct above
            : std::integral_constant<bool, (Needle < Entry::key)>
        {};
    };

NS_END  // internal

template<auto        Needle,
         typename... Entries>
struct find_by_key_bsearch
{
#ifndef DJINTERP_NO_SORTED_ASSERT
    static_assert(is_key_sorted_v<Entries...>,
                  "find_by_key_bsearch requires Entries sorted by ::key.");
#endif

private:
    using preds = internal::nttp_key_preds<Needle>;

public:
    using engine = bsearch_by<preds::template below,
                              preds::template above,
                              Entries...>;

    using type = typename engine::type;

    static constexpr bool        found = engine::found;
    static constexpr std::size_t index = engine::index;
};

// find_by_key_bsearch_t
//   type: convenience alias.
template<auto        Needle,
         typename... Entries>
using find_by_key_bsearch_t =
    typename find_by_key_bsearch<Needle, Entries...>::type;


// ---------------------------------------------------------------------------
// VIII.3  find_by_type_key_bsearch  (type-key shorthand; comparator required)
// ---------------------------------------------------------------------------
//   Convention: entry::key_type is a TYPE.  Types have no intrinsic
// order, so the caller MUST supply a strict-weak ordering as a binary
// type comparator Compare<A, B> with `static constexpr bool ::value`
// meaning "A sorts before B".  Entries must be sorted ascending
// under that same Compare.
//
// Example comparator:
//   template<typename A, typename B>
//   struct by_size : std::integral_constant<bool, (sizeof(A) < sizeof(B))> {};

NS_INTERNAL

    // type_key_preds
    //   helper: bakes a needle TYPE and a comparator into the engine's
    // predicate shape.  below(E) = Compare<E::key_type, needle>;
    // above(E) = Compare<needle, E::key_type>.
    template<typename                           Needle,
             template<typename, typename> class Compare>
    struct type_key_preds
    {
        template<typename Entry>
        struct below
            : std::integral_constant<bool,
                Compare<typename Entry::key_type, Needle>::value>
        {};

        template<typename Entry>
        struct above
            : std::integral_constant<bool,
                Compare<Needle, typename Entry::key_type>::value>
        {};
    };

    // type_keys_sorted
    //   helper: true iff entries' ::key_type are non-decreasing under
    // Compare (no adjacent pair strictly out of order).
    template<template<typename, typename> class Compare,
             typename...                         Entries>
    struct type_keys_sorted;

    template<template<typename, typename> class Compare>
    struct type_keys_sorted<Compare> : std::true_type
    {};

    template<template<typename, typename> class Compare,
             typename                            Only>
    struct type_keys_sorted<Compare, Only> : std::true_type
    {};

    template<template<typename, typename> class Compare,
             typename                           A,
             typename                           B,
             typename...                        Rest>
    struct type_keys_sorted<Compare, A, B, Rest...>
        : std::integral_constant<bool,
            ( !Compare<typename B::key_type,
                        typename A::key_type>::value &&
              type_keys_sorted<Compare, B, Rest...>::value )>
    {};

NS_END  // internal

template<template<typename, typename> class Compare,
         typename                            Needle,
         typename...                         Entries>
struct find_by_type_key_bsearch
{
#ifndef DJINTERP_NO_SORTED_ASSERT
    static_assert(internal::type_keys_sorted<Compare, Entries...>::value,
                  "find_by_type_key_bsearch requires Entries sorted by "
                  "::key_type under Compare.");
#endif

private:
    using preds = internal::type_key_preds<Needle, Compare>;

public:
    using engine = bsearch_by<preds::template below,
                              preds::template above,
                              Entries...>;

    using type = typename engine::type;

    static constexpr bool        found = engine::found;
    static constexpr std::size_t index = engine::index;
};

// find_by_type_key_bsearch_t
//   type: convenience alias.
template<template<typename, typename> class Compare,
         typename                            Needle,
         typename...                         Entries>
using find_by_type_key_bsearch_t =
    typename find_by_type_key_bsearch<Compare, Needle, Entries...>::type;


// ---------------------------------------------------------------------------
// VIII.4  find_by_key_auto  (dispatch - read the caveat)
// ---------------------------------------------------------------------------

// find_by_key_auto
//   trait: dispatches to find_by_key_bsearch when is_key_sorted_v
// holds, else to the linear find_by_key (V.1).  Same result members.
//
//   CAVEAT - not a free speedup.  Deciding the branch runs is_key_sorted
// (O(N)) BEFORE searching, so on top of the search you always pay the
// sortedness scan; for small N this is strictly more work than calling
// find_by_key outright.  Prefer find_by_key_bsearch explicitly when you
// already know the data is sorted; reach for auto only when the caller
// genuinely cannot know and correctness-on-either-input matters more
// than compile cost.
template<auto        Needle,
         typename... Entries>
struct find_by_key_auto
    : std::conditional_t<is_key_sorted_v<Entries...>,
                         find_by_key_bsearch<Needle, Entries...>,
                         find_by_key<Needle, Entries...>>
{};

// find_by_key_auto_t
//   type: convenience alias.
template<auto        Needle,
         typename... Entries>
using find_by_key_auto_t = typename find_by_key_auto<Needle, Entries...>::type;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_UTIL_LOOKUP_LOOKUP_HPP
