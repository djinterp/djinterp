/*******************************************************************************
* djinterp [core]                                            enum_map_traits.hpp
*
* Compile-time structural traits for enum-keyed map containers.
*   Extends map_traits.hpp with detection and classification specific to
* maps whose key type is an enumeration.  Provides:
*
*   1. Enum introspection traits (operate on the enum type itself):
*      - scoped vs unscoped classification
*      - underlying integral type extraction
*      - cardinality detection via three protocols:
*        (a) user specialization of enum_info<E>,
*        (b) sentinel enumerator (_count / _last),
*        (c) explicit first/last range.
*      - dense enum detection (contiguous values from a known base)
*
*   2. Enum-map traits (operate on the map container):
*      - is the key type a known-cardinality enum?
*      - is the map completeness-checkable (runtime predicate)?
*      - DEnumMapStrategy: dense_array, sparse_sorted, sparse_hashed,
*        sparse_linear -- determines the optimal backing layout.
*      - Strategy deduction based on enum and backing properties.
*
*   3. Combined classification: enum_map_class<T> aggregates all
*      detection into a single struct, following container_class<T>.
*
* ENUM CARDINALITY PROTOCOL:
*   The framework needs to know how many enumerators exist.  Three
* mechanisms are supported, checked in priority order:
*
*   Priority 1: enum_info<E> specialization (user-provided).
*     template<>
*     struct enum_info<Color>
*     {
*         static constexpr std::size_t count = 3;
*         static constexpr Color       first = Color::red;
*         static constexpr Color       last  = Color::blue;
*     };
*
*   Priority 2: sentinel enumerator.
*     enum Color { red, green, blue, _count };
*     // detected via Color::_count or Color::count
*
*   Priority 3: first/last range sentinels.
*     enum Color { _first = 0, red = 0, green, blue, _last = 2 };
*     // detected via Color::_first / Color::_last
*
*   If none of these are available, cardinality is unknown and
* dense optimization is disabled.
*
* NAMING CONVENTION:
*   Expression detectors:    enum_<method>_expr_t
*   Tagged struct traits:    has_enum_<cap> / is_enum_<prop>
*   Variable template _v:   has_enum_<cap>_v / is_enum_<prop>_v
*   Tagless traits:          enum_map_can_<action>
*                            enum_map_does_<category>
*                            enum_map_is_<identity>
*
* DEPENDENCIES:
*   djinterp.hpp               -- namespace macros, clean_t
*   type_traits.hpp            -- detection idiom, conjunction, void_t
*   container_traits.hpp       -- has_key_type, iteration classification
*   map_traits.hpp             -- has_enum_key, is_map_structured
*
*
* path:      /inc/djinterp/core/container/map/enum_map_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.30
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Enum Introspection Primitives
      -----------------------------

II.   Enum Cardinality Protocol (enum_info)
      -------------------------------------

III.  Sentinel Detection
      ------------------

IV.   Cardinality Resolution
      ----------------------

V.    Dense Enum Detection
      --------------------

VI.   Enum Map Strategy Enum
      ----------------------

VII.  Strategy Deduction
      ------------------

VIII. Tagged Struct Traits (has_/is_)
      -------------------------------

IX.   Tagless Constexpr Bool Traits (C++17)
      -------------------------------------

X.    Combined Classification (enum_map_class)
      ----------------------------------------
*/

#ifndef DJINTERP_CONTAINER_MAP_ENUM_MAP_TRAITS_HPP
#define DJINTERP_CONTAINER_MAP_ENUM_MAP_TRAITS_HPP 1

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
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"
#include "./map_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// =============================================================================
// I.   Enum Introspection Primitives
// =============================================================================
// Traits that operate on an enum type directly, independent
// of any container.

// is_scoped_enum
//   trait: true if E is a scoped enum (enum class). Scoped enums do not
// implicitly convert to their underlying integral type.
template<typename E,
         typename = void>
struct is_scoped_enum : std::false_type
{};

// is_scoped_enum<E, std::enable_if_t< std::is_enum_v<E> &&
// !std::is_convertible_v< E, std::underlying_type_t<E>> >>
//   trait: the `std::enable_if_t< std::is_enum_v<E> &&
// !std::is_convertible_v< E, std::underlying_type_t<E>> >` case; it reports
// true.
template<typename E>
struct is_scoped_enum<E,
    std::enable_if_t<
        std::is_enum_v<E> &&
        !std::is_convertible_v<
            E, std::underlying_type_t<E>>
    >> : std::true_type
{};

