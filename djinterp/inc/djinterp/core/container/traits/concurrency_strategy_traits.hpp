/******************************************************************************
* djinterp [container]                         concurrency_strategy_traits.hpp
*
* Concurrency-strategy classification traits.
*   Orthogonal companion to threadsafe_container_traits.hpp.  Where the
* threadsafe traits classify a container by safety LEVEL (none, atomic_only,
* exclusive, shared, timed, shared_timed), this header classifies by the
* synchronization STRATEGY:
*     locked    - mutex / rwlock under a lock policy
*                 (threadsafe_array, threadsafe_tree, ...)
*     cow       - copy-on-write with snapshot handles
*                 (cow_array, cow_state-backed containers)
*     rcu       - read-copy-update / epoch-based reclamation
*                 (rcu_array, rcu_protected-backed containers)
*     atomic    - lock-free per-element std::atomic<T>
*                 (atomic_array, atomic counters)
*     hazard    - hazard-pointer-protected lock-free pointer chasing
*                 (lock-free lists, queues with deferred reclamation)
*     none      - no synchronization
*     hybrid    - combines two or more of the above
*
*   The two axes (LEVEL and STRATEGY) are independent.  A
* cow_array<T, N, L, I, shared_lock_policy> is BOTH `shared` level AND
* `cow` strategy.  An atomic_array<T, N> is `atomic_only` level AND
* `atomic` strategy.
*   Detection has two paths:
*     1. Tag-alias (preferred): containers expose
*        `using concurrency_strategy_tag = locked_strategy_tag;`
*        and the trait reads it directly.  Fast and unambiguous.
*     2. Structural fallback: when no tag is present, the trait
*        looks for canonical members (e.g. `lock_policy_type` +
*        `read_lock()` for locked; `snapshot()` returning an
*        immutable_snapshot for cow; etc.).
*
* DEPENDENCIES:
*   threadsafe_container_traits.hpp  - level detection, mutex extraction
*   threadsafe.hpp                   - strategy primitives (cow_state,
*                                      rcu_protected, hazard_domain)
*
*
* path:      /inc/djinterp/core/container/sync/concurrency_strategy_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.26
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.      strategy enum and tag types
II.     tag-alias detection
III.    structural detection helpers
IV.     per-strategy predicates
V.      strategy deduction
VI.     combined dlassification
*/

#ifndef DJINTERP_CONTAINER_CONCURRENCY_STRATEGY_TRAITS_
#define DJINTERP_CONTAINER_CONCURRENCY_STRATEGY_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../sync/concurrency_strategy_tags.hpp"
#include "../traits/container_traits.hpp"
#include "../traits/threadsafe_container_traits.hpp"


NS_DJINTERP

// =============================================================================
// I.   Strategy Enum and Tag Types
// =============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// concurrency_strategy
//   enum: classifies the synchronization strategy.
// Orthogonal to thread_safety_level - the two axes
// combine to fully describe a container's concurrency.
enum class concurrency_strategy
{
    none    = 0,
    locked  = 1,
    cow     = 2,
    rcu     = 3,
    atomic  = 4,
    hazard  = 5,
    hybrid  = 6
};

using concurrency_strategy = concurrency_strategy;

#else

struct concurrency_strategy
{
    enum value_type
    {
        none    = 0,
        locked  = 1,
        cow     = 2,
        rcu     = 3,
        atomic  = 4,
        hazard  = 5,
        hybrid  = 6
    };
};

typedef concurrency_strategy::value_type concurrency_strategy;

#endif  // C++11

// strategy tag types
//   tag-dispatch: concrete types expose one of these as
// `concurrency_strategy_tag` to declare their strategy.
//
//   The seven tag struct definitions live in
// /meta/concurrency_strategy_tags.hpp so that the foundation
// primitives in /sync (atomic, cow, rcu, hazard_pointer)
// and the container base in /container can self-tag
// without taking a transitive dependency on this trait
// header.  They are visible here unchanged through the
// concurrency_strategy_tags.hpp include above.


// =============================================================================
// II.  Tag-Alias Detection
// =============================================================================

// has_concurrency_strategy_tag
//   type trait: true if the container exposes a
// concurrency_strategy_tag alias.
D_TYPE_TRAIT_TRUE(has_concurrency_strategy_tag,
                  typename _Type::concurrency_strategy_tag)


