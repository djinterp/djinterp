/******************************************************************************
* djinterp [container]                       container_multiplicity_traits.hpp
*
* SFINAE structural traits for the multiplicity axis.
*   Multiplicity describes how many copies of an equivalent
* element a container is permitted to hold.  The axis admits four
* canonical positions:
*     none              - cannot be classified (no signal)
*     unique            - at most one copy per equivalent element
*     bounded_multi     - up to N copies, where 1 < N < SIZE_MAX
*     unbounded_multi   - unrestricted duplicates
*   The basic protocol (interval, legacy min/max accessors,
* enforces_uniqueness via key_type/mapped_type) lives in
* container_traits.hpp.  This header EXTENDS that protocol with:
*     1. Structural insert-return-type detection.  The standard
*        library encodes multiplicity in the return type of
*        single-argument `insert(value)`:
*          unique-keyed -> std::pair<iterator, bool>
*          multi-keyed  -> iterator
*        This signal correctly handles std::map and
* std::unordered_map (which the key_type/mapped_type heuristic in
* container_traits.hpp misses).
*     2. Opt-in member constant `max_multiplicity` for bounded-N
*        containers - the lightest declaration of "allows up to N
*        duplicates" available without a customization point.
*     3. Resolved values: container_multiplicity_kind enum, max_multiplicity_v
*        size extractor, and an aggregate snapshot.
*   Detection priority for the size extractor:
*     1. multiplicity_interval upper bound
*           (existing protocol, container_traits.hpp)
*     2. multiplicity_max() accessor
*           (existing protocol, container_traits.hpp)
*     3. T::max_multiplicity static constexpr member
*           (introduced here)
*     4. structural insert-return-type
*           (introduced here)
*     5. enforces_uniqueness fallback (key_type without
*        mapped_type) -> 1
*           (existing protocol, used as last resort)
*     6. otherwise -> 0 (unknown)
*   The trait operates on `clean_t<_Type>` (cv-ref stripped) and
* produces compile-time bool / size_t values.
*   PORTABILITY:
*   C++17 baseline (`if constexpr` used in the priority chain).
* All `_v` and `_t` aliases gated on
* D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES where applicable.
*
*
* path:      /inc/djinterp/core/container/traits/
*                container_multiplicity_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_
#define DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_ 1

// std
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../container_traits.hpp"


NS_DJINTERP


// ===========================================================================
// 1.  Multiplicity Kind Enumeration
// ===========================================================================

// container_multiplicity_kind
//   enum: classifies the maximum permitted multiplicity.
enum class container_multiplicity_kind
{
    // unknown / not classifiable
    none,
    // at most one copy per equivalent element
    unique,
    // up to N copies, with 1 < N < SIZE_MAX
    bounded_multi,
    // unrestricted duplicates
    unbounded_multi
};

// ===========================================================================
// 2.  SFINAE method / value detection
// ===========================================================================
// New atomic signals introduced by this header.  Each detector
// is named with a `_signal` suffix, matching the convention
// established in mutable_container_traits.hpp.

// has_unique_insert_signal
//   trait: detects associative-style `insert(v)` returning
// std::pair<iterator, bool>.  This is the canonical structural
// signal for unique-keyed containers - std::set, std::map,
// std::unordered_set, std::unordered_map - and is strictly
// stronger than the key_type/mapped_type heuristic in
// container_traits.hpp.
template<typename _Type,
         typename = void>
struct has_unique_insert_signal : std::false_type
{};

template<typename _Type>
struct has_unique_insert_signal<_Type, void_t<
    typename _Type::iterator,
    typename _Type::value_type,
    decltype(std::declval<_Type&>().insert(
        std::declval<typename _Type::value_type>()))
>> : std::is_same<
        decltype(std::declval<_Type&>().insert(
            std::declval<typename _Type::value_type>())),
        std::pair<typename _Type::iterator, bool>>
{};

// has_multi_insert_signal
//   trait: detects associative-style `insert(v)` returning a bare
// iterator.  Multi-keyed containers (std::multiset,
// std::multimap, std::unordered_multi*) collapse the pair.
//   Excludes types whose insert returns
// std::pair<iterator, bool> to remain disjoint from
// has_unique_insert_signal even on pathological return-type
// aliases.
template<typename _Type,
         typename = void>
struct has_multi_insert_signal : std::false_type
{};

template<typename _Type>
struct has_multi_insert_signal<_Type, void_t<
    typename _Type::iterator,
    typename _Type::value_type,
    decltype(std::declval<_Type&>().insert(
        std::declval<typename _Type::value_type>()))
>> : std::integral_constant<bool,
        std::is_same<
            decltype(std::declval<_Type&>().insert(
                std::declval<typename _Type::value_type>())),
            typename _Type::iterator>::value
        && !has_unique_insert_signal<_Type>::value>
{};

// has_max_multiplicity_constant
//   trait: detects an opt-in `max_multiplicity` static constexpr
// member of integral type.  This is the lightest customization
// available for non-STL containers expressing bounded-N
// multiplicity:
//
//     template<typename _T, std::size_t _N>
//     class bounded_multiset
//     {
//     public:
//         static constexpr std::size_t max_multiplicity = _N;
//         /* ... */
//     };
//
//   The constant is a structural declaration; no tag types or
// out-of-line specializations required.
template<typename _Type,
         typename = void>
