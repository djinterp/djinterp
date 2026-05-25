/******************************************************************************
* djinterp [meta]                                                  lookup.hpp
*
*   Compile-time lookup-by-key idioms over type packs.  Domain-agnostic:
* knows nothing about options, settings, commands, configs, or any other
* downstream concept.  Whatever your domain is, if its entries expose
* either a value-key (a static `::key` member) or a type-key (a `::key_type`
* alias), the traits here can search, count, locate, and project them.
*
*   Two parallel families, distinguished by what the entries' keys ARE:
*
*     Value-keyed entries (entry::key is an NTTP value):
*       find_by_key<_Key, _Entries...>          - first match or sentinel
*       contains_key<_Key, _Entries...>         - bool trait
*       key_index_of<_Key, _Entries...>         - size_t (or lookup_npos)
*       keys_of<_Entries...>                    - value_pack of every key
*
*     Type-keyed entries (entry::key_type is a type):
*       find_by_type_key<_Type, _Entries...>    - first match or sentinel
*       contains_type_key<_Type, _Entries...>   - bool trait
*       type_key_index_of<_Type, _Entries...>   - size_t (or lookup_npos)
*       type_keys_of<_Entries...>               - type_pack of every key
*
*   Plus two carrier shapes:
*
*     value_pack<auto...>     - results carrier for NTTP packs
*     type_pack<typename...>  - results carrier for type packs
*
*   And two NTTP-pack predicates (also useful in their own right):
*
*     value_pack_contains<_Needle, _Pack...>    - bool trait
*     value_pack_unique<_Pack...>               - bool trait
*
*   Search policy is first-match-wins on a left-to-right walk, matching
* the intuition from std::find / std::ranges::find.  Misses yield the
* `lookup_not_found` sentinel and the `lookup_npos` index, both
* well-typed and inspectable.  Recursion is O(N) per lookup; the
* boolean predicates short-circuit via the recursive ||.
*
*
* path:      /inc/djinterp/core/meta/lookup.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.24
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Sentinels and constants
      1. lookup_not_found
      2. lookup_npos
II.   Result carriers
      1. value_pack
      2. type_pack
III.  NTTP-pack predicates
      1. value_pack_contains
      2. value_pack_unique
IV.   Type-pack predicates
      1. type_pack_contains
      2. type_pack_unique
V.    Value-keyed lookup (convention: entry::key is an NTTP)
      1. find_by_key
      2. contains_key
      3. key_index_of
      4. keys_of
VI.   Type-keyed lookup (convention: entry::key_type is a type)
      1. find_by_type_key
      2. contains_type_key
      3. type_key_index_of
      4. type_keys_of
*/

#ifndef DJINTERP_LOOKUP_
#define DJINTERP_LOOKUP_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Sentinels and constants
// ===========================================================================

// lookup_not_found
//   type: sentinel returned by find_*<...>::type when no entry
// matches the search.  Inspectable: callers can detect a miss
// either via the ::found bool or by checking the returned type
// against this tag.
struct lookup_not_found
{};

// lookup_npos
//   value: sentinel index returned by *_index_of<...>::value
// when no entry matches the search.  Mirrors std::string::npos
// in spirit: a size_t value distinguishable from any real index.
inline constexpr std::size_t lookup_npos =
    static_cast<std::size_t>(-1);


// ===========================================================================
// II.  Result carriers
// ===========================================================================

// value_pack
//   trait: a type-level carrier for a pack of NTTP values.
// Used as the result carrier for keys_of<...> and as a return
// shape for any trait that emits a value pack.  Carries no
// instance state; only the pack and its size.
//
// Example:
//   using ks = value_pack<1, 2, 3>;
//   static_assert(ks::size == 3, "");
template<auto... _Values>
struct value_pack
{
    static constexpr std::size_t size = sizeof...(_Values);
};

// type_pack
//   trait: a type-level carrier for a pack of types.  Used as
// the result carrier for type_keys_of<...> and any trait that
// emits a type pack.
//
// Example:
//   using ts = type_pack<int, double, char>;
//   static_assert(ts::size == 3, "");
template<typename... _Types>
struct type_pack
{
    static constexpr std::size_t size = sizeof...(_Types);
};


// ===========================================================================
// III. NTTP-pack predicates
// ===========================================================================

// value_pack_contains
//   trait: true iff some value in the NTTP pack equals _Needle.
// Short-circuits on the first match via the recursive ||.
template<auto    _Needle,
         auto... _Pack>
struct value_pack_contains;

template<auto _Needle>
struct value_pack_contains<_Needle> : std::false_type
{};

template<auto    _Needle,
         auto    _First,
         auto... _Rest>
struct value_pack_contains<_Needle, _First, _Rest...>
    : std::integral_constant<bool,
        ( (_Needle == _First) ||
          value_pack_contains<_Needle, _Rest...>::value )>
{};