template<typename E>
inline constexpr bool is_scoped_enum_v =
    is_scoped_enum<E>::value;

// is_unscoped_enum
//   trait: true if E is an unscoped enum (plain enum).
template<typename E>
struct is_unscoped_enum
{
    static constexpr bool value =
        ( std::is_enum_v<E> &&
          !is_scoped_enum_v<E> );
};

template<typename E>
inline constexpr bool is_unscoped_enum_v =
    is_unscoped_enum<E>::value;

// enum_underlying_type_of
//   trait: extracts the underlying integral type of an enum. Yields void for
// non-enum types.
NS_INTERNAL

    template<typename E,
             typename = void>
    struct enum_underlying_helper
    {
        using type = void;
    };

    template<typename E>
    struct enum_underlying_helper<E,
        std::enable_if_t<std::is_enum_v<E>>>
    {
        using type = std::underlying_type_t<E>;
    };

NS_END  // internal

template<typename E>
struct enum_underlying_type_of
{
    using type =
        typename internal::enum_underlying_helper<E>::type;
};

// enum_underlying_type_of_t
//   type: the carrier of enum_underlying_type_of -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename E>
using enum_underlying_type_of_t =
    typename enum_underlying_type_of<E>::type;


// =============================================================================
// II.  Enum Cardinality Protocol            (enum_info)
// =============================================================================
// The primary customization point for communicating enum
// cardinality to the trait system.  Users specialize this
// struct for their enum types.
//
// Default: all members are absent.  Specializations should
// provide:
//   static constexpr std::size_t count  -- number of enumerators.
//   static constexpr E           first  -- first enumerator value.
//   static constexpr E           last   -- last enumerator value.
//
// `first` and `last` are optional; `count` alone is sufficient
// for completeness checking.  All three enable dense detection.

// enum_info
//   trait: primary template. Specializations provide count, first, and last
// for specific enum types.
template<typename E,
         typename = void>
struct enum_info
{
    // intentionally empty: no cardinality information available for
    // unspecialized enums.
};

// --- detection of enum_info specialization ---

// has_enum_info_count
//   trait: true if enum_info<E> provides a count member.
template<typename E,
         typename = void>
struct has_enum_info_count : std::false_type
{};

// has_enum_info_count<E, std::void_t< decltype(enum_info<E>
//   trait: the `std::void_t< decltype(enum_info<E` case; it reports true.
template<typename E>
struct has_enum_info_count<E,
    std::void_t<
        decltype(enum_info<E>::count)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_info_count_v =
    has_enum_info_count<E>::value;

// has_enum_info_first
//   trait: true if enum_info<E> provides a first member.
template<typename E,
         typename = void>
struct has_enum_info_first : std::false_type
{};

// has_enum_info_first<E, std::void_t< decltype(enum_info<E>
//   trait: the `std::void_t< decltype(enum_info<E` case; it reports true.
template<typename E>
struct has_enum_info_first<E,
    std::void_t<
        decltype(enum_info<E>::first)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_info_first_v =
    has_enum_info_first<E>::value;

// has_enum_info_last
//   trait: true if enum_info<E> provides a last member.
template<typename E,
         typename = void>
struct has_enum_info_last : std::false_type
{};

// has_enum_info_last<E, std::void_t< decltype(enum_info<E>
//   trait: the `std::void_t< decltype(enum_info<E` case; it reports true.
template<typename E>
struct has_enum_info_last<E,
    std::void_t<
        decltype(enum_info<E>::last)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_info_last_v =
    has_enum_info_last<E>::value;

// has_enum_info_range
//   trait: true if enum_info<E> provides both first and last.
template<typename E>
struct has_enum_info_range
{
    static constexpr bool value =
        ( has_enum_info_first_v<E> &&
          has_enum_info_last_v<E> );
};

template<typename E>
inline constexpr bool has_enum_info_range_v =
    has_enum_info_range<E>::value;

