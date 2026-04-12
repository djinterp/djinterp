/******************************************************************************
* djinterp [container]                                          set_traits.hpp
*
* djinterp set container traits header:
*   This header provides tagless, structural SFINAE-based trait detection for
* set-like containers. A type is classified as "set-like" when it exposes
* key_type and value_type but NOT mapped_type — i.e. it is an associative
* container whose elements ARE the keys.
*
*   Detection covers four standard variants plus the C++23 flat adaptors:
*   - ordered unique set       (std::set, flat_set)
*   - ordered multi set        (std::multiset, flat_multiset)
*   - unordered unique set     (std::unordered_set)
*   - unordered multi set      (std::unordered_multiset)
*   - any structural equivalent exposing the same interface
*
*   All detection uses clean_t<T> (cv-ref stripped) and produces
* static constexpr bool with _v suffixes.
*
*
* path:      /inc/djinterp/container/set_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_SET_TRAITS_
#define DJINTERP_SET_TRAITS_ 1

#include ".\djinterp.hpp"
#include ".\meta\type_traits.hpp"
#include ".\container\traits\container_traits.hpp"


///////////////////////////////////////////////////////////////////////////////
///              I.   SET-LIKE STRUCTURAL DETECTION                         ///
///////////////////////////////////////////////////////////////////////////////

NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// =========================================================================
//  1.  Core set-like detection
// =========================================================================
// A type is set-like when it has key_type and value_type, is iterable,
// but does NOT expose mapped_type (which would make it map-like).

// is_set_like
//   trait: true when T is a set-like associative container — has key_type,
// has value_type, iterable (begin/end), and no mapped_type.
template<typename _Type,
         typename = void>
struct is_set_like : std::false_type
{};

template<typename _Type>
struct is_set_like<_Type, D_VOID_T<
    typename clean_t<_Type>::key_type,
    typename clean_t<_Type>::value_type,
    decltype(std::declval<const clean_t<_Type>&>().begin()),
    decltype(std::declval<const clean_t<_Type>&>().end())
>> : D_NEGATION<has_mapped_type<clean_t<_Type>>>
{};

// is_set_like_v
//   variable template: value of is_set_like<_Type>.
template<typename _Type>
inline constexpr bool is_set_like_v = is_set_like<_Type>::value;


// =========================================================================
//  2.  Ordered / unordered classification
// =========================================================================

// is_ordered_set
//   trait: true when T is set-like with key_compare and no hasher.
// This corresponds to std::set, std::multiset, flat_set, flat_multiset,
// or any structural equivalent maintaining sorted order.
template<typename _Type,
         typename = void>
struct is_ordered_set : std::false_type
{};

template<typename _Type>
struct is_ordered_set<_Type, D_VOID_T<
    typename clean_t<_Type>::key_compare
>> : D_CONJUNCTION<
    is_set_like<_Type>,
    D_NEGATION<has_hasher_type<clean_t<_Type>>>
>
{
};

// is_ordered_set_v
//   variable template: value of is_ordered_set<_Type>.
template<typename _Type>
inline constexpr bool is_ordered_set_v = is_ordered_set<_Type>::value;


// is_unordered_set
//   trait: true when T is set-like with a hasher alias.
// This corresponds to std::unordered_set, std::unordered_multiset,
// or any structural equivalent using hash-based lookup.
template<typename _Type,
         typename = void>
struct is_unordered_set : std::false_type
{};

template<typename _Type>
struct is_unordered_set<_Type, D_VOID_T<
    typename clean_t<_Type>::hasher
>> : is_set_like<_Type>
{};

// is_unordered_set_v
//   variable template: value of is_unordered_set<_Type>.
template<typename _Type>
inline constexpr bool is_unordered_set_v = is_unordered_set<_Type>::value;


// =========================================================================
//  3.  Uniqueness / multiplicity classification
// =========================================================================

// is_unique_set
//   trait: true when T is set-like and enforces unique keys.
// Detection: set-like + key_type present + no mapped_type implies the
// container_class uniqueness axis (key_type && !mapped_type).
// For disambiguation from multisets, we additionally probe for count()
// returning an integral type bounded to {0,1} — but structurally the
// primary signal is the absence of equal_range returning more than one
// element, which cannot be detected at compile time. The framework's
// container_class::enforces_uniqueness already handles this via the
// key_type + !mapped_type rule from container_traits.hpp.
template<typename _Type,
         typename = void>
struct is_unique_set : std::false_type
{};

template<typename _Type>
struct is_unique_set<_Type, std::enable_if_t<
    is_set_like_v<_Type>
>> : std::bool_constant<container_class<clean_t<_Type>>::enforces_uniqueness>
{};

// is_unique_set_v
//   variable template: value of is_unique_set<_Type>.
template<typename _Type>
inline constexpr bool is_unique_set_v = is_unique_set<_Type>::value;


// is_multi_set
//   trait: true when T is set-like and allows duplicate keys.
template<typename _Type,
         typename = void>
struct is_multi_set : std::false_type
{};

template<typename _Type>
struct is_multi_set<_Type, std::enable_if_t<
    is_set_like_v<_Type>
>> : std::bool_constant<
    container_class<clean_t<_Type>>::allows_duplicates
>
{};

// is_multi_set_v
//   variable template: value of is_multi_set<_Type>.
template<typename _Type>
inline constexpr bool is_multi_set_v = is_multi_set<_Type>::value;


// =========================================================================
//  4.  Compound classifications
// =========================================================================

// is_ordered_unique_set
//   trait: true for std::set-like containers (sorted + unique).
template<typename _Type>
struct is_ordered_unique_set : D_CONJUNCTION<
    is_ordered_set<_Type>,
    is_unique_set<_Type>
>
{};

// is_ordered_unique_set_v
//   variable template: value of is_ordered_unique_set<_Type>.
template<typename _Type>
inline constexpr bool is_ordered_unique_set_v =
    is_ordered_unique_set<_Type>::value;


// is_ordered_multi_set
//   trait: true for std::multiset-like containers (sorted + duplicates).
template<typename _Type>
struct is_ordered_multi_set : D_CONJUNCTION<
    is_ordered_set<_Type>,
    is_multi_set<_Type>
>
{};

// is_ordered_multi_set_v
//   variable template: value of is_ordered_multi_set<_Type>.
template<typename _Type>
inline constexpr bool is_ordered_multi_set_v =
    is_ordered_multi_set<_Type>::value;


// is_unordered_unique_set
//   trait: true for std::unordered_set-like containers (hashed + unique).
template<typename _Type>
struct is_unordered_unique_set : D_CONJUNCTION<
    is_unordered_set<_Type>,
    is_unique_set<_Type>
>
{};

// is_unordered_unique_set_v
//   variable template: value of is_unordered_unique_set<_Type>.
template<typename _Type>
inline constexpr bool is_unordered_unique_set_v =
    is_unordered_unique_set<_Type>::value;


// is_unordered_multi_set
//   trait: true for std::unordered_multiset-like containers
// (hashed + duplicates).
template<typename _Type>
struct is_unordered_multi_set : D_CONJUNCTION<
    is_unordered_set<_Type>,
    is_multi_set<_Type>
>
{};

// is_unordered_multi_set_v
//   variable template: value of is_unordered_multi_set<_Type>.
template<typename _Type>
inline constexpr bool is_unordered_multi_set_v =
    is_unordered_multi_set<_Type>::value;


// =========================================================================
//  5.  Flat set detection (C++23 adaptors)
// =========================================================================

// is_flat_set
//   trait: true when T is a set-like container backed by another container
// (the flat_set / flat_multiset pattern). Detected via the presence of
// backing_container_type combined with set-like structure.
template<typename _Type,
         typename = void>
struct is_flat_set : std::false_type
{};

template<typename _Type>
struct is_flat_set<_Type, D_VOID_T<
    typename clean_t<_Type>::backing_container_type
>> : is_ordered_set<_Type>
{};

// is_flat_set_v
//   variable template: value of is_flat_set<_Type>.
template<typename _Type>
inline constexpr bool is_flat_set_v = is_flat_set<_Type>::value;


// is_flat_unique_set
//   trait: true for flat_set-like (backed + sorted + unique).
template<typename _Type>
struct is_flat_unique_set : D_CONJUNCTION<
    is_flat_set<_Type>,
    is_unique_set<_Type>
>
{};

// is_flat_unique_set_v
//   variable template: value of is_flat_unique_set<_Type>.
template<typename _Type>
inline constexpr bool is_flat_unique_set_v =
    is_flat_unique_set<_Type>::value;


// is_flat_multi_set
//   trait: true for flat_multiset-like (backed + sorted + duplicates).
template<typename _Type>
struct is_flat_multi_set : D_CONJUNCTION<
    is_flat_set<_Type>,
    is_multi_set<_Type>
>
{};

// is_flat_multi_set_v
//   variable template: value of is_flat_multi_set<_Type>.
template<typename _Type>
inline constexpr bool is_flat_multi_set_v =
    is_flat_multi_set<_Type>::value;


// =========================================================================
//  6.  Set-specific method detection
// =========================================================================

// has_set_find
//   trait: true when T has a find(key_type) member returning an iterator.
D_TYPE_TRAIT_TRUE(has_set_find,
    decltype(std::declval<const clean_t<_Type>&>().find(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_count
//   trait: true when T has a count(key_type) member returning a size type.
D_TYPE_TRAIT_TRUE(has_set_count,
    decltype(std::declval<const clean_t<_Type>&>().count(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_contains
//   trait: true when T has a contains(key_type) member (C++20).
D_TYPE_TRAIT_TRUE(has_set_contains,
    decltype(std::declval<const clean_t<_Type>&>().contains(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_equal_range
//   trait: true when T has an equal_range(key_type) member.
D_TYPE_TRAIT_TRUE(has_set_equal_range,
    decltype(std::declval<const clean_t<_Type>&>().equal_range(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_lower_bound
//   trait: true when T has a lower_bound(key_type) member.
D_TYPE_TRAIT_TRUE(has_set_lower_bound,
    decltype(std::declval<const clean_t<_Type>&>().lower_bound(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_upper_bound
//   trait: true when T has an upper_bound(key_type) member.
D_TYPE_TRAIT_TRUE(has_set_upper_bound,
    decltype(std::declval<const clean_t<_Type>&>().upper_bound(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_insert
//   trait: true when T has an insert(value_type) member.
D_TYPE_TRAIT_TRUE(has_set_insert,
    decltype(std::declval<clean_t<_Type>&>().insert(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_set_emplace
//   trait: true when T has an emplace() member.
D_TYPE_TRAIT_TRUE(has_set_emplace,
    decltype(std::declval<clean_t<_Type>&>().emplace(
        std::declval<typename clean_t<_Type>::value_type>())))

// has_set_erase_key
//   trait: true when T has an erase(key_type) member.
D_TYPE_TRAIT_TRUE(has_set_erase_key,
    decltype(std::declval<clean_t<_Type>&>().erase(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_extract
//   trait: true when T has an extract(key_type) member (C++17).
D_TYPE_TRAIT_TRUE(has_set_extract,
    decltype(std::declval<clean_t<_Type>&>().extract(
        std::declval<typename clean_t<_Type>::key_type>())))

// has_set_merge
//   trait: true when T has a merge() member (C++17).
D_TYPE_TRAIT_TRUE(has_set_merge,
    decltype(std::declval<clean_t<_Type>&>().merge(
        std::declval<clean_t<_Type>&>())))


// =========================================================================
//  7.  Strategy enum
// =========================================================================

// set_lookup_strategy
//   enum: dispatch strategy for set element lookup, selected at compile
// time by set_class. Priority is top-to-bottom (first match wins).
enum class set_lookup_strategy
{
    // contains
    //   strategy: use contains() for membership test (C++20).
    contains,

    // find
    //   strategy: use find() != end() for membership test.
    find,

    // count
    //   strategy: use count() > 0 for membership test.
    count,

    // linear
    //   strategy: linear scan via iterators (fallback).
    linear,

    // unsupported
    //   strategy: no lookup capability detected.
    unsupported
};


// set_insert_strategy
//   enum: dispatch strategy for set element insertion.
enum class set_insert_strategy
{
    // emplace
    //   strategy: use emplace() for in-place construction.
    emplace,

    // insert
    //   strategy: use insert(value_type).
    insert,

    // unsupported
    //   strategy: no insertion capability detected.
    unsupported
};


// set_erase_strategy
//   enum: dispatch strategy for set element removal.
enum class set_erase_strategy
{
    // erase_key
    //   strategy: use erase(key_type) for key-based removal.
    erase_key,

    // erase_iterator
    //   strategy: use erase(iterator) after find.
    erase_iterator,

    // unsupported
    //   strategy: no erasure capability detected.
    unsupported
};


// =========================================================================
//  8.  Classification struct
// =========================================================================

// set_class
//   struct: aggregates all set-specific trait detections into a single
// compile-time classification. Query this instead of individual _v traits
// when the full picture is needed.
template<typename _Type>
struct set_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // --- core classification ---
    D_STATIC_CONSTEXPR bool is_set             = is_set_like_v<_Type>;
    D_STATIC_CONSTEXPR bool is_ordered         = is_ordered_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_unordered       = is_unordered_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_unique          = is_unique_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_multi           = is_multi_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_flat            = is_flat_set_v<_Type>;

    // --- compound classifications ---
    D_STATIC_CONSTEXPR bool is_ordered_unique  = is_ordered_unique_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_ordered_multi   = is_ordered_multi_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_unordered_unique = is_unordered_unique_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_unordered_multi = is_unordered_multi_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_flat_unique     = is_flat_unique_set_v<_Type>;
    D_STATIC_CONSTEXPR bool is_flat_multi      = is_flat_multi_set_v<_Type>;

    // --- method availability ---
    D_STATIC_CONSTEXPR bool has_find           = has_set_find_v<_Type>;
    D_STATIC_CONSTEXPR bool has_count          = has_set_count_v<_Type>;
    D_STATIC_CONSTEXPR bool has_contains       = has_set_contains_v<_Type>;
    D_STATIC_CONSTEXPR bool has_equal_range    = has_set_equal_range_v<_Type>;
    D_STATIC_CONSTEXPR bool has_lower_bound    = has_set_lower_bound_v<_Type>;
    D_STATIC_CONSTEXPR bool has_upper_bound    = has_set_upper_bound_v<_Type>;
    D_STATIC_CONSTEXPR bool has_insert         = has_set_insert_v<_Type>;
    D_STATIC_CONSTEXPR bool has_emplace        = has_set_emplace_v<_Type>;
    D_STATIC_CONSTEXPR bool has_erase_key      = has_set_erase_key_v<_Type>;
    D_STATIC_CONSTEXPR bool has_extract        = has_set_extract_v<_Type>;
    D_STATIC_CONSTEXPR bool has_merge          = has_set_merge_v<_Type>;

    // --- range lookup support ---
    D_STATIC_CONSTEXPR bool has_range_lookup   =
        ( has_lower_bound &&
          has_upper_bound );

    D_STATIC_CONSTEXPR bool has_full_lookup    =
        ( has_find        &&
          has_count       &&
          has_equal_range );

    // --- strategies ---
    D_STATIC_CONSTEXPR set_lookup_strategy lookup_strategy =
        ( has_contains ? set_lookup_strategy::contains  :
          has_find     ? set_lookup_strategy::find       :
          has_count    ? set_lookup_strategy::count      :
          is_set       ? set_lookup_strategy::linear     :
                         set_lookup_strategy::unsupported );

    D_STATIC_CONSTEXPR set_insert_strategy insert_strategy =
        ( has_emplace ? set_insert_strategy::emplace     :
          has_insert  ? set_insert_strategy::insert      :
                        set_insert_strategy::unsupported );

    D_STATIC_CONSTEXPR set_erase_strategy erase_strategy =
        ( has_erase_key                                    ?
              set_erase_strategy::erase_key                 :
          has_set_find_v<_Type> && has_erase_v<clean_type> ?
              set_erase_strategy::erase_iterator            :
              set_erase_strategy::unsupported );

    // --- aggregate ---
    D_STATIC_CONSTEXPR bool is_readable =
        ( lookup_strategy != set_lookup_strategy::unsupported );

    D_STATIC_CONSTEXPR bool is_writable =
        ( insert_strategy != set_insert_strategy::unsupported );

    D_STATIC_CONSTEXPR bool is_erasable =
        ( erase_strategy != set_erase_strategy::unsupported );

    D_STATIC_CONSTEXPR bool is_fully_mutable =
        ( is_writable &&
          is_erasable );
};


// =========================================================================
//  9.  Type extractors
// =========================================================================

// set_key_type_of_t
//   alias template: extracts T::key_type if present, void otherwise.
template<typename _Type,
         typename = void>
struct set_key_type_of
{
    using type = void;
};

template<typename _Type>
struct set_key_type_of<_Type, D_VOID_T<typename clean_t<_Type>::key_type>>
{
    using type = typename clean_t<_Type>::key_type;
};

// set_key_type_of_t
//   alias template: shorthand for set_key_type_of<_Type>::type.
template<typename _Type>
using set_key_type_of_t = typename set_key_type_of<_Type>::type;

// set_key_compare_of_t
//   alias template: extracts T::key_compare if present, void otherwise.
template<typename _Type,
         typename = void>
struct set_key_compare_of
{
    using type = void;
};

template<typename _Type>
struct set_key_compare_of<_Type, D_VOID_T<typename clean_t<_Type>::key_compare>>
{
    using type = typename clean_t<_Type>::key_compare;
};

// set_key_compare_of_t
//   alias template: shorthand for set_key_compare_of<_Type>::type.
template<typename _Type>
using set_key_compare_of_t = typename set_key_compare_of<_Type>::type;


// set_hasher_of_t
//   alias template: extracts T::hasher if present, void otherwise.
template<typename _Type,
         typename = void>
struct set_hasher_of
{
    using type = void;
};

template<typename _Type>
struct set_hasher_of<_Type, D_VOID_T<typename clean_t<_Type>::hasher>>
{
    using type = typename clean_t<_Type>::hasher;
};

// set_hasher_of_t
//   alias template: shorthand for set_hasher_of<_Type>::type.
template<typename _Type>
using set_hasher_of_t = typename set_hasher_of<_Type>::type;


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_SET_TRAITS_
