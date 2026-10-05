/*******************************************************************************
* djinterp [core]                                                 map_traits.hpp
*
* Compile-time structural traits for map-like containers.
*   Provides three layers of detection for any type that exhibits map
* (key -> value) semantics, regardless of whether it is a djinterp
* overlay, an STL associative container, or a user-defined type:
*
*   Layer 1: Expression detectors (alias templates).
*     Detect specific methods and nested types by forming an expression
*     and testing well-formedness via void_t / is_detected.  These are
*     the atomic building blocks.
*
*   Layer 2: Tagged struct traits (has_/is_ convention).
*     Compound SFINAE structs built from Layer 1 detectors, producing
*     ::value and _v variable template aliases.  Available in C++11+.
*
*   Layer 3: Tagless constexpr bool traits (can_/does_/is_ convention).
*     First-class constexpr bool values via variable template partial
*     specialization over void_t.  No struct wrapper, no ::value, no _v.
*     Usable directly in if-constexpr, static_assert, and enable_if.
*     Requires C++17.
*
*   Additionally: map_class<T> aggregates all detection results into a
* single classification struct, following the same pattern as
* container_class<T>, container_binary_class<T>, etc.
*
* DETECTION AXES:
*   1. Map structure:  key_type + mapped_type + pair-valued elements.
*   2. Key uniqueness: distinguishes map (unique) from multimap.
*   3. Ordering:       sorted (tree-backed), hashed, or unordered.
*   4. Lookup:         find-by-key, count-by-key, lower/upper_bound,
*                      equal_range, operator[], at.
*   5. Mutation:       insert-by-key, insert_or_assign, try_emplace,
*                      erase-by-key, emplace.
*   6. Overlay:        is the map an overlay?  What strategy?
*   7. Enum key:       is the key type a scoped or unscoped enum?
*   8. Value homogeneity: are all mapped values the same type?
*                      (always true for template-parameterized maps;
*                       detectable for variant-based or type-erased maps.)
*
* NAMING CONVENTION:
*   Expression detectors:   map_<method>_expr_t
*   Tagged struct traits:    has_map_<capability> / is_map_<property>
*   Variable template _v:   has_map_<capability>_v / is_map_<property>_v
*   Tagless traits:          map_can_<action> / map_does_<category>
*                            map_is_<identity>
*
* DEPENDENCIES:
*   djinterp.hpp               -- namespace macros, clean_t
*   type_traits.hpp            -- detection idiom, conjunction, void_t
*   container_traits.hpp       -- has_key_type, has_mapped_type, etc.
*   map.hpp                    -- DMapOverlayStrategy, vocabulary types
*
*
* path:      /inc/djinterp/core/container/map/map_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.30
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Expression Detectors
      --------------------

II.   Tagged Struct Traits (has_/is_)
      -------------------------------

III.  Tagless Constexpr Bool Traits (C++17)
      -------------------------------------

IV.   Combined Classification (map_class)
      -----------------------------------
*/

#ifndef DJINTERP_CONTAINER_MAP_MAP_TRAITS_HPP
#define DJINTERP_CONTAINER_MAP_MAP_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // std
    #include <variant>
#endif


NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// =============================================================================
// I.   Expression Detectors
// =============================================================================
// Alias templates that form expressions against a type.  If the
// expression is well-formed, the alias resolves to the expression's
// type; otherwise substitution fails silently.  These are consumed
// by is_detected<> and by the tagless void_t partial specializations.
//
// All detectors use const& where the operation is non-mutating and
// & where mutation is required, matching the canonical map interface.

// --- nested type detectors ---

// map_key_type_expr_t
//   detector: key_type nested alias.
template<typename Type>
using map_key_type_expr_t = typename Type::key_type;

// map_mapped_type_expr_t
//   detector: mapped_type nested alias.
template<typename Type>
using map_mapped_type_expr_t = typename Type::mapped_type;

// map_key_compare_expr_t
//   detector: key_compare nested alias.
template<typename Type>
using map_key_compare_expr_t = typename Type::key_compare;

// map_value_compare_expr_t
//   detector: value_compare nested alias.
template<typename Type>
using map_value_compare_expr_t = typename Type::value_compare;

// map_hasher_expr_t
//   detector: hasher nested alias.
template<typename Type>
using map_hasher_expr_t = typename Type::hasher;

// map_overlay_strategy_expr_t
//   detector: overlay_strategy static member.
template<typename Type>
using map_overlay_strategy_expr_t =
    decltype(Type::overlay_strategy);

