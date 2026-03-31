/******************************************************************************
* djinterp [container]                                                map.hpp
*
* Map overlay module for djinterp containers.
*   Defines the compile-time traits, strategy dispatch, vocabulary types,
* CRTP base, and iterator adapters needed to overlay map (key -> value)
* semantics onto any djinterp container that meets the backing
* requirements.
*
*   A map overlay is NOT a container.  It is a semantic layer that:
*     - enforces key uniqueness on insertion,
*     - provides key-based lookup, erasure, and access,
*     - delegates all storage and memory layout to an underlying
*       (backing) container.
*
*   The overlay adds no virtual functions, no RTTI, and no heap
* allocations of its own.  Strategy selection is fully constexpr;
* dead branches are eliminated by if-constexpr dispatch.
*
*   The backing container must store elements convertible to
* map_entry<Key, Value> (a std::pair<const Key, Value> alias) and
* must be iterable with at least forward iteration.  Additional
* capabilities (random-access, sorted invariant, native find) unlock
* more efficient overlay strategies.
*
*   This module is a dependency of bimap.hpp.  The entry types,
* backing requirement traits, and lookup infrastructure are designed
* for reuse by bidirectional map overlays.
*
* OVERLAY STRATEGIES:
*   identity        -- backing already provides full map semantics.
*   sorted_flat     -- backing is contiguous + random-access;
*                      binary-search maintenance (flat_map style).
*   sorted_node     -- backing is sorted + node-based (tree/skip-list);
*                      logarithmic lookup via native ordering.
*   hashed          -- backing provides bucket_count()/hash_function();
*                      O(1) amortized lookup via native hashing.
*   linear_scan     -- fallback: O(n) key search on every operation.
*
* ZERO-OVERHEAD GUARANTEE:
*   When the overlay strategy is `identity` -- i.e. the backing
* container natively satisfies every map invariant -- the overlay
* is a zero-cost abstraction.  is_zero_cost_map_overlay_v<M>
* provides a compile-time certificate of this property.
*
* NAMESPACE LAYOUT:
*   djinterp::container::map       -- vocabulary types, CRTP base,
*                                     iterators, factory functions.
*   djinterp::container::traits    -- backing requirement traits,
*                                     strategy deduction, classification
*                                     structs, C++20 concepts.
*
* DEPENDENCIES:
*   djinterp.hpp                       -- namespace macros, clean_t
*   type_traits.hpp                    -- D_TYPE_TRAIT_TRUE, conjunction
*   container_traits.hpp               -- container classification
*   container_compare_traits.hpp       -- element compatibility
*   container_conversion_traits.hpp    -- conversion tier
*
* TABLE OF CONTENTS
* =================
* I.      Namespace and Keywords
* II.     Vocabulary Types                  (djinterp::container::map)
* III.    Map Overlay Strategy Enum         (djinterp::container::traits)
* IV.     Backing Requirement Traits        (djinterp::container::traits)
* V.      Element Compatibility Traits      (djinterp::container::traits)
* VI.     Strategy Deduction                (djinterp::container::traits)
* VII.    Combined Classification           (djinterp::container::traits)
* VIII.   Zero-Overhead Validation          (djinterp::container::traits)
* IX.     C++20 Concepts                    (djinterp::container::traits)
* X.      Map Overlay CRTP Base             (djinterp::container::map)
* XI.     Key-Projected Iterators           (djinterp::container::map)
* XII.    Value-Projected Iterators         (djinterp::container::map)
* XIII.   Factory Functions                 (djinterp::container::map)
*
*
* path:      /inc/container/map.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_MAP_
#define DJINTERP_MAP_ 1

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "./meta/container_traits.hpp"
#include "./meta/container_compare_traits.hpp"
#include "./meta/container_conversion_traits.hpp"
#include "./meta/iterator_traits.hpp"


NS_DJINTERP
NS_CONTAINER


// =============================================================================
// I.   Namespace and Keywords
// =============================================================================

#ifndef D_KEYWORD_MAP
    #define D_KEYWORD_MAP   map
#endif

#ifndef NS_MAP
    #define NS_MAP          D_NAMESPACE(D_KEYWORD_MAP)
#endif


// =============================================================================
// II.  Vocabulary Types                     (djinterp::container::map)
// =============================================================================
// map_entry is the canonical element type stored by any map
// overlay.  It is a type alias for std::pair<const Key, Value>,
// matching the STL convention.  Defined here so that backing
// containers, overlay bases, and bimap can all reference the
// same element type without circular dependencies.

NS_MAP

// map_entry
//   type: canonical key-value pair stored in a map overlay.
template<typename _Key,
         typename _Value>
using map_entry = std::pair<const _Key, _Value>;

// map_result
//   struct: result of a lookup or insertion operation.
// Holds an iterator to the element and a boolean indicating
// whether insertion took place (true) or the key already
// existed (false).
template<typename _Iterator>
struct map_result
{
    _Iterator iterator;
    bool      inserted;
};

NS_END  // map


// =============================================================================
// III. Map Overlay Strategy Enum            (djinterp::container::traits)
// =============================================================================

NS_TRAITS

// DMapOverlayStrategy
//   enum: classifies how the overlay enforces map semantics
// over the backing container.  Strategy selection is fully
// constexpr.
enum class DMapOverlayStrategy
{
    // backing already provides full map semantics
    // (key_type, mapped_type, find-by-key, unique insert).
    // overlay is zero-cost: pure forwarding.
    identity = 0,

    // backing is contiguous + random-access + sorted.
    // binary-search lookup, shift-based insert/erase.
    // O(log n) lookup, O(n) insert/erase amortized.
    sorted_flat = 1,

    // backing is node-based + sorted (tree, skip-list).
    // logarithmic lookup via the backing's own ordering.
    // O(log n) lookup, O(log n) insert/erase.
    sorted_node = 2,

    // backing provides hash-based lookup natively.
    // O(1) amortized lookup, insert, erase.
    hashed = 3,

    // fallback: no acceleration structure.
    // O(n) key search on every operation.
    linear_scan = 4
};


// =============================================================================
// IV.  Backing Requirement Traits           (djinterp::container::traits)
// =============================================================================
// Detect whether a container type can serve as the backing
// store for a map overlay.  Requirements are tiered:
//   Minimum:   iterable + value_type exists
//   Writable:  minimum + (push_back or insert) + erase
//   Full:      writable + element compatible with map_entry

// --- element pair detection ---

// has_pair_element
//   trait: true if the container's value_type is a
// std::pair instantiation.
NS_INTERNAL

    template<typename _Type>
    struct is_pair_type : std::false_type
    {};

    template<typename _A,
             typename _B>
    struct is_pair_type<std::pair<_A, _B>> : std::true_type
    {};

NS_END  // internal

template<typename _Container,
         typename = void>
struct has_pair_element : std::false_type
{};

template<typename _Container>
struct has_pair_element<_Container,
    std::enable_if_t<
        has_value_type_v<clean_t<_Container>>  &&
        internal::is_pair_type<
            typename clean_t<
                _Container>::value_type>::value
    >> : std::true_type
{};

template<typename _Container>
inline constexpr bool has_pair_element_v =
    has_pair_element<_Container>::value;

// --- key/value extraction from pair elements ---

// pair_key_type_of
//   trait: extracts the key type (first) from a container
// whose value_type is a std::pair.  Yields void otherwise.
NS_INTERNAL

    template<typename _Container,
             typename = void>
    struct pair_key_type_of_helper
    {
        using type = void;
    };

    template<typename _Container>
    struct pair_key_type_of_helper<_Container,
        std::enable_if_t<
            has_pair_element_v<_Container>>>
    {
        using type = typename clean_t<
            _Container>::value_type::first_type;
    };

NS_END  // internal

template<typename _Container>
struct pair_key_type_of
{
    using type =
        typename internal::pair_key_type_of_helper<
            clean_t<_Container>>::type;
};

template<typename _Container>
using pair_key_type_of_t =
    typename pair_key_type_of<_Container>::type;

// pair_mapped_type_of
//   trait: extracts the mapped type (second) from a container
// whose value_type is a std::pair.  Yields void otherwise.
NS_INTERNAL

    template<typename _Container,
             typename = void>
    struct pair_mapped_type_of_helper
    {
        using type = void;
    };

    template<typename _Container>
    struct pair_mapped_type_of_helper<_Container,
        std::enable_if_t<
            has_pair_element_v<_Container>>>
    {
        using type = typename clean_t<
            _Container>::value_type::second_type;
    };

NS_END  // internal

template<typename _Container>
struct pair_mapped_type_of
{
    using type =
        typename internal::pair_mapped_type_of_helper<
            clean_t<_Container>>::type;
};

template<typename _Container>
using pair_mapped_type_of_t =
    typename pair_mapped_type_of<_Container>::type;


// --- backing requirement compounds ---

// is_map_backing_iterable
//   trait: true if the container is iterable and exposes a
// value_type.  Minimum bar for read-only map overlay.
template<typename _Container>
struct is_map_backing_iterable
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( is_iterable_container_v<C> &&
          has_value_type_v<C> );
};

template<typename _Container>
inline constexpr bool is_map_backing_iterable_v =
    is_map_backing_iterable<_Container>::value;

// is_map_backing_writable
//   trait: true if the container supports insertion and
// erasure in addition to iteration.  Required for a mutable
// map overlay.
template<typename _Container>
struct is_map_backing_writable
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( is_map_backing_iterable_v<C> &&
          ( has_push_back_v<C> ||
            has_insert_v<C> )          &&
          has_erase_v<C> );
};

template<typename _Container>
inline constexpr bool is_map_backing_writable_v =
    is_map_backing_writable<_Container>::value;

// is_map_backing_compatible
//   trait: true if the container's elements are pair-typed
// AND the container is writable.  Full compatibility for
// a mutable map overlay without element conversion.
template<typename _Container>
struct is_map_backing_compatible
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( is_map_backing_writable_v<C> &&
          has_pair_element_v<C> );
};

template<typename _Container>
inline constexpr bool is_map_backing_compatible_v =
    is_map_backing_compatible<_Container>::value;


// =============================================================================
// V.   Element Compatibility Traits         (djinterp::container::traits)
// =============================================================================
// Detect whether the backing container's pair element types
// are compatible with a specific Key and Value type.

// has_compatible_map_key
//   trait: true if the backing container's pair first_type
// is the same as or convertible to _Key.
template<typename _Container,
         typename _Key>
struct has_compatible_map_key
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( has_pair_element_v<C> &&
          ( std::is_same_v<
                pair_key_type_of_t<C>,
                const _Key>            ||
            std::is_same_v<
                std::remove_const_t<
                    pair_key_type_of_t<C>>,
                _Key>                  ||
            std::is_convertible_v<
                pair_key_type_of_t<C>,
                _Key> ) );
};

template<typename _Container,
         typename _Key>
inline constexpr bool has_compatible_map_key_v =
    has_compatible_map_key<_Container, _Key>::value;

// has_compatible_map_value
//   trait: true if the backing container's pair second_type
// is the same as or convertible to _Value.
template<typename _Container,
         typename _Value>
struct has_compatible_map_value
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( has_pair_element_v<C> &&
          ( std::is_same_v<
                pair_mapped_type_of_t<C>,
                _Value>                ||
            std::is_convertible_v<
                pair_mapped_type_of_t<C>,
                _Value> ) );
};

template<typename _Container,
         typename _Value>
inline constexpr bool has_compatible_map_value_v =
    has_compatible_map_value<_Container, _Value>::value;

// has_compatible_map_entry
//   trait: true if the backing container's pair element is
// compatible with map_entry<_Key, _Value>.
template<typename _Container,
         typename _Key,
         typename _Value>
struct has_compatible_map_entry
{
    static constexpr bool value =
        ( has_compatible_map_key_v<_Container, _Key>   &&
          has_compatible_map_value_v<_Container, _Value> );
};

template<typename _Container,
         typename _Key,
         typename _Value>
inline constexpr bool has_compatible_map_entry_v =
    has_compatible_map_entry<
        _Container, _Key, _Value>::value;


// =============================================================================
// VI.  Strategy Deduction                   (djinterp::container::traits)
// =============================================================================
// Given a backing container, deduce the best overlay strategy
// at compile time.  The deduction considers:
//   1. Does the backing already expose full map semantics?
//      (key_type + mapped_type + find-by-key)  -> identity
//   2. Does the backing have hash infrastructure?
//      (hasher + bucket_count + hash_function) -> hashed
//   3. Is the backing sorted + contiguous + random-access?
//      (key_compare + data() + random-access)  -> sorted_flat
//   4. Is the backing sorted + node-based?
//      (key_compare + no data())               -> sorted_node
//   5. Otherwise                               -> linear_scan

// --- native map detection ---

// has_native_map_find
//   trait: true if the container has a find() that accepts
// a key_type argument.
template<typename _Container,
         typename = void>
struct has_native_map_find : std::false_type
{};

template<typename _Container>
struct has_native_map_find<_Container,
    std::void_t<
        decltype(std::declval<const _Container&>().find(
            std::declval<
                typename _Container::key_type
                    const&>()))
    >> : std::true_type
{};

template<typename _Container>
inline constexpr bool has_native_map_find_v =
    has_native_map_find<_Container>::value;

// has_native_map_semantics
//   trait: true if the backing container already provides
// full associative map semantics (key_type, mapped_type,
// find-by-key, unique keys).
template<typename _Container>
struct has_native_map_semantics
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( has_key_type_v<C>          &&
          has_mapped_type_v<C>       &&
          has_native_map_find_v<C>   &&
          is_iterable_container_v<C> &&
          enforces_uniqueness_v<C> );
};

template<typename _Container>
inline constexpr bool has_native_map_semantics_v =
    has_native_map_semantics<_Container>::value;

// --- hash infrastructure detection ---

// has_map_hash_infrastructure
//   trait: true if the container provides hash-based lookup
// infrastructure sufficient for map overlay dispatch.
template<typename _Container>
struct has_map_hash_infrastructure
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( has_hasher_type_v<C> &&
          has_native_map_find_v<C> );
};

template<typename _Container>
inline constexpr bool has_map_hash_infrastructure_v =
    has_map_hash_infrastructure<_Container>::value;

// --- sorted + contiguous detection ---

// has_sorted_contiguous_layout
//   trait: true if the container is sorted, contiguous,
// and random-access -- the flat-map pattern.
template<typename _Container>
struct has_sorted_contiguous_layout
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( is_sorted_container_v<C>       &&
          has_data_accessor_v<C>         &&
          is_random_access_iterable_v<C> );
};

template<typename _Container>
inline constexpr bool has_sorted_contiguous_layout_v =
    has_sorted_contiguous_layout<_Container>::value;

// has_sorted_node_layout
//   trait: true if the container is sorted but not
// contiguous -- tree-like or skip-list-like.
template<typename _Container>
struct has_sorted_node_layout
{
    using C = clean_t<_Container>;

    static constexpr bool value =
        ( is_sorted_container_v<C>       &&
          !has_data_accessor_v<C>        &&
          is_iterable_container_v<C> );
};

template<typename _Container>
inline constexpr bool has_sorted_node_layout_v =
    has_sorted_node_layout<_Container>::value;


// --- strategy deduction ---

// map_overlay_strategy_for
//   trait: deduces the optimal DMapOverlayStrategy for a
// given backing container.
template<typename _Container>
struct map_overlay_strategy_for
{
    using C = clean_t<_Container>;

    static constexpr DMapOverlayStrategy value =
        has_native_map_semantics_v<C>
            ? DMapOverlayStrategy::identity

        : has_map_hash_infrastructure_v<C>
            ? DMapOverlayStrategy::hashed

        : has_sorted_contiguous_layout_v<C>
            ? DMapOverlayStrategy::sorted_flat

        : has_sorted_node_layout_v<C>
            ? DMapOverlayStrategy::sorted_node

        : DMapOverlayStrategy::linear_scan;
};

template<typename _Container>
inline constexpr DMapOverlayStrategy
    map_overlay_strategy_for_v =
        map_overlay_strategy_for<_Container>::value;


// =============================================================================
// VII. Combined Classification              (djinterp::container::traits)
// =============================================================================

// map_overlay_class
//   struct: complete compile-time classification of a
// map overlay over a specific backing container with
// given key and value types.
template<typename _Container,
         typename _Key,
         typename _Value>
struct map_overlay_class
{
    using C = clean_t<_Container>;

    // --- backing compatibility ---
    static constexpr bool is_iterable    =
        is_map_backing_iterable_v<C>;
    static constexpr bool is_writable    =
        is_map_backing_writable_v<C>;
    static constexpr bool is_compatible  =
        is_map_backing_compatible_v<C>;

    // --- element compatibility ---
    static constexpr bool has_pair       =
        has_pair_element_v<C>;
    static constexpr bool key_matches    =
        has_compatible_map_key_v<C, _Key>;
    static constexpr bool mapped_matches =
        has_compatible_map_value_v<C, _Value>;
    static constexpr bool entry_matches  =
        has_compatible_map_entry_v<C, _Key, _Value>;

    // --- native capability detection ---
    static constexpr bool has_native_find =
        has_native_map_find_v<C>;
    static constexpr bool has_native_map  =
        has_native_map_semantics_v<C>;
    static constexpr bool has_hash        =
        has_map_hash_infrastructure_v<C>;
    static constexpr bool is_sorted_flat  =
        has_sorted_contiguous_layout_v<C>;
    static constexpr bool is_sorted_node  =
        has_sorted_node_layout_v<C>;

    // --- overlay strategy ---
    static constexpr DMapOverlayStrategy strategy =
        map_overlay_strategy_for_v<C>;

    // --- zero-overhead check ---
    static constexpr bool is_zero_cost   =
        ( strategy == DMapOverlayStrategy::identity );

    // --- from underlying container_class ---
    static constexpr bool is_backed      =
        is_backed_container_v<C>;
    static constexpr bool is_fundamental =
        is_fundamental_container_v<C>;
};


// =============================================================================
// VIII. Zero-Overhead Validation            (djinterp::container::traits)
// =============================================================================

// is_zero_cost_map_overlay
//   trait: true if the map overlay adds no runtime cost
// over the backing container -- i.e. the strategy is
// identity.
template<typename _Container>
struct is_zero_cost_map_overlay
{
    static constexpr bool value =
        ( map_overlay_strategy_for_v<
              clean_t<_Container>> ==
          DMapOverlayStrategy::identity );
};

template<typename _Container>
inline constexpr bool is_zero_cost_map_overlay_v =
    is_zero_cost_map_overlay<_Container>::value;

// is_efficient_map_overlay
//   trait: true if the overlay strategy provides
// sub-linear lookup (identity, hashed, sorted_flat,
// or sorted_node).  False only for linear_scan.
template<typename _Container>
struct is_efficient_map_overlay
{
    static constexpr bool value =
        ( map_overlay_strategy_for_v<
              clean_t<_Container>> !=
          DMapOverlayStrategy::linear_scan );
};

template<typename _Container>
inline constexpr bool is_efficient_map_overlay_v =
    is_efficient_map_overlay<_Container>::value;


// =============================================================================
// IX.  C++20 Concepts                       (djinterp::container::traits)
// =============================================================================

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // map_backing
    //   concept: constrains _C to types that satisfy the full
    // map backing requirements for key type _K and value type _V.
    template<typename _C,
             typename _K,
             typename _V>
    concept map_backing =
        is_map_backing_compatible_v<_C> &&
        has_compatible_map_entry_v<_C, _K, _V>;

    // efficient_map_backing
    //   concept: constrains _C to types that provide sub-linear
    // lookup as a map backing.
    template<typename _C>
    concept efficient_map_backing =
        is_map_backing_compatible_v<_C> &&
        is_efficient_map_overlay_v<_C>;

    // zero_cost_map_backing
    //   concept: constrains _C to types where the map overlay
    // is a zero-cost abstraction.
    template<typename _C>
    concept zero_cost_map_backing =
        is_map_backing_compatible_v<_C> &&
        is_zero_cost_map_overlay_v<_C>;

#endif  // C++20

NS_END  // traits


// =============================================================================
// X.   Map Overlay CRTP Base                (djinterp::container::map)
// =============================================================================
// The overlay base provides the map-semantic API.  The
// _Derived class owns the backing container and exposes it
// via the CRTP hook backing().
//
// Template parameters:
//   _Derived    -- the concrete map type (CRTP).
//   _Key        -- the key type.
//   _Value      -- the mapped value type.
//   _Backing    -- the backing container type.
//   _Compare    -- the key comparison function object type.
//                   defaults to std::less<_Key>.
//
// The _Derived type must provide:
//   _Backing&       backing()       noexcept;
//   const _Backing& backing() const noexcept;

NS_MAP

template<typename _Derived,
         typename _Key,
         typename _Value,
         typename _Backing,
         typename _Compare = std::less<_Key>>
class map_overlay_base
{
private:
    using self_type    = map_overlay_base;
    using derived_type = _Derived;

    static constexpr traits::DMapOverlayStrategy
        m_strategy =
            traits::map_overlay_strategy_for_v<_Backing>;

    // --- CRTP access ---

    constexpr derived_type&
    derived() noexcept
    {
        return static_cast<derived_type&>(*this);
    }

    constexpr const derived_type&
    derived() const noexcept
    {
        return static_cast<const derived_type&>(
            *this);
    }

public:
    // --- public type aliases ---

    using key_type               = _Key;
    using mapped_type            = _Value;
    using value_type             =
        map_entry<_Key, _Value>;
    using key_compare            = _Compare;
    using backing_container_type = _Backing;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

    using iterator =
        decltype(std::declval<_Backing&>().begin());
    using const_iterator =
        decltype(std::declval<
            const _Backing&>().cbegin());

    using insert_result = map_result<iterator>;

    // --- overlay strategy introspection ---

    static constexpr traits::DMapOverlayStrategy
        overlay_strategy = m_strategy;

    // --- capacity ---

    constexpr size_type
    size() const noexcept
    {
        return derived().backing().size();
    }

    constexpr bool
    empty() const noexcept
    {
        return derived().backing().empty();
    }

    // --- iteration ---

    constexpr iterator
    begin() noexcept
    {
        return derived().backing().begin();
    }

    constexpr const_iterator
    begin() const noexcept
    {
        return derived().backing().cbegin();
    }

    constexpr iterator
    end() noexcept
    {
        return derived().backing().end();
    }

    constexpr const_iterator
    end() const noexcept
    {
        return derived().backing().cend();
    }

    constexpr const_iterator
    cbegin() const noexcept
    {
        return derived().backing().cbegin();
    }

    constexpr const_iterator
    cend() const noexcept
    {
        return derived().backing().cend();
    }

    // --- lookup ---

    // find
    //   returns an iterator to the entry with the given key,
    // or end() if not found.  Dispatch is compile-time via
    // overlay strategy.
    constexpr const_iterator
    find(const _Key& _key) const
    {
        return find_impl(_key);
    }

    constexpr iterator
    find(const _Key& _key)
    {
        return find_impl_mut(_key);
    }

    // contains
    //   returns true if an entry with the given key exists.
    constexpr bool
    contains(const _Key& _key) const
    {
        return (find(_key) != cend());
    }

    // count
    //   returns 0 or 1 (map enforces uniqueness).
    constexpr size_type
    count(const _Key& _key) const
    {
        return contains(_key) ? 1 : 0;
    }

    // at
    //   returns a reference to the mapped value for _key.
    // precondition: _key exists in the map.
    // undefined behavior if _key is not found.
    constexpr const _Value&
    at(const _Key& _key) const
    {
        auto it = find(_key);

        return it->second;
    }

    constexpr _Value&
    at(const _Key& _key)
    {
        auto it = find(_key);

        return it->second;
    }

    // --- insertion ---

    // insert
    //   inserts a key-value pair if the key does not already
    // exist.  Returns a map_result with an iterator and a
    // boolean indicating whether insertion occurred.
    constexpr insert_result
    insert(const value_type& _entry)
    {
        return insert_impl(_entry);
    }

    constexpr insert_result
    insert(value_type&& _entry)
    {
        return insert_impl(std::move(_entry));
    }

    // insert_or_assign
    //   inserts a key-value pair if the key does not exist,
    // or assigns the value if it does.
    template<typename _V>
    constexpr insert_result
    insert_or_assign(const _Key& _key,
                     _V&&        _value)
    {
        auto it = find_impl_mut(_key);

        // key exists: assign
        if (it != derived().backing().end())
        {
            it->second = std::forward<_V>(_value);

            return { it, false };
        }

        // key does not exist: insert
        return insert_impl(
            value_type(_key,
                       std::forward<_V>(_value)));
    }

    // --- erasure ---

    // erase
    //   erases the entry with the given key.  Returns the
    // number of entries erased (0 or 1).
    constexpr size_type
    erase(const _Key& _key)
    {
        auto it = find_impl_mut(_key);

        if (it == derived().backing().end())
        {
            return 0;
        }

        derived().backing().erase(it);

        return 1;
    }

    // erase (iterator)
    //   erases the entry at the given iterator position.
    constexpr iterator
    erase(const_iterator _pos)
    {
        return derived().backing().erase(_pos);
    }

    // clear
    //   erases all entries.
    constexpr void
    clear()
    {
        derived().backing().clear();

        return;
    }

    // --- comparison ---

    constexpr key_compare
    key_comp() const
    {
        return _Compare{};
    }

protected:
    map_overlay_base()  = default;
    ~map_overlay_base() = default;

    map_overlay_base(
        const map_overlay_base&)            = default;
    map_overlay_base& operator=(
        const map_overlay_base&)            = default;
    map_overlay_base(
        map_overlay_base&&)                 = default;
    map_overlay_base& operator=(
        map_overlay_base&&)                 = default;

private:

    // -----------------------------------------------------------------
    // lookup dispatch (private)
    // -----------------------------------------------------------------
    // Each strategy has its own find implementation.  The
    // if-constexpr branches ensure only the matching path
    // is compiled.

    constexpr const_iterator
    find_impl(const _Key& _key) const
    {
        // identity: delegate to the backing's native find
        if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::identity)
        {
            return derived().backing().find(_key);
        }

        // hashed: delegate to the backing's native find
        else if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::hashed)
        {
            return derived().backing().find(_key);
        }

        // sorted_flat or sorted_node: binary search
        else if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::sorted_flat
            ||
            m_strategy ==
                traits::DMapOverlayStrategy::sorted_node)
        {
            return sorted_find(
                derived().backing().cbegin(),
                derived().backing().cend(),
                _key);
        }

        // linear_scan: O(n) walk
        else
        {
            return linear_find(
                derived().backing().cbegin(),
                derived().backing().cend(),
                _key);
        }
    }

    constexpr iterator
    find_impl_mut(const _Key& _key)
    {
        // identity or hashed: delegate to native find
        if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::identity
            ||
            m_strategy ==
                traits::DMapOverlayStrategy::hashed)
        {
            return derived().backing().find(_key);
        }

        // sorted_flat or sorted_node: binary search
        else if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::sorted_flat
            ||
            m_strategy ==
                traits::DMapOverlayStrategy::sorted_node)
        {
            return sorted_find(
                derived().backing().begin(),
                derived().backing().end(),
                _key);
        }

        // linear_scan: O(n) walk
        else
        {
            return linear_find(
                derived().backing().begin(),
                derived().backing().end(),
                _key);
        }
    }

    // --- sorted find (lower_bound + key match) ---

    template<typename _Iter>
    static constexpr _Iter
    sorted_find(_Iter       _begin,
                _Iter       _end,
                const _Key& _key)
    {
        _Compare comp{};

        auto it = std::lower_bound(
            _begin, _end, _key,
            [&comp](const auto& _entry,
                    const _Key& _k)
            {
                return comp(_entry.first, _k);
            });

        // lower_bound found a position; check exact match
        if ( it != _end              &&
             !comp(_key, it->first) )
        {
            return it;
        }

        return _end;
    }

    // --- linear find ---

    template<typename _Iter>
    static constexpr _Iter
    linear_find(_Iter       _begin,
                _Iter       _end,
                const _Key& _key)
    {
        _Compare comp{};

        for (auto it = _begin; it != _end; ++it)
        {
            // equivalence: !comp(a,b) && !comp(b,a)
            if ( !comp(it->first, _key) &&
                 !comp(_key, it->first) )
            {
                return it;
            }
        }

        return _end;
    }


    // -----------------------------------------------------------------
    // insertion dispatch (private)
    // -----------------------------------------------------------------

    template<typename _Entry>
    constexpr insert_result
    insert_impl(_Entry&& _entry)
    {
        // identity or hashed: delegate to the backing
        if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::identity
            ||
            m_strategy ==
                traits::DMapOverlayStrategy::hashed)
        {
            auto result = derived().backing().insert(
                std::forward<_Entry>(_entry));

            return { result.first, result.second };
        }

        // sorted_flat or sorted_node: find insertion
        // point, check for duplicate, insert at position
        else if constexpr (
            m_strategy ==
                traits::DMapOverlayStrategy::sorted_flat
            ||
            m_strategy ==
                traits::DMapOverlayStrategy::sorted_node)
        {
            return sorted_insert(
                std::forward<_Entry>(_entry));
        }

        // linear_scan: check for duplicate, append
        else
        {
            return linear_insert(
                std::forward<_Entry>(_entry));
        }
    }

    template<typename _Entry>
    constexpr insert_result
    sorted_insert(_Entry&& _entry)
    {
        _Compare comp{};

        auto it = std::lower_bound(
            derived().backing().begin(),
            derived().backing().end(),
            _entry.first,
            [&comp](const auto& _existing,
                    const _Key& _k)
            {
                return comp(_existing.first, _k);
            });

        // duplicate check
        if ( it != derived().backing().end() &&
             !comp(_entry.first, it->first) )
        {
            return { it, false };
        }

        // insert at sorted position
        auto pos = derived().backing().insert(
            it, std::forward<_Entry>(_entry));

        return { pos, true };
    }

    template<typename _Entry>
    constexpr insert_result
    linear_insert(_Entry&& _entry)
    {
        // check for duplicate via linear scan
        auto it = linear_find(
            derived().backing().begin(),
            derived().backing().end(),
            _entry.first);

        if (it != derived().backing().end())
        {
            return { it, false };
        }

        // append to end
        if constexpr (
            traits::has_push_back_v<_Backing>)
        {
            derived().backing().push_back(
                std::forward<_Entry>(_entry));

            auto last = derived().backing().end();
            --last;

            return { last, true };
        }
        else
        {
            auto pos = derived().backing().insert(
                derived().backing().end(),
                std::forward<_Entry>(_entry));

            return { pos, true };
        }
    }
};


// =============================================================================
// XI.  Key-Projected Iterators              (djinterp::container::map)
// =============================================================================
// Wraps an iterator over map_entry elements and yields only
// the key (first) on dereference.

template<typename _Iterator>
class key_iterator
{
public:
    using base_traits      =
        std::iterator_traits<_Iterator>;
    using difference_type  =
        typename base_traits::difference_type;
    using value_type       =
        typename std::remove_const<
            typename base_traits::
                value_type::first_type
        >::type;
    using reference        = const value_type&;
    using pointer          = const value_type*;
    using iterator_category =
        typename base_traits::iterator_category;

    constexpr key_iterator() = default;

    constexpr explicit
    key_iterator(_Iterator _it) : m_it(_it)
    {}

    constexpr reference
    operator*() const
    {
        return m_it->first;
    }

    constexpr pointer
    operator->() const
    {
        return &(m_it->first);
    }

    constexpr key_iterator&
    operator++()
    {
        ++m_it;

        return *this;
    }

    constexpr key_iterator
    operator++(int)
    {
        auto tmp = *this;
        ++m_it;

        return tmp;
    }

    constexpr key_iterator&
    operator--()
    {
        --m_it;

        return *this;
    }

    constexpr key_iterator
    operator--(int)
    {
        auto tmp = *this;
        --m_it;

        return tmp;
    }

    constexpr _Iterator
    base() const
    {
        return m_it;
    }

    friend constexpr bool
    operator==(key_iterator _a,
               key_iterator _b)
    {
        return (_a.m_it == _b.m_it);
    }

    friend constexpr bool
    operator!=(key_iterator _a,
               key_iterator _b)
    {
        return (_a.m_it != _b.m_it);
    }

private:
    _Iterator m_it;
};


// =============================================================================
// XII. Value-Projected Iterators            (djinterp::container::map)
// =============================================================================
// Wraps an iterator over map_entry elements and yields only
// the mapped value (second) on dereference.

template<typename _Iterator>
class value_iterator
{
public:
    using base_traits      =
        std::iterator_traits<_Iterator>;
    using difference_type  =
        typename base_traits::difference_type;
    using value_type       =
        typename base_traits::
            value_type::second_type;
    using reference        = const value_type&;
    using pointer          = const value_type*;
    using iterator_category =
        typename base_traits::iterator_category;

    constexpr value_iterator() = default;

    constexpr explicit
    value_iterator(_Iterator _it) : m_it(_it)
    {}

    constexpr reference
    operator*() const
    {
        return m_it->second;
    }

    constexpr pointer
    operator->() const
    {
        return &(m_it->second);
    }

    constexpr value_iterator&
    operator++()
    {
        ++m_it;

        return *this;
    }

    constexpr value_iterator
    operator++(int)
    {
        auto tmp = *this;
        ++m_it;

        return tmp;
    }

    constexpr value_iterator&
    operator--()
    {
        --m_it;

        return *this;
    }

    constexpr value_iterator
    operator--(int)
    {
        auto tmp = *this;
        --m_it;

        return tmp;
    }

    constexpr _Iterator
    base() const
    {
        return m_it;
    }

    friend constexpr bool
    operator==(value_iterator _a,
               value_iterator _b)
    {
        return (_a.m_it == _b.m_it);
    }

    friend constexpr bool
    operator!=(value_iterator _a,
               value_iterator _b)
    {
        return (_a.m_it != _b.m_it);
    }

private:
    _Iterator m_it;
};


// =============================================================================
// XIII. Factory Functions                   (djinterp::container::map)
// =============================================================================

// make_key_iterator
template<typename _Iterator>
constexpr key_iterator<_Iterator>
make_key_iterator(_Iterator _it)
{
    return key_iterator<_Iterator>(_it);
}

// make_value_iterator
template<typename _Iterator>
constexpr value_iterator<_Iterator>
make_value_iterator(_Iterator _it)
{
    return value_iterator<_Iterator>(_it);
}


NS_END  // map
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_MAP_