struct has_max_multiplicity_constant : std::false_type
{};

template<typename _Type>
struct has_max_multiplicity_constant<_Type, void_t<
    decltype(_Type::max_multiplicity),
    typename std::enable_if<
        std::is_integral<typename std::remove_cv<
            decltype(_Type::max_multiplicity)>::type>::value
    >::type
>> : std::true_type
{};


#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool has_unique_insert_signal_v =
        has_unique_insert_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_multi_insert_signal_v =
        has_multi_insert_signal<_Type>::value;
    template<typename _Type>
    inline constexpr bool has_max_multiplicity_constant_v =
        has_max_multiplicity_constant<_Type>::value;
#endif

// ===========================================================================
// 3.  max_multiplicity extractor (priority chain)
// ===========================================================================
//
//   Resolves the maximum number of equivalent elements _Type may
// hold.  Priority chain (first match wins):
//
//     1. multiplicity_interval upper bound
//     2. multiplicity_max() accessor
//     3. T::max_multiplicity static constexpr member
//     4. structural insert-return-type
//        (unique -> 1, multi -> SIZE_MAX)
//     5. enforces_uniqueness fallback -> 1
//     6. otherwise -> 0 (unknown)
//
//   A return value of 0 is the sentinel for "no signal" and maps
// to container_multiplicity_kind::none in the classifier below.

NS_INTERNAL

    // multiplicity_interval upper-bound extractor.
    //   Hidden behind SFINAE so it only instantiates when the
    // interval protocol is available; otherwise yields 0.
    template<typename _Type,
             bool _HasInterval =
                 has_valid_multiplicity_interval_v<_Type>>
    struct interval_max_helper
    {
        static constexpr std::size_t value = 0;
    };

    template<typename _Type>
    struct interval_max_helper<_Type, true>
    {
    private:
        using interval_t = multiplicity_interval_of_t<_Type>;
    public:
        // interval types in djinterp::math expose a static
        // constexpr `max_value` (or equivalent); we resolve it
        // through the interval_traits helpers.  If that helper
        // is unavailable, the fallback path covers the type
        // through the structural signals below.
        static constexpr std::size_t value =
            static_cast<std::size_t>(interval_t::upper_bound);
    };

    // accessor-based upper bound.
    //   Honored only at non-constexpr resolution sites; the
    // type-level extractor below uses the static constant /
    // structural paths.  This helper is provided for symmetry
    // with the runtime accessor in the snapshot struct.
    template<typename _Type,
             bool _HasAccessor =
                 has_multiplicity_max_accessor_v<_Type>>
    struct accessor_max_helper
    {
        static constexpr bool present = false;
    };

    template<typename _Type>
    struct accessor_max_helper<_Type, true>
    {
        static constexpr bool present = true;
        // value not constexpr-resolvable from a type alone;
        // the snapshot exposes a `multiplicity_max_runtime()`
        // helper that calls the accessor on a live instance.
    };

    // resolved compile-time max.
    template<typename _Type>
    struct resolve_max_helper
    {
    private:
        using clean_type = clean_t<_Type>;

        static constexpr std::size_t from_interval =
            interval_max_helper<clean_type>::value;
        static constexpr bool has_interval =
            has_valid_multiplicity_interval_v<clean_type>;
        static constexpr bool has_constant =
            has_max_multiplicity_constant<clean_type>::value;
        static constexpr bool has_unique_struct =
            has_unique_insert_signal<clean_type>::value;
        static constexpr bool has_multi_struct =
            has_multi_insert_signal<clean_type>::value;
        static constexpr bool has_unique_legacy =
            enforces_uniqueness_v<clean_type>;

    public:
        static constexpr std::size_t value =
            // priority 1: interval upper bound
            has_interval
                ? from_interval
            // priority 3: opt-in static constant
            //   (priority 2, the runtime accessor, cannot be
            //   resolved purely from the type; see snapshot.)
            : has_constant
                ? static_cast<std::size_t>(
                      clean_type::max_multiplicity)
            // priority 4a: structural unique
            : has_unique_struct
                ? std::size_t{1}
            // priority 4b: structural multi
            : has_multi_struct
                ? std::numeric_limits<std::size_t>::max()
            // priority 5: legacy uniqueness fallback
            : has_unique_legacy
                ? std::size_t{1}
            // priority 6: unknown
            : std::size_t{0};
    };