// map_backing_type_expr_t
//   detector: backing_container_type nested alias.
template<typename Type>
using map_backing_type_expr_t =
    typename Type::backing_container_type;

// --- lookup method detectors ---

// map_find_expr_t
//   detector: find(key_type const&) const method.
template<typename Type>
using map_find_expr_t =
    decltype(std::declval<const Type&>().find(
        std::declval<typename Type::key_type const&>()));

// map_count_expr_t
//   detector: count(key_type const&) const method.
template<typename Type>
using map_count_expr_t =
    decltype(std::declval<const Type&>().count(
        std::declval<typename Type::key_type const&>()));

// map_contains_expr_t
//   detector: contains(key_type const&) const method.
template<typename Type>
using map_contains_expr_t =
    decltype(std::declval<const Type&>().contains(
        std::declval<typename Type::key_type const&>()));

// map_lower_bound_expr_t
//   detector: lower_bound(key_type const&) const method.
template<typename Type>
using map_lower_bound_expr_t =
    decltype(std::declval<const Type&>().lower_bound(
        std::declval<typename Type::key_type const&>()));

// map_upper_bound_expr_t
//   detector: upper_bound(key_type const&) const method.
template<typename Type>
using map_upper_bound_expr_t =
    decltype(std::declval<const Type&>().upper_bound(
        std::declval<typename Type::key_type const&>()));

// map_equal_range_expr_t
//   detector: equal_range(key_type const&) const method.
template<typename Type>
using map_equal_range_expr_t =
    decltype(std::declval<const Type&>().equal_range(
        std::declval<typename Type::key_type const&>()));

// map_at_expr_t
//   detector: at(key_type const&) const method.
template<typename Type>
using map_at_expr_t =
    decltype(std::declval<const Type&>().at(
        std::declval<typename Type::key_type const&>()));

// map_subscript_expr_t
//   detector: operator[](key_type const&) method.
template<typename Type>
using map_subscript_expr_t =
    decltype(std::declval<Type&>()[
        std::declval<typename Type::key_type const&>()]);

// --- mutation method detectors ---

// map_insert_pair_expr_t
//   detector: insert(value_type) method.
template<typename Type>
using map_insert_pair_expr_t =
    decltype(std::declval<Type&>().insert(
        std::declval<typename Type::value_type>()));

// map_insert_or_assign_expr_t
//   detector: insert_or_assign(key, value) method.
template<typename Type>
using map_insert_or_assign_expr_t =
    decltype(std::declval<Type&>().insert_or_assign(
        std::declval<typename Type::key_type>(),
        std::declval<typename Type::mapped_type>()));

// map_try_emplace_expr_t
//   detector: try_emplace(key, args...) method.
template<typename Type>
using map_try_emplace_expr_t =
    decltype(std::declval<Type&>().try_emplace(
        std::declval<typename Type::key_type>()));

// map_erase_key_expr_t
//   detector: erase(key_type const&) method.
template<typename Type>
using map_erase_key_expr_t =
    decltype(std::declval<Type&>().erase(
        std::declval<typename Type::key_type const&>()));

// map_key_comp_expr_t
//   detector: key_comp() const method.
template<typename Type>
using map_key_comp_expr_t =
    decltype(std::declval<const Type&>().key_comp());

// map_value_comp_expr_t
//   detector: value_comp() const method.
template<typename Type>
using map_value_comp_expr_t =
    decltype(std::declval<const Type&>().value_comp());


// =============================================================================
// II.  Tagged Struct Traits                 (has_/is_ convention)
// =============================================================================
// Compound struct-based traits built from the expression detectors
// above.  Each produces ::value and a _v variable template alias.
// Available in C++11+.

// --- structural detection ---

// is_map_structured
//   trait: true if the type has both key_type and mapped_type, indicating a
// key-value associative structure.
template<typename Type>
struct is_map_structured
{
    using C = clean_t<Type>;

    static constexpr bool value =
        ( has_key_type_v<C> &&
          has_mapped_type_v<C> );
};

template<typename Type>
inline constexpr bool is_map_structured_v =
    is_map_structured<Type>::value;