// value_pack_contains_v
//   value: convenience alias.
template<auto    _Needle,
         auto... _Pack>
inline constexpr bool value_pack_contains_v =
    value_pack_contains<_Needle, _Pack...>::value;


// value_pack_unique
//   trait: true iff every value in the NTTP pack is distinct.
// O(N^2): each step checks whether the head appears in the
// tail, then recurses on the tail.
template<auto... _Pack>
struct value_pack_unique;

template<>
struct value_pack_unique<> : std::true_type
{};

template<auto    _First,
         auto... _Rest>
struct value_pack_unique<_First, _Rest...>
    : std::integral_constant<bool,
        ( !value_pack_contains<_First, _Rest...>::value &&
          value_pack_unique<_Rest...>::value )>
{};

// value_pack_unique_v
//   value: convenience alias.
template<auto... _Pack>
inline constexpr bool value_pack_unique_v =
    value_pack_unique<_Pack...>::value;


// ===========================================================================
// IV.  Type-pack predicates
// ===========================================================================

// type_pack_contains
//   trait: true iff some type in the pack is the same as
// _Needle.  Short-circuits on the first match.
template<typename    _Needle,
         typename... _Pack>
struct type_pack_contains;

template<typename _Needle>
struct type_pack_contains<_Needle> : std::false_type
{};

template<typename    _Needle,
         typename    _First,
         typename... _Rest>
struct type_pack_contains<_Needle, _First, _Rest...>
    : std::integral_constant<bool,
        ( std::is_same<_Needle, _First>::value ||
          type_pack_contains<_Needle, _Rest...>::value )>
{};

// type_pack_contains_v
//   value: convenience alias.
template<typename    _Needle,
         typename... _Pack>
inline constexpr bool type_pack_contains_v =
    type_pack_contains<_Needle, _Pack...>::value;


// type_pack_unique
//   trait: true iff every type in the pack is distinct.
template<typename... _Pack>
struct type_pack_unique;

template<>
struct type_pack_unique<> : std::true_type
{};

template<typename    _First,
         typename... _Rest>
struct type_pack_unique<_First, _Rest...>
    : std::integral_constant<bool,
        ( !type_pack_contains<_First, _Rest...>::value &&
          type_pack_unique<_Rest...>::value )>
{};

// type_pack_unique_v
//   value: convenience alias.
template<typename... _Pack>
inline constexpr bool type_pack_unique_v =
    type_pack_unique<_Pack...>::value;


// ===========================================================================
// V.   Value-keyed lookup
// ===========================================================================
//   Convention: each entry exposes a static NTTP member named
// ::key, whose value is the entry's lookup key.  Lookup matches
// via `==` between the needle and entry::key.

// find_by_key
//   trait: finds the first entry whose ::key equals _Needle.
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
template<auto       _Needle,
         typename... _Entries>
struct find_by_key;

// base case: empty pack - miss
template<auto _Needle>
struct find_by_key<_Needle>
{
    using type = lookup_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = lookup_npos;
};

// recursive case
template<auto       _Needle,
         typename   _Head,
         typename... _Tail>
struct find_by_key<_Needle, _Head, _Tail...>
{
private:
    static constexpr bool head_matches = (_Head::key == _Needle);

    using next_t = find_by_key<_Needle, _Tail...>;

public:
    using type =
        std::conditional_t<head_matches, _Head, typename next_t::type>;

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
template<auto       _Needle,
         typename... _Entries>
using find_by_key_t = typename find_by_key<_Needle, _Entries...>::type;


// contains_key
//   trait: true iff some entry's ::key equals _Needle.  Equivalent
// to find_by_key<...>::found, but written standalone so it
// short-circuits via the recursive || without computing ::type.
template<auto       _Needle,
         typename... _Entries>
struct contains_key;

template<auto _Needle>
struct contains_key<_Needle> : std::false_type
{};

template<auto       _Needle,
         typename   _Head,
         typename... _Tail>
struct contains_key<_Needle, _Head, _Tail...>
    : std::integral_constant<bool,
        ( (_Head::key == _Needle) ||
          contains_key<_Needle, _Tail...>::value )>
{};

// contains_key_v
//   value: convenience alias.
template<auto       _Needle,
         typename... _Entries>
inline constexpr bool contains_key_v =
    contains_key<_Needle, _Entries...>::value;


// key_index_of
//   trait: returns the index of the first entry whose ::key
// equals _Needle, or lookup_npos if no such entry exists.
template<auto       _Needle,
         typename... _Entries>
struct key_index_of;

template<auto _Needle>
struct key_index_of<_Needle>
    : std::integral_constant<std::size_t, lookup_npos>
{};

template<auto       _Needle,
         typename   _Head,
         typename... _Tail>
struct key_index_of<_Needle, _Head, _Tail...>
{
private:
    static constexpr bool head_matches = (_Head::key == _Needle);