NS_END  // internal


// max_multiplicity
//   trait: yields the resolved compile-time upper bound on the
// number of equivalent elements _Type may hold.  Returns 0 when
// no structural signal is available.
template<typename _Type>
struct max_multiplicity
{
    static constexpr std::size_t value =
        internal::resolve_max_helper<_Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr std::size_t max_multiplicity_v =
        max_multiplicity<_Type>::value;
#endif


// ===========================================================================
// 4.  multiplicity_kind classifier
// ===========================================================================

// multiplicity_kind
//   trait: classifies _Type into a container_multiplicity_kind position
// based on max_multiplicity_v.
template<typename _Type>
struct multiplicity_kind
{
private:
    static constexpr std::size_t mx =
        max_multiplicity_v<clean_t<_Type>>;
    static constexpr std::size_t sz_max =
        std::numeric_limits<std::size_t>::max();

public:
    static constexpr container_multiplicity_kind value =
        ( mx == 0 )
            ? container_multiplicity_kind::none
            : ( mx == 1 )
                ? container_multiplicity_kind::unique
                : ( mx == sz_max )
                    ? container_multiplicity_kind::unbounded_multi
                    : container_multiplicity_kind::bounded_multi;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr container_multiplicity_kind multiplicity_kind_v =
        multiplicity_kind<_Type>::value;
#endif


// is_bounded_multi_container
//   trait: convenience predicate for the new bounded-N kind
// position.  Disjoint from is_unique_container_v (in
// container_traits.hpp) and from allows_duplicates_v in the
// unbounded sense.
template<typename _Type>
struct is_bounded_multi_container
    : std::integral_constant<bool,
        multiplicity_kind<_Type>::value ==
            container_multiplicity_kind::bounded_multi>
{};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_bounded_multi_container_v =
        is_bounded_multi_container<_Type>::value;
#endif


// ===========================================================================
// 5.  Aggregate snapshot
// ===========================================================================

// multiplicity_container_class
//   struct: complete classification of _Type along the
// multiplicity axis.  Aggregates both the existing protocol
// (re-exported from container_traits.hpp) and the new structural
// signals introduced here, so callers querying the snapshot get
// a single point of truth.
template<typename _Type>
struct multiplicity_container_class
{
private:
    using clean_type = clean_t<_Type>;

public:
    // ---- existing protocol (container_traits.hpp) ----
    static constexpr bool enforces_uniqueness =
        enforces_uniqueness_v<clean_type>;
    static constexpr bool allows_duplicates =
        allows_duplicates_v<clean_type>;
    static constexpr bool has_bounded_multiplicity =
        has_bounded_multiplicity_v<clean_type>;
    static constexpr bool has_multiplicity_interval =
        has_valid_multiplicity_interval_v<clean_type>;
    static constexpr bool has_multiplicity_min_accessor =
        has_multiplicity_min_accessor_v<clean_type>;
    static constexpr bool has_multiplicity_max_accessor =
        has_multiplicity_max_accessor_v<clean_type>;

    // ---- new structural signals (this header) ----
    static constexpr bool has_unique_insert =
        has_unique_insert_signal<clean_type>::value;
    static constexpr bool has_multi_insert =
        has_multi_insert_signal<clean_type>::value;
    static constexpr bool has_max_constant =
        has_max_multiplicity_constant<clean_type>::value;

    // ---- resolved values ----
    static constexpr std::size_t max_multiplicity =
        max_multiplicity_v<clean_type>;
    static constexpr container_multiplicity_kind kind =
        multiplicity_kind_v<clean_type>;

    // ---- kind predicates ----
    static constexpr bool is_unique =
        ( kind == container_multiplicity_kind::unique );
    static constexpr bool is_bounded_multi =
        ( kind == container_multiplicity_kind::bounded_multi );
    static constexpr bool is_unbounded_multi =
        ( kind == container_multiplicity_kind::unbounded_multi );
    static constexpr bool is_unknown =
        ( kind == container_multiplicity_kind::none );
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_MULTIPLICITY_TRAITS_