// has_map_pair_element
//   trait: true if the type's value_type is a std::pair whose first_type
// corresponds to const key_type.
NS_INTERNAL

    template<typename Type,
             typename = void>
    struct map_pair_check : std::false_type
    {};

    template<typename Type>
    struct map_pair_check<Type,
        std::enable_if_t<
            is_map_structured_v<Type>                   &&
            has_value_type_v<clean_t<Type>>              &&
            std::is_same_v<
                typename clean_t<Type>::value_type,
                std::pair<
                    const typename clean_t<Type>::key_type,
                    typename clean_t<Type>::mapped_type>>
        >> : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_map_pair_element
{
    static constexpr bool value =
        internal::map_pair_check<Type>::value;
};

template<typename Type>
inline constexpr bool has_map_pair_element_v =
    has_map_pair_element<Type>::value;

// --- key uniqueness ---

// is_unique_key_map
//   trait: true if the map enforces key uniqueness. Detection: has key_type +
// mapped_type + count() returns at most 1, OR structurally: no "multi" prefix
// detection (we rely on count returning size_type where 0 or 1 is the contract
// for unique-key maps, vs arbitrary for multi). Heuristic: a map with both
// find() and count() that also exposes insert() returning a pair<iterator,
// bool> is unique-key. Multimaps return just an iterator from insert.
template<typename Type>
struct is_unique_key_map
{
    using C = clean_t<Type>;

    // detect insert returning pair<iter, bool>
    static constexpr bool value =
        ( is_map_structured_v<C> &&
          is_detected_v<map_find_expr_t, C> &&
          is_detected_v<map_insert_pair_expr_t, C> );
};

template<typename Type>
inline constexpr bool is_unique_key_map_v =
    is_unique_key_map<Type>::value;

// --- lookup capability ---

// has_map_find
//   trait: true if the type provides find-by-key.
template<typename Type>
struct has_map_find
{
    static constexpr bool value =
        is_detected_v<map_find_expr_t, clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_find_v =
    has_map_find<Type>::value;

// has_map_count
//   trait: true if the type provides count-by-key.
template<typename Type>
struct has_map_count
{
    static constexpr bool value =
        is_detected_v<map_count_expr_t, clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_count_v =
    has_map_count<Type>::value;

// has_map_contains
//   trait: true if the type provides contains().
template<typename Type>
struct has_map_contains
{
    static constexpr bool value =
        is_detected_v<map_contains_expr_t, clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_contains_v =
    has_map_contains<Type>::value;

// has_map_at
//   trait: true if the type provides at(key).
template<typename Type>
struct has_map_at
{
    static constexpr bool value =
        is_detected_v<map_at_expr_t, clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_at_v =
    has_map_at<Type>::value;

// has_map_subscript
//   trait: true if the type provides operator[](key).
template<typename Type>
struct has_map_subscript
{
    static constexpr bool value =
        is_detected_v<map_subscript_expr_t, clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_subscript_v =
    has_map_subscript<Type>::value;

// has_map_lower_bound
//   trait: true if the type provides lower_bound(key).
template<typename Type>
struct has_map_lower_bound
{
    static constexpr bool value =
        is_detected_v<map_lower_bound_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_lower_bound_v =
    has_map_lower_bound<Type>::value;

// has_map_upper_bound
//   trait: true if the type provides upper_bound(key).
template<typename Type>
struct has_map_upper_bound
{
    static constexpr bool value =
        is_detected_v<map_upper_bound_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_upper_bound_v =
    has_map_upper_bound<Type>::value;

// has_map_equal_range
//   trait: true if the type provides equal_range(key).
template<typename Type>
struct has_map_equal_range
{
    static constexpr bool value =
        is_detected_v<map_equal_range_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_equal_range_v =
    has_map_equal_range<Type>::value;

// --- mutation capability ---

// has_map_insert
//   trait: true if the type provides insert(value_type).
template<typename Type>
struct has_map_insert
{
    static constexpr bool value =
        is_detected_v<map_insert_pair_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_insert_v =
    has_map_insert<Type>::value;

// has_map_insert_or_assign
//   trait: true if the type provides insert_or_assign(key, value).
template<typename Type>
struct has_map_insert_or_assign
{
    static constexpr bool value =
        is_detected_v<map_insert_or_assign_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_insert_or_assign_v =
    has_map_insert_or_assign<Type>::value;

// has_map_try_emplace
//   trait: true if the type provides try_emplace(key, args...).
template<typename Type>
struct has_map_try_emplace
{
    static constexpr bool value =
        is_detected_v<map_try_emplace_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_try_emplace_v =
    has_map_try_emplace<Type>::value;

// has_map_erase_key
//   trait: true if the type provides erase(key).
template<typename Type>
struct has_map_erase_key
{
    static constexpr bool value =
        is_detected_v<map_erase_key_expr_t,
                      clean_t<Type>>;
};

template<typename Type>
inline constexpr bool has_map_erase_key_v =
    has_map_erase_key<Type>::value;

// --- ordering detection ---

// is_sorted_map
//   trait: true if the map maintains a sorted ordering (key_compare present,
// no hasher).
template<typename Type>
struct is_sorted_map
{
    using C = clean_t<Type>;