// has_full_enum_info
//   trait: true if enum_info<E> provides count, first, and last.
template<typename E>
struct has_full_enum_info
{
    static constexpr bool value =
        ( has_enum_info_count_v<E> &&
          has_enum_info_range_v<E> );
};

template<typename E>
inline constexpr bool has_full_enum_info_v =
    has_full_enum_info<E>::value;


// =============================================================================
// III. Sentinel Detection
// =============================================================================
// Detect common sentinel patterns in enum definitions:
//   _count / count:   trailing enumerator giving cardinality.
//   _first / _last:   range boundary sentinels.
//
// Detection uses expression SFINAE.  If the enumerator exists,
// the expression E::_count is well-formed.

// --- _count sentinel ---

// has_enum_sentinel_count
//   trait: true if E has E::_count enumerator.
template<typename E,
         typename = void>
struct has_enum_sentinel_count : std::false_type
{};

// has_enum_sentinel_count<E, std::void_t< decltype(E::_count) >>
//   trait: the `std::void_t< decltype(E::_count) >` case; it reports true.
template<typename E>
struct has_enum_sentinel_count<E,
    std::void_t<
        decltype(E::_count)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_sentinel_count_v =
    has_enum_sentinel_count<E>::value;

// --- count sentinel (no underscore) ---

// has_enum_sentinel_count_alt
//   trait: true if E has E::count enumerator.
template<typename E,
         typename = void>
struct has_enum_sentinel_count_alt : std::false_type
{};

// has_enum_sentinel_count_alt<E, std::void_t< decltype(E::count) >>
//   trait: the `std::void_t< decltype(E::count) >` case; it reports true.
template<typename E>
struct has_enum_sentinel_count_alt<E,
    std::void_t<
        decltype(E::count)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_sentinel_count_alt_v =
    has_enum_sentinel_count_alt<E>::value;

// --- _first sentinel ---

// has_enum_sentinel_first
//   trait: true if E has E::_first enumerator.
template<typename E,
         typename = void>
struct has_enum_sentinel_first : std::false_type
{};

// has_enum_sentinel_first<E, std::void_t< decltype(E::_first) >>
//   trait: the `std::void_t< decltype(E::_first) >` case; it reports true.
template<typename E>
struct has_enum_sentinel_first<E,
    std::void_t<
        decltype(E::_first)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_sentinel_first_v =
    has_enum_sentinel_first<E>::value;

// --- _last sentinel ---

// has_enum_sentinel_last
//   trait: true if E has E::_last enumerator.
template<typename E,
         typename = void>
struct has_enum_sentinel_last : std::false_type
{};

// has_enum_sentinel_last<E, std::void_t< decltype(E::_last) >>
//   trait: the `std::void_t< decltype(E::_last) >` case; it reports true.
template<typename E>
struct has_enum_sentinel_last<E,
    std::void_t<
        decltype(E::_last)
    >> : std::true_type
{};

template<typename E>
inline constexpr bool has_enum_sentinel_last_v =
    has_enum_sentinel_last<E>::value;

// has_enum_sentinel_range
//   trait: true if E has both _first and _last sentinels.
template<typename E>
struct has_enum_sentinel_range
{
    static constexpr bool value =
        ( has_enum_sentinel_first_v<E> &&
          has_enum_sentinel_last_v<E> );
};

template<typename E>
inline constexpr bool has_enum_sentinel_range_v =
    has_enum_sentinel_range<E>::value;

// has_any_enum_sentinel
//   trait: true if the enum provides any sentinel-based cardinality
// information.
template<typename E>
struct has_any_enum_sentinel
{
    static constexpr bool value =
        ( has_enum_sentinel_count_v<E>     ||
          has_enum_sentinel_count_alt_v<E> ||
          has_enum_sentinel_range_v<E> );
};

template<typename E>
inline constexpr bool has_any_enum_sentinel_v =
    has_any_enum_sentinel<E>::value;


// =============================================================================
// IV.  Cardinality Resolution
// =============================================================================
// Resolves the enum cardinality from the highest-priority
// available source.  The result is a constexpr std::size_t.
//
// Priority:
//   1. enum_info<E>::count        (user specialization)
//   2. static_cast<size_t>(E::_count)
//   3. static_cast<size_t>(E::count)
//   4. _last - _first + 1         (from sentinels or enum_info)
//   5. unknown (0)