NS_INTERNAL

    // tag_to_strategy
    //   helper: maps a tag type to its concurrency_strategy value.
    template<typename _Tag>
    struct tag_to_strategy
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::none;
    };

    template<>
    struct tag_to_strategy<locked_strategy_tag>
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::locked;
    };

    template<>
    struct tag_to_strategy<cow_strategy_tag>
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::cow;
    };

    template<>
    struct tag_to_strategy<rcu_strategy_tag>
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::rcu;
    };

    template<>
    struct tag_to_strategy<atomic_strategy_tag>
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::atomic;
    };

    template<>
    struct tag_to_strategy<hazard_strategy_tag>
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::hazard;
    };

    template<>
    struct tag_to_strategy<hybrid_strategy_tag>
    {
        static constexpr concurrency_strategy value =
            concurrency_strategy::hybrid;
    };

    // safe_strategy_tag
    //   helper: yields _Type::concurrency_strategy_tag if
    // the alias exists, otherwise none_strategy_tag.  The
    // SFINAE indirection is required because `&&` short-
    // circuits value evaluation but NOT type instantiation
    // - a bare `typename _Type::concurrency_strategy_tag`
    // hard-fails for types that lack the alias, regardless
    // of any preceding has_*_v<> guard in the same
    // expression.  Using this helper makes the tag lookup
    // always well-formed; types without the alias yield
    // none_strategy_tag, which never matches any real
    // strategy tag in the per-predicate is_same<> checks.
    template<typename _Type,
             bool = has_concurrency_strategy_tag_v<_Type>>
    struct safe_strategy_tag
    {
        using type = none_strategy_tag;
    };

    template<typename _Type>
    struct safe_strategy_tag<_Type, true>
    {
        using type = typename _Type::concurrency_strategy_tag;
    };

    template<typename _Type>
    using safe_strategy_tag_t =
        typename safe_strategy_tag<_Type>::type;

NS_END  // internal


// =============================================================================
// III. Structural Detection Helpers
// =============================================================================
// Used as fallback when no concurrency_strategy_tag is
// present.  Each helper detects a canonical member that
// indicates the strategy.

// --- locked: container has a lock policy + read_lock() ---
D_TYPE_TRAIT_TRUE(has_read_lock_method,
                  decltype(std::declval<const _Type&>().read_lock()))

D_TYPE_TRAIT_TRUE(has_write_lock_method,
                  decltype(std::declval<_Type&>().write_lock()))


// --- cow: container has a snapshot() returning a copy-handle ---
D_TYPE_TRAIT_TRUE(has_snapshot_method,
                  decltype(std::declval<const _Type&>().snapshot()))

D_TYPE_TRAIT_TRUE(has_cow_state_type,
                  typename _Type::cow_state_type)


// --- rcu: container has rcu_read() / rcu_protected member ---
D_TYPE_TRAIT_TRUE(has_rcu_protected_type,
                  typename _Type::rcu_protected_type)

D_TYPE_TRAIT_TRUE(has_epoch_type,
                  typename _Type::epoch_counter_type)


// --- atomic: value_type is std::atomic<U>, or has load(i, order) ---
NS_INTERNAL

    template<typename _Type, typename = void>
    struct value_is_atomic_check : std::false_type
    {};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    template<typename _Type>
    struct value_is_atomic_check<_Type,
        std::void_t<typename _Type::value_type>>
    {
        // strip cvref, then probe for the std::atomic<U>
        // interface: a load(memory_order) member that
        // returns something convertible from the atomic.
        template<typename _U>
        static auto test(int)
            -> decltype(std::declval<_U&>().load(
                            std::memory_order_seq_cst),
                        std::true_type{});

        template<typename>
        static std::false_type test(...);

        static constexpr bool value =
            decltype(test<typename _Type::value_type>(0))::value;
    };
#endif

NS_END  // internal


D_TYPE_TRAIT_TRUE(has_atomic_load_at,
    decltype(std::declval<const _Type&>().load(
        std::declval<std::size_t>())))


// --- hazard: container has a hazard_domain member type ---

D_TYPE_TRAIT_TRUE(has_hazard_domain_type,
    typename _Type::hazard_domain_type)


// =============================================================================
// IV.  Per-Strategy Predicates
// =============================================================================

// is_locked_container
//   type trait: true if the container uses mutex/rwlock
// synchronization under a lock policy.
//
//   Detection precedence: when the type declares a
// `concurrency_strategy_tag` alias the tag is authoritative
// and structural detection is ignored; otherwise the
// structural fallback (lock policy + read_lock/write_lock
// methods) is consulted.  This mirrors the same rule used
// by `concurrency_strategy_helper` below and prevents
// incidental structural matches (e.g. a locked container
// that also exposes `snapshot()`) from being misclassified
// as a different strategy.
template<typename _Type>
struct is_locked_container
{
private:
    using clean_type = clean_t<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr bool tag_present =
        has_concurrency_strategy_tag_v<clean_type>;