    static constexpr bool value =
        ( is_map_structured_v<C>    &&
          has_key_compare_v<C>      &&
          !has_hasher_type_v<C> );
};

template<typename Type>
inline constexpr bool is_sorted_map_v =
    is_sorted_map<Type>::value;

// is_hashed_map
//   trait: true if the map uses hash-based lookup (hasher present).
template<typename Type>
struct is_hashed_map
{
    using C = clean_t<Type>;

    static constexpr bool value =
        ( is_map_structured_v<C> &&
          has_hasher_type_v<C> );
};

template<typename Type>
inline constexpr bool is_hashed_map_v =
    is_hashed_map<Type>::value;

// --- overlay detection ---

// is_map_overlay
//   trait: true if the type is a map overlay (exposes overlay_strategy and
// backing_container_type).
template<typename Type>
struct is_map_overlay
{
    using C = clean_t<Type>;

    static constexpr bool value =
        ( is_map_structured_v<C>                      &&
          is_detected_v<map_overlay_strategy_expr_t, C> &&
          is_detected_v<map_backing_type_expr_t, C> );
};

template<typename Type>
inline constexpr bool is_map_overlay_v =
    is_map_overlay<Type>::value;

// --- enum key detection ---

// has_enum_key
//   trait: true if the map's key_type is an enum (scoped or unscoped).
template<typename Type,
         typename = void>
struct has_enum_key : std::false_type
{};

// has_enum_key<Type, std::enable_if_t< has_key_type_v<clean_t<Type>> &&
// std::is_enum_v< typename clean_t<Type>
//   trait: the `std::enable_if_t< has_key_type_v<clean_t<Type>> &&
// std::is_enum_v< typename clean_t<Type` case; it reports true.
template<typename Type>
struct has_enum_key<Type,
    std::enable_if_t<
        has_key_type_v<clean_t<Type>>  &&
        std::is_enum_v<
            typename clean_t<Type>::key_type>
    >> : std::true_type
{};

template<typename Type>
inline constexpr bool has_enum_key_v =
    has_enum_key<Type>::value;

// has_scoped_enum_key
//   trait: true if the map's key_type is a scoped enum (enum class).
NS_INTERNAL

    template<typename E,
             typename = void>
    struct is_scoped_enum_check : std::false_type
    {};

    template<typename E>
    struct is_scoped_enum_check<E,
        std::enable_if_t<
            std::is_enum_v<E> &&
            !std::is_convertible_v<E, std::underlying_type_t<E>>
        >> : std::true_type
    {};

NS_END  // internal

template<typename Type,
         typename = void>
struct has_scoped_enum_key : std::false_type
{};

// has_scoped_enum_key<Type, std::enable_if_t< has_key_type_v<clean_t<Type>>
// && internal::is_scoped_enum_check< typename clean_t<Type>
//   trait: the `std::enable_if_t< has_key_type_v<clean_t<Type>> &&
// internal::is_scoped_enum_check< typename clean_t<Type` case; it reports
// true.
template<typename Type>
struct has_scoped_enum_key<Type,
    std::enable_if_t<
        has_key_type_v<clean_t<Type>>  &&
        internal::is_scoped_enum_check<
            typename clean_t<Type>::key_type>::value
    >> : std::true_type
{};

template<typename Type>
inline constexpr bool has_scoped_enum_key_v =
    has_scoped_enum_key<Type>::value;

// --- value homogeneity ---

// has_homogeneous_values
//   trait: true if the map's mapped_type is a concrete (non-variant, non-any)
// type. A template-parameterized map always has homogeneous values. Detection:
// mapped_type exists AND is not std::any AND is not a std::variant.
NS_INTERNAL