// has_known_enum_count
//   trait: true if the enum's cardinality can be determined at compile time by
// any protocol.
template<typename E>
struct has_known_enum_count
{
    static constexpr bool value =
        ( has_enum_info_count_v<E>         ||
          has_enum_sentinel_count_v<E>     ||
          has_enum_sentinel_count_alt_v<E> ||
          has_enum_sentinel_range_v<E>     ||
          has_enum_info_range_v<E> );
};

template<typename E>
inline constexpr bool has_known_enum_count_v =
    has_known_enum_count<E>::value;

// enum_count_of
//   trait: resolves the enum cardinality. Returns 0 if unknown.
NS_INTERNAL

    // --- dispatched resolution via bool template params --- Parameters encode
    // which protocol is available, in priority order. The first `true` wins.

    template<typename E,
             bool = has_enum_info_count_v<E>,
             bool = has_enum_sentinel_count_v<E>,
             bool = has_enum_sentinel_count_alt_v<E>,
             bool = has_enum_sentinel_range_v<E>,
             bool = has_enum_info_range_v<E>>
    struct enum_count_resolver
    {
        // fallback: unknown
        static constexpr std::size_t value = 0;
    };

    template<typename E,
             bool B2, bool B3, bool B4, bool B5>
    struct enum_count_resolver<
        E, true, B2, B3, B4, B5>
    {
        static constexpr std::size_t value =
            enum_info<E>::count;
    };

    template<typename E,
             bool B3, bool B4, bool B5>
    struct enum_count_resolver<
        E, false, true, B3, B4, B5>
    {
        static constexpr std::size_t value =
            static_cast<std::size_t>(E::_count);
    };

    template<typename E,
             bool B4, bool B5>
    struct enum_count_resolver<
        E, false, false, true, B4, B5>
    {
        static constexpr std::size_t value =
            static_cast<std::size_t>(E::count);
    };

    // priority 4: sentinel range _first.._last
    template<typename E,
             bool B5>
    struct enum_count_resolver<
        E, false, false, false, true, B5>
    {
        static constexpr std::size_t value =
            static_cast<std::size_t>(E::_last) -
            static_cast<std::size_t>(E::_first) + 1;
    };

    // priority 5: enum_info range first..last
    template<typename E>
    struct enum_count_resolver<
        E, false, false, false, false, true>
    {
        static constexpr std::size_t value =
            static_cast<std::size_t>(
                enum_info<E>::last) -
            static_cast<std::size_t>(
                enum_info<E>::first) + 1;
    };

NS_END  // internal

template<typename E>
struct enum_count_of
{
    static constexpr std::size_t value =
        internal::enum_count_resolver<E>::value;
};

template<typename E>
inline constexpr std::size_t enum_count_of_v =
    enum_count_of<E>::value;


// --- first value resolution ---

// has_known_enum_first
//   trait: true if we can determine the first enumerator.
template<typename E>
struct has_known_enum_first
{
    static constexpr bool value =
        ( has_enum_info_first_v<E> ||
          has_enum_sentinel_first_v<E> );
};

template<typename E>
inline constexpr bool has_known_enum_first_v =
    has_known_enum_first<E>::value;

// has_known_enum_last
//   trait: true if we can determine the last enumerator.
template<typename E>
struct has_known_enum_last
{
    static constexpr bool value =
        ( has_enum_info_last_v<E> ||
          has_enum_sentinel_last_v<E> );
};

template<typename E>
inline constexpr bool has_known_enum_last_v =
    has_known_enum_last<E>::value;

// has_known_enum_range
//   trait: true if both first and last are determinable.
template<typename E>
struct has_known_enum_range
{
    static constexpr bool value =
        ( has_known_enum_first_v<E> &&
          has_known_enum_last_v<E> );
};

template<typename E>
inline constexpr bool has_known_enum_range_v =
    has_known_enum_range<E>::value;