    static constexpr bool by_tag =
        ( tag_present &&
          std::is_same<
              internal::safe_strategy_tag_t<clean_type>,
              locked_strategy_tag>::value );
#else
    static const bool tag_present = false;
    static const bool by_tag      = false;
#endif

    static constexpr bool by_structure =
        ( has_lock_policy_type_v<clean_type> &&
          ( has_read_lock_method_v<clean_type> ||
            has_write_lock_method_v<clean_type> ) );

public:
    // tag wins when present; structural detection only
    // fires for tag-less types.
    static constexpr bool value =
        ( tag_present ? by_tag : by_structure );
};

template<typename _Type>
inline constexpr bool is_locked_container_v =
    is_locked_container<_Type>::value;


// is_mutex_container
//   alias: synonym for is_locked_container.  "mutex"
// reads more naturally at some call sites; "locked" reads
// more naturally at others.  Both spellings are first-class.
template<typename _Type>
using is_mutex_container = is_locked_container<_Type>;

template<typename _Type>
inline constexpr bool is_mutex_container_v =
    is_locked_container_v<_Type>;


// is_cow_container
//   type trait: true if the container uses copy-on-write.
//   Detection precedence matches `is_locked_container`:
// the strategy tag (when declared) is authoritative,
// otherwise structural detection (cow_state alias or a
// `snapshot()` method) is used.
template<typename _Type>
struct is_cow_container
{
private:
    using clean_type = clean_t<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr bool tag_present =
        has_concurrency_strategy_tag_v<clean_type>;

    static constexpr bool by_tag =
        ( tag_present &&
          std::is_same<
              internal::safe_strategy_tag_t<clean_type>,
              cow_strategy_tag>::value );
#else
    static const bool tag_present = false;
    static const bool by_tag      = false;
#endif

    static constexpr bool by_structure =
        ( has_cow_state_type_v<clean_type> ||
          has_snapshot_method_v<clean_type> );

public:
    static constexpr bool value =
        ( tag_present ? by_tag : by_structure );
};

template<typename _Type>
inline constexpr bool is_cow_container_v =
    is_cow_container<_Type>::value;


// is_rcu_container
//   type trait: true if the container uses RCU /
// epoch-based reclamation.
//   Detection precedence matches `is_locked_container`:
// the strategy tag (when declared) is authoritative.
template<typename _Type>
struct is_rcu_container
{
private:
    using clean_type = clean_t<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr bool tag_present =
        has_concurrency_strategy_tag_v<clean_type>;

    static constexpr bool by_tag =
        ( tag_present &&
          std::is_same<
              internal::safe_strategy_tag_t<clean_type>,
              rcu_strategy_tag>::value );
#else
    static const bool tag_present = false;
    static const bool by_tag      = false;
#endif

    static constexpr bool by_structure =
        ( has_rcu_protected_type_v<clean_type> ||
          has_epoch_type_v<clean_type> );

public:
    static constexpr bool value =
        ( tag_present ? by_tag : by_structure );
};

template<typename _Type>
inline constexpr bool is_rcu_container_v =
    is_rcu_container<_Type>::value;


// is_atomic_container
//   type trait: true if the container's elements are
// individually atomic (lock-free element access).
//   Detection precedence matches `is_locked_container`:
// the strategy tag (when declared) is authoritative.
template<typename _Type>
struct is_atomic_container
{
private:
    using clean_type = clean_t<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr bool tag_present =
        has_concurrency_strategy_tag_v<clean_type>;

    static constexpr bool by_tag =
        ( tag_present &&
          std::is_same<
              internal::safe_strategy_tag_t<clean_type>,
              atomic_strategy_tag>::value );

    static constexpr bool by_structure =
        ( internal::value_is_atomic_check<clean_type>::value ||
          has_atomic_load_at_v<clean_type> );
#else
    static const bool tag_present  = false;
    static const bool by_tag       = false;
    static const bool by_structure = false;
#endif

public:
    static constexpr bool value =
        ( tag_present ? by_tag : by_structure );
};

template<typename _Type>
inline constexpr bool is_atomic_container_v =
    is_atomic_container<_Type>::value;