    template<typename Type>
    struct is_variant_type : std::false_type
    {};

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    template<typename... Ts>
    struct is_variant_type<std::variant<Ts...>> : std::true_type
    {};
#endif

NS_END  // internal

template<typename Type,
         typename = void>
struct has_homogeneous_values : std::false_type
{};

// has_homogeneous_values<Type, std::enable_if_t<
// has_mapped_type_v<clean_t<Type>> && !internal::is_variant_type< typename
// clean_t<Type>
//   trait: the `std::enable_if_t< has_mapped_type_v<clean_t<Type>> &&
// !internal::is_variant_type< typename clean_t<Type` case; it reports true.
template<typename Type>
struct has_homogeneous_values<Type,
    std::enable_if_t<
        has_mapped_type_v<clean_t<Type>>    &&
        !internal::is_variant_type<
            typename clean_t<Type>::mapped_type>::value
    >> : std::true_type
{};

template<typename Type>
inline constexpr bool has_homogeneous_values_v =
    has_homogeneous_values<Type>::value;


// =============================================================================
// III. Tagless Constexpr Bool Traits        (C++17)
// =============================================================================
// First-class constexpr bool values.  No struct wrapper, no ::value.
// Naming: map_can_<action>, map_does_<category>, map_is_<identity>.

// -------------------------------------------------------------------------
// A.  structural identity
// -------------------------------------------------------------------------

// map_is_structured
//   tagless trait: true if the type has key_type + mapped_type.
template<typename Type,
         typename = void>
constexpr bool map_is_structured = false;

template<typename Type>
constexpr bool map_is_structured<Type,
    std::void_t<
        map_key_type_expr_t<Type>,
        map_mapped_type_expr_t<Type>>> = true;

// map_is_pair_valued
//   tagless trait: true if value_type is pair<const K, V>.
template<typename Type,
         typename = void>
constexpr bool map_is_pair_valued = false;

template<typename Type>
constexpr bool map_is_pair_valued<Type,
    std::enable_if_t<
        has_map_pair_element_v<Type>>> = true;

// map_is_overlay
//   tagless trait: true if the type is a map overlay.
template<typename Type,
         typename = void>
constexpr bool map_is_overlay = false;

template<typename Type>
constexpr bool map_is_overlay<Type,
    std::void_t<
        map_overlay_strategy_expr_t<Type>,
        map_backing_type_expr_t<Type>>> = true;

// map_is_sorted
//   tagless trait: true if the map has a sorted ordering.
template<typename Type>
constexpr bool map_is_sorted =
    is_sorted_map_v<Type>;

// map_is_hashed
//   tagless trait: true if the map has hash-based lookup.
template<typename Type>
constexpr bool map_is_hashed =
    is_hashed_map_v<Type>;

// map_has_enum_key
//   tagless trait: true if the key type is an enum.
template<typename Type>
constexpr bool map_has_enum_key =
    has_enum_key_v<Type>;

// map_has_homogeneous_values
//   tagless trait: true if the mapped type is homogeneous.
template<typename Type>
constexpr bool map_has_homogeneous_values =
    has_homogeneous_values_v<Type>;

// -------------------------------------------------------------------------
// B.  lookup capabilities
// -------------------------------------------------------------------------

// map_can_find
//   tagless trait: true if find(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_find = false;

template<typename Type>
constexpr bool map_can_find<Type,
    std::void_t<map_find_expr_t<Type>>> = true;

// map_can_count
//   tagless trait: true if count(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_count = false;

template<typename Type>
constexpr bool map_can_count<Type,
    std::void_t<map_count_expr_t<Type>>> = true;

// map_can_contains
//   tagless trait: true if contains(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_contains = false;

template<typename Type>
constexpr bool map_can_contains<Type,
    std::void_t<map_contains_expr_t<Type>>> = true;

// map_can_at
//   tagless trait: true if at(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_at = false;

template<typename Type>
constexpr bool map_can_at<Type,
    std::void_t<map_at_expr_t<Type>>> = true;

// map_can_subscript
//   tagless trait: true if operator[](key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_subscript = false;

template<typename Type>
constexpr bool map_can_subscript<Type,
    std::void_t<map_subscript_expr_t<Type>>> = true;

// map_can_lower_bound
//   tagless trait: true if lower_bound(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_lower_bound = false;

template<typename Type>
constexpr bool map_can_lower_bound<Type,
    std::void_t<map_lower_bound_expr_t<Type>>> = true;

// map_can_upper_bound
//   tagless trait: true if upper_bound(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_upper_bound = false;

template<typename Type>
constexpr bool map_can_upper_bound<Type,
    std::void_t<map_upper_bound_expr_t<Type>>> = true;

// map_can_equal_range
//   tagless trait: true if equal_range(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_equal_range = false;

template<typename Type>
constexpr bool map_can_equal_range<Type,
    std::void_t<map_equal_range_expr_t<Type>>> = true;

// -------------------------------------------------------------------------
// C.  mutation capabilities
// -------------------------------------------------------------------------

// map_can_insert
//   tagless trait: true if insert(value_type) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_insert = false;

template<typename Type>
constexpr bool map_can_insert<Type,
    std::void_t<map_insert_pair_expr_t<Type>>> = true;

// map_can_insert_or_assign
//   tagless trait: true if insert_or_assign(key, value) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_insert_or_assign = false;

template<typename Type>
constexpr bool map_can_insert_or_assign<Type,
    std::void_t<
        map_insert_or_assign_expr_t<Type>>> = true;

// map_can_try_emplace
//   tagless trait: true if try_emplace(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_try_emplace = false;

template<typename Type>
constexpr bool map_can_try_emplace<Type,
    std::void_t<map_try_emplace_expr_t<Type>>> = true;

// map_can_erase_key
//   tagless trait: true if erase(key) is available.
template<typename Type,
         typename = void>
constexpr bool map_can_erase_key = false;

template<typename Type>
constexpr bool map_can_erase_key<Type,
    std::void_t<map_erase_key_expr_t<Type>>> = true;

// -------------------------------------------------------------------------
// D.  compound capability tags
// -------------------------------------------------------------------------

// map_does_full_lookup
//   tagless trait: true if find + count + contains are all available.
template<typename Type>
constexpr bool map_does_full_lookup =
    ( map_can_find<Type>     &&
      map_can_count<Type>    &&
      map_can_contains<Type> );

// map_does_ordered_lookup
//   tagless trait: true if the map supports ordered-range queries (lower_bound
// + upper_bound + equal_range).
template<typename Type>
constexpr bool map_does_ordered_lookup =
    ( map_can_lower_bound<Type>  &&
      map_can_upper_bound<Type>  &&
      map_can_equal_range<Type> );

// map_does_full_mutation
//   tagless trait: true if insert + erase_key + insert_or_assign are all
// available.
template<typename Type>
constexpr bool map_does_full_mutation =
    ( map_can_insert<Type>            &&
      map_can_erase_key<Type>         &&
      map_can_insert_or_assign<Type> );


// =============================================================================
// IV.  Combined Classification              (map_class)
// =============================================================================

// map_class
//   struct: complete compile-time classification of a map-like container type.
template<typename Type>
struct map_class
{
    using C = clean_t<Type>;