// enum_first_of
//   trait: resolves the first enumerator value as size_t. Returns 0 if unknown
// (convention: unknown implies 0-based).
NS_INTERNAL

    template<typename E,
             bool = has_enum_info_first_v<E>,
             bool = has_enum_sentinel_first_v<E>>
    struct enum_first_resolver
    {
        // fallback: assume 0-based
        static constexpr std::size_t value = 0;
    };

    template<typename E, bool B2>
    struct enum_first_resolver<E, true, B2>
    {
        static constexpr std::size_t value =
            static_cast<std::size_t>(
                enum_info<E>::first);
    };

    template<typename E>
    struct enum_first_resolver<E, false, true>
    {
        static constexpr std::size_t value =
            static_cast<std::size_t>(E::_first);
    };

NS_END  // internal

template<typename E>
struct enum_first_of
{
    static constexpr std::size_t value =
        internal::enum_first_resolver<E>::value;
};

template<typename E>
inline constexpr std::size_t enum_first_of_v =
    enum_first_of<E>::value;


// =============================================================================
// V.   Dense Enum Detection
// =============================================================================
// A dense enum has contiguous underlying values from a known
// base, enabling O(1) array-indexed lookup.  Requirements:
//   1. Cardinality is known (and nonzero).
//   2. The range [first, first + count) is contiguous.
//      (Assumed when the user provides enum_info or uses
//       sequential enum definitions with sentinels.)

// is_dense_enum
//   trait: true if the enum is suitable for array-indexed storage. Requires
// known cardinality and either a known first value or a sentinel-based count.
template<typename E>
struct is_dense_enum
{
    static constexpr bool value =
        ( std::is_enum_v<E>               &&
          has_known_enum_count_v<E>       &&
          enum_count_of_v<E> > 0          &&
          ( has_known_enum_first_v<E>     ||
            has_enum_sentinel_count_v<E>  ||
            has_enum_sentinel_count_alt_v<E> ) );
};

template<typename E>
inline constexpr bool is_dense_enum_v =
    is_dense_enum<E>::value;

// is_zero_based_dense_enum
//   trait: true if the dense enum starts at 0. Enables direct
// static_cast<size_t>(key) indexing without subtracting a base offset.
template<typename E>
struct is_zero_based_dense_enum
{
    static constexpr bool value =
        ( is_dense_enum_v<E>     &&
          enum_first_of_v<E> == 0 );
};

template<typename E>
inline constexpr bool is_zero_based_dense_enum_v =
    is_zero_based_dense_enum<E>::value;


// =============================================================================
// VI.  Enum Map Strategy Enum
// =============================================================================

// DEnumMapStrategy
//   enum: classifies the optimal backing layout for an enum-keyed map.
enum class DEnumMapStrategy
{
    // key enum is dense with known cardinality. storage is a flat array
    // indexed by (static_cast<underlying>(key) - first). O(1) lookup, insert,
    // erase. Fixed memory.
    dense_array = 0,

    // key enum has known cardinality but is not dense, or the backing is
    // sorted + contiguous. binary-search over a sorted vector of pairs. O(log
    // n) lookup.
    sparse_sorted = 1,

    // backing provides hash infrastructure. O(1) amortized lookup.
    sparse_hashed = 2,

    // no acceleration available. O(n) linear scan.
    sparse_linear = 3
};


// =============================================================================
// VII. Strategy Deduction
// =============================================================================

// enum_map_strategy_for
//   trait: deduces the optimal DEnumMapStrategy given an enum type and a
// backing container type.
template<typename Enum,
         typename Backing>
struct enum_map_strategy_for
{
    using E = Enum;
    using B = clean_t<Backing>;

    static constexpr DEnumMapStrategy value =
        is_dense_enum_v<E>
            ? DEnumMapStrategy::dense_array

        : ( is_sorted_container_v<B> &&
            has_data_accessor_v<B> )
            ? DEnumMapStrategy::sparse_sorted

        : has_hasher_type_v<B>
            ? DEnumMapStrategy::sparse_hashed

        : DEnumMapStrategy::sparse_linear;
};

template<typename Enum,
         typename Backing>
inline constexpr DEnumMapStrategy
    enum_map_strategy_for_v =
        enum_map_strategy_for<Enum, Backing>::value;


// =============================================================================
// VIII. Tagged Struct Traits                (has_/is_)
// =============================================================================
// Traits that operate on a map container type to detect
// enum-map-specific properties.