    static constexpr std::size_t tail_index =
        key_index_of<_Needle, _Tail...>::value;

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
template<auto       _Needle,
         typename... _Entries>
inline constexpr std::size_t key_index_of_v =
    key_index_of<_Needle, _Entries...>::value;


// keys_of
//   trait: emits a value_pack containing every entry's ::key
// value, in left-to-right order.  Useful for diagnostics, for
// passing to further metaprograms, or for emitting all keys at
// once.
//
// Example:
//   using ks = keys_of<e1, e2, e3>::type;
//   // ks == value_pack<e1::key, e2::key, e3::key>
template<typename... _Entries>
struct keys_of
{
    using type = value_pack<_Entries::key...>;
};

// keys_of_t
//   type: convenience alias for keys_of<...>::type.
template<typename... _Entries>
using keys_of_t = typename keys_of<_Entries...>::type;


// ===========================================================================
// VI.  Type-keyed lookup
// ===========================================================================
//   Convention: each entry exposes a `::key_type` alias whose
// value is the entry's lookup key (a type).  Lookup matches via
// std::is_same between the needle type and entry::key_type.

// find_by_type_key
//   trait: finds the first entry whose ::key_type is the same
// type as _Needle.  Result members mirror find_by_key.
//
// Example:
//   struct e1 { using key_type = int;    };
//   struct e2 { using key_type = double; };
//   using f = find_by_type_key<double, e1, e2>;
//   // f::type  == e2
//   // f::found == true
//   // f::index == 1
template<typename    _Needle,
         typename... _Entries>
struct find_by_type_key;

// base case: empty pack - miss
template<typename _Needle>
struct find_by_type_key<_Needle>
{
    using type = lookup_not_found;

    static constexpr bool        found = false;
    static constexpr std::size_t index = lookup_npos;
};

// recursive case
template<typename    _Needle,
         typename    _Head,
         typename... _Tail>
struct find_by_type_key<_Needle, _Head, _Tail...>
{
private:
    static constexpr bool head_matches =
        std::is_same<typename _Head::key_type, _Needle>::value;

    using next_t = find_by_type_key<_Needle, _Tail...>;

public:
    using type =
        std::conditional_t<head_matches, _Head, typename next_t::type>;

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
template<typename    _Needle,
         typename... _Entries>
using find_by_type_key_t =
    typename find_by_type_key<_Needle, _Entries...>::type;


// contains_type_key
//   trait: true iff some entry's ::key_type is the same type
// as _Needle.  Short-circuits via the recursive ||.
template<typename    _Needle,
         typename... _Entries>
struct contains_type_key;

template<typename _Needle>
struct contains_type_key<_Needle> : std::false_type
{};

template<typename    _Needle,
         typename    _Head,
         typename... _Tail>
struct contains_type_key<_Needle, _Head, _Tail...>
    : std::integral_constant<bool,
        ( std::is_same<typename _Head::key_type, _Needle>::value ||
          contains_type_key<_Needle, _Tail...>::value )>
{};

// contains_type_key_v
//   value: convenience alias.
template<typename    _Needle,
         typename... _Entries>
inline constexpr bool contains_type_key_v =
    contains_type_key<_Needle, _Entries...>::value;


// type_key_index_of
//   trait: returns the index of the first entry whose
// ::key_type is the same as _Needle, or lookup_npos otherwise.
template<typename    _Needle,
         typename... _Entries>
struct type_key_index_of;

template<typename _Needle>
struct type_key_index_of<_Needle>
    : std::integral_constant<std::size_t, lookup_npos>
{};

template<typename    _Needle,
         typename    _Head,
         typename... _Tail>
struct type_key_index_of<_Needle, _Head, _Tail...>
{
private:
    static constexpr bool head_matches =
        std::is_same<typename _Head::key_type, _Needle>::value;

    static constexpr std::size_t tail_index =
        type_key_index_of<_Needle, _Tail...>::value;

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
template<typename    _Needle,
         typename... _Entries>
inline constexpr std::size_t type_key_index_of_v =
    type_key_index_of<_Needle, _Entries...>::value;


// type_keys_of
//   trait: emits a type_pack containing every entry's
// ::key_type, in left-to-right order.
//
// Example:
//   using ts = type_keys_of<e1, e2, e3>::type;
//   // ts == type_pack<e1::key_type, e2::key_type, e3::key_type>
template<typename... _Entries>
struct type_keys_of
{
    using type = type_pack<typename _Entries::key_type...>;
};

// type_keys_of_t
//   type: convenience alias for type_keys_of<...>::type.
template<typename... _Entries>
using type_keys_of_t = typename type_keys_of<_Entries...>::type;


NS_END  // djinterp


#endif  // DJINTERP_LOOKUP_