    // --- structural ---
    static constexpr bool is_structured     =
        is_map_structured_v<C>;
    static constexpr bool is_pair_valued    =
        has_map_pair_element_v<C>;
    static constexpr bool is_unique_key     =
        is_unique_key_map_v<C>;
    static constexpr bool is_overlay        =
        is_map_overlay_v<C>;

    // --- key type ---
    static constexpr bool has_enum_key_type =
        has_enum_key_v<C>;
    static constexpr bool has_scoped_enum_key_type =
        has_scoped_enum_key_v<C>;

    // --- value homogeneity ---
    static constexpr bool is_homogeneous    =
        has_homogeneous_values_v<C>;

    // --- ordering ---
    static constexpr bool is_sorted         =
        is_sorted_map_v<C>;
    static constexpr bool is_hashed         =
        is_hashed_map_v<C>;

    // --- lookup ---
    static constexpr bool can_find          =
        has_map_find_v<C>;
    static constexpr bool can_count         =
        has_map_count_v<C>;
    static constexpr bool can_contains      =
        has_map_contains_v<C>;
    static constexpr bool can_at            =
        has_map_at_v<C>;
    static constexpr bool can_subscript     =
        has_map_subscript_v<C>;
    static constexpr bool can_lower_bound   =
        has_map_lower_bound_v<C>;
    static constexpr bool can_upper_bound   =
        has_map_upper_bound_v<C>;
    static constexpr bool can_equal_range   =
        has_map_equal_range_v<C>;

    // --- mutation ---
    static constexpr bool can_insert        =
        has_map_insert_v<C>;
    static constexpr bool can_insert_or_assign =
        has_map_insert_or_assign_v<C>;
    static constexpr bool can_try_emplace   =
        has_map_try_emplace_v<C>;
    static constexpr bool can_erase_key     =
        has_map_erase_key_v<C>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_MAP_MAP_TRAITS_HPP