// is_hazard_container
//   type trait: true if the container uses hazard pointer
// reclamation.
//   Detection precedence matches `is_locked_container`:
// the strategy tag (when declared) is authoritative.
template<typename _Type>
struct is_hazard_container
{
private:
    using clean_type = clean_t<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr bool tag_present =
        has_concurrency_strategy_tag_v<clean_type>;

    static constexpr bool by_tag =
        ( tag_present &&
          std::is_same<
              internal::safe_strategy_tag_t<clean_type>,
              hazard_strategy_tag>::value );
#else
    static const bool tag_present = false;
    static const bool by_tag      = false;
#endif

    static constexpr bool by_structure =
        has_hazard_domain_type_v<clean_type>;

public:
    static constexpr bool value =
        ( tag_present ? by_tag : by_structure );
};

template<typename _Type>
inline constexpr bool is_hazard_container_v =
    is_hazard_container<_Type>::value;


// is_concurrent_container
//   type trait: true if the container uses any
// concurrency strategy at all.
template<typename _Type>
struct is_concurrent_container
{
    static constexpr bool value =
        ( is_locked_container_v<_Type>  ||
          is_cow_container_v<_Type>     ||
          is_rcu_container_v<_Type>     ||
          is_atomic_container_v<_Type>  ||
          is_hazard_container_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_concurrent_container_v =
    is_concurrent_container<_Type>::value;


// =============================================================================
// V.   Strategy Deduction
// =============================================================================

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

NS_INTERNAL

    template<typename _Type>
    struct concurrency_strategy_helper
    {
        using clean_type = clean_t<_Type>;

        // count of strategy hits - used to detect hybrid
        static constexpr int hit_count =
            ( is_locked_container_v<clean_type>  ? 1 : 0 ) +
            ( is_cow_container_v<clean_type>     ? 1 : 0 ) +
            ( is_rcu_container_v<clean_type>     ? 1 : 0 ) +
            ( is_atomic_container_v<clean_type>  ? 1 : 0 ) +
            ( is_hazard_container_v<clean_type>  ? 1 : 0 );

        // tag wins if present
        static constexpr bool has_tag = has_concurrency_strategy_tag_v<clean_type>;

        static constexpr concurrency_strategy value =
            has_tag
                ? internal::tag_to_strategy<
                      internal::safe_strategy_tag_t<clean_type>>::value
            : ( hit_count >= 2 )
                ? concurrency_strategy::hybrid
            : is_locked_container_v<clean_type>
                ? concurrency_strategy::locked
            : is_cow_container_v<clean_type>
                ? concurrency_strategy::cow
            : is_rcu_container_v<clean_type>
                ? concurrency_strategy::rcu
            : is_atomic_container_v<clean_type>
                ? concurrency_strategy::atomic
            : is_hazard_container_v<clean_type>
                ? concurrency_strategy::hazard
            : concurrency_strategy::none;
    };

NS_END  // internal


// concurrency_strategy_of
//   type trait: deduces the concurrency_strategy value
// for a container.
template<typename _Type>
struct concurrency_strategy_of
{
    static constexpr concurrency_strategy value =
        internal::concurrency_strategy_helper<_Type>::value;
};

template<typename _Type>
inline constexpr concurrency_strategy concurrency_strategy_of_v =
    concurrency_strategy_of<_Type>::value;

#endif  // C++11


// =============================================================================
// VI.  Combined Classification
// =============================================================================

// container_concurrency_class
//   struct: complete concurrency classification.  Combines
// strategy (this file) with safety level (threadsafe
// container traits).  All members are static constexpr.
template<typename _Type>
struct container_concurrency_class
{
    // strategy
    static constexpr bool is_locked = is_locked_container_v<_Type>;
    static constexpr bool is_cow    = is_cow_container_v<_Type>;
    static constexpr bool is_rcu    = is_rcu_container_v<_Type>;
    static constexpr bool is_atomic = is_atomic_container_v<_Type>;
    static constexpr bool is_hazard = is_hazard_container_v<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static constexpr concurrency_strategy strategy =
        concurrency_strategy_of_v<_Type>;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    // level (forwarded from threadsafe traits)
    static constexpr thread_safety_level level = container_thread_safety_level_v<_Type>;

    // aggregate
    static constexpr bool is_concurrent = is_concurrent_container_v<_Type>;
    static constexpr bool concurrent_reads =
        supports_concurrent_reads_v<_Type>;
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONCURRENCY_STRATEGY_TRAITS_