// is_enum_keyed_map
//   trait: true if the container is map-structured and its key_type is an
// enum.
template<typename Map>
struct is_enum_keyed_map
{
    using C = clean_t<Map>;

    static constexpr bool value =
        ( is_map_structured_v<C> &&
          has_enum_key_v<C> );
};

template<typename Map>
inline constexpr bool is_enum_keyed_map_v =
    is_enum_keyed_map<Map>::value;

// has_known_key_cardinality
//   trait: true if the map's enum key has a known compile-time cardinality.
template<typename Map,
         typename = void>
struct has_known_key_cardinality : std::false_type
{};

// has_known_key_cardinality<Map, std::enable_if_t<
// is_enum_keyed_map_v<clean_t<Map>> && has_known_enum_count_v< typename
// clean_t<Map>
//   trait: the `std::enable_if_t< is_enum_keyed_map_v<clean_t<Map>> &&
// has_known_enum_count_v< typename clean_t<Map` case; it reports true.
template<typename Map>
struct has_known_key_cardinality<Map,
    std::enable_if_t<
        is_enum_keyed_map_v<clean_t<Map>> &&
        has_known_enum_count_v<
            typename clean_t<Map>::key_type>
    >> : std::true_type
{};

template<typename Map>
inline constexpr bool has_known_key_cardinality_v =
    has_known_key_cardinality<Map>::value;

// is_dense_enum_map
//   trait: true if the map's enum key is dense, enabling O(1) array-indexed
// storage.
template<typename Map,
         typename = void>
struct is_dense_enum_map : std::false_type
{};

// is_dense_enum_map<Map, std::enable_if_t< is_enum_keyed_map_v<clean_t<Map>>
// && is_dense_enum_v< typename clean_t<Map>
//   trait: the `std::enable_if_t< is_enum_keyed_map_v<clean_t<Map>> &&
// is_dense_enum_v< typename clean_t<Map` case; it reports true.
template<typename Map>
struct is_dense_enum_map<Map,
    std::enable_if_t<
        is_enum_keyed_map_v<clean_t<Map>> &&
        is_dense_enum_v<
            typename clean_t<Map>::key_type>
    >> : std::true_type
{};

template<typename Map>
inline constexpr bool is_dense_enum_map_v =
    is_dense_enum_map<Map>::value;

// key_cardinality_of
//   trait: resolves the key enum's cardinality for a map. Returns 0 if unknown
// or not enum-keyed.
NS_INTERNAL

    template<typename Map,
             bool = is_enum_keyed_map_v<clean_t<Map>>>
    struct key_cardinality_helper
    {
        static constexpr std::size_t value = 0;
    };

    template<typename Map>
    struct key_cardinality_helper<Map, true>
    {
        static constexpr std::size_t value =
            enum_count_of_v<
                typename clean_t<Map>::key_type>;
    };

NS_END  // internal

template<typename Map>
struct key_cardinality_of
{
    static constexpr std::size_t value =
        internal::key_cardinality_helper<Map>::value;
};

template<typename Map>
inline constexpr std::size_t key_cardinality_of_v =
    key_cardinality_of<Map>::value;

// is_completeness_checkable
//   trait: true if the map's size can be compared against its key enum's
// cardinality at runtime. Structural check: known cardinality + has size().
template<typename Map>
struct is_completeness_checkable
{
    using C = clean_t<Map>;

    static constexpr bool value =
        ( is_enum_keyed_map_v<C>          &&
          has_known_key_cardinality_v<C>  &&
          has_size_accessor_v<C> );
};

template<typename Map>
inline constexpr bool is_completeness_checkable_v =
    is_completeness_checkable<Map>::value;


// --- cardinality protocol source detection for maps --- These detect *which*
// mechanism provides the cardinality for a given map's key type. Safely yield
// false if the map is not enum-keyed.

NS_INTERNAL

    template<typename Map,
             bool = is_enum_keyed_map_v<clean_t<Map>>>
    struct cardinality_source
    {
        static constexpr bool via_enum_info      = false;
        static constexpr bool via_sentinel_count = false;
        static constexpr bool via_sentinel_range = false;
        static constexpr bool via_any            = false;
    };

    template<typename Map>
    struct cardinality_source<Map, true>
    {
        using K = typename clean_t<Map>::key_type;

        static constexpr bool via_enum_info      =
            has_enum_info_count_v<K>;
        static constexpr bool via_sentinel_count =
            ( has_enum_sentinel_count_v<K> ||
              has_enum_sentinel_count_alt_v<K> );
        static constexpr bool via_sentinel_range =
            has_enum_sentinel_range_v<K>;
        static constexpr bool via_any            =
            has_known_enum_count_v<K>;
    };

NS_END  // internal


// =============================================================================
// IX.  Tagless Constexpr Bool Traits        (C++17)
// =============================================================================

// -------------------------------------------------------------------------
// A.  enum type identity
// -------------------------------------------------------------------------

// enum_is_scoped
//   tagless trait: true if E is a scoped enum.
template<typename E>
constexpr bool enum_is_scoped =
    is_scoped_enum_v<E>;

// enum_is_dense
//   tagless trait: true if E is a dense enum.
template<typename E>
constexpr bool enum_is_dense =
    is_dense_enum_v<E>;

// enum_is_zero_based
//   tagless trait: true if E is a zero-based dense enum.
template<typename E>
constexpr bool enum_is_zero_based =
    is_zero_based_dense_enum_v<E>;

// -------------------------------------------------------------------------
// B.  enum-map container identity
// -------------------------------------------------------------------------

// enum_map_is_enum_keyed
//   tagless trait: true if the map has an enum key.
template<typename Map>
constexpr bool enum_map_is_enum_keyed =
    is_enum_keyed_map_v<Map>;

// enum_map_is_dense
//   tagless trait: true if the map could use dense storage.
template<typename Map>
constexpr bool enum_map_is_dense =
    is_dense_enum_map_v<Map>;

// enum_map_is_completeness_checkable
//   tagless trait: true if runtime completeness can be verified.
template<typename Map>
constexpr bool enum_map_is_completeness_checkable =
    is_completeness_checkable_v<Map>;

// -------------------------------------------------------------------------
// C.  enum-map capabilities
// -------------------------------------------------------------------------

// enum_map_can_dense_index
//   tagless trait: true if the map can use O(1) array indexing via the enum's
// underlying value.
template<typename Map,
         typename = void>
constexpr bool enum_map_can_dense_index = false;

template<typename Map>
constexpr bool enum_map_can_dense_index<Map,
    std::enable_if_t<
        is_dense_enum_map_v<clean_t<Map>>
    >> = true;

// enum_map_does_know_cardinality
//   tagless trait: true if the enum key's cardinality is known at compile
// time.
template<typename Map>
constexpr bool enum_map_does_know_cardinality =
    has_known_key_cardinality_v<Map>;


// =============================================================================
// X.   Combined Classification              (enum_map_class)
// =============================================================================

// enum_map_class
//   struct: complete compile-time classification of an enum-keyed map
// container type.
template<typename Map>
struct enum_map_class
{
    using C = clean_t<Map>;

    // --- structural ---
    static constexpr bool is_map_structured     =
        is_map_structured_v<C>;
    static constexpr bool is_enum_keyed         =
        is_enum_keyed_map_v<C>;
    static constexpr bool is_homogeneous        =
        has_homogeneous_values_v<C>;

    // --- enum key properties ---
    static constexpr bool key_is_scoped         =
        has_scoped_enum_key_v<C>;
    static constexpr bool key_has_known_count   =
        has_known_key_cardinality_v<C>;
    static constexpr std::size_t key_count      =
        key_cardinality_of_v<C>;
    static constexpr bool key_is_dense          =
        is_dense_enum_map_v<C>;

    // --- cardinality protocol source ---
    static constexpr bool via_enum_info         =
        internal::cardinality_source<C>::via_enum_info;
    static constexpr bool via_sentinel_count    =
        internal::cardinality_source<C>::via_sentinel_count;
    static constexpr bool via_sentinel_range    =
        internal::cardinality_source<C>::via_sentinel_range;

    // --- completeness ---
    static constexpr bool can_check_completeness =
        is_completeness_checkable_v<C>;

    // --- overlay ---
    static constexpr bool is_overlay            =
        is_map_overlay_v<C>;
    static constexpr bool is_backed             =
        is_underlying_container_v<C>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_MAP_ENUM_MAP_TRAITS_HPP
