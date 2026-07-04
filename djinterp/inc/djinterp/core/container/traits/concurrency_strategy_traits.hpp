/******************************************************************************
* djinterp [container]                         concurrency_strategy_traits.hpp
*
* Concurrency-strategy classification traits.
*   Orthogonal companion to threadsafe_container_traits.hpp.  Where the
* threadsafe traits classify a container by safety LEVEL (none, atomic_only,
* exclusive, shared, timed, shared_timed), this header classifies by the
* synchronization STRATEGY - the realisation by which a container witnesses
* the monograph's concurrency axis (linearizability of every history):
*     locked    - mutual exclusion under a lock policy (one lock per op)
*                 (threadsafe_array, threadsafe_tree, ...)
*     cow       - copy-on-write: writers publish a new immutable version
*                 (cow_array, cow_state-backed containers)
*     rcu       - read-copy-update: readers run lock-free, writers defer
*                 reclamation to a grace period
*                 (rcu_array, rcu_protected-backed containers)
*     atomic    - lock-free per-element std::atomic<T>
*                 (atomic_array, atomic counters)
*     hazard    - hazard-pointer-protected lock-free pointer chasing
*                 (lock-free lists, queues with deferred reclamation)
*     none      - no synchronization (the sequential, non-concurrent default;
*                 an IMMUTABLE container is nonetheless concurrent vacuously)
*     hybrid    - combines two or more of the above
*
*   FORMAL BASIS (containers_monograph, "Concurrency").  A container is
* CONCURRENT iff every history it admits, over every finite agent set, is
* LINEARIZABLE with respect to its sequential specification.  The immutable
* case is trivial (no mutation to order), so the axis has weight only on
* mutable, shared containers.  What distinguishes concurrent containers is
* not their content but their realisation, and each realisation leaves a
* characteristic SIGNATURE on the surrounding axes:
*     - a PROGRESS grade   : sequential < blocking < lock-free < wait-free
*     - an ARITY           : single-writer/multi-reader < multi-writer/...
*     - an ITERATION mode  : snapshot / weakly-consistent (/ inconsistent)
*     - a RECLAMATION duty : none / individual / reference-count /
*                            grace-period / hazard / collector
* Section VI records that signature per strategy; Section IV.b records the
* vacuous-concurrency rule (immutable => concurrent) and its complement
* (mutable + unsynchronised => sequential).
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
IV.b    vacuous concurrency and the sequential complement
V.      strategy deduction
VI.     monograph concurrency signature (progress / arity / iteration / reclamation)
VII.    combined classification
*/

#ifndef DJINTERP_CONTAINER_CONCURRENCY_STRATEGY_TRAITS_
#define DJINTERP_CONTAINER_CONCURRENCY_STRATEGY_TRAITS_ 1

// std
#include <cstddef>
#include <cstdint>
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

// C++98: expose the enumerators through the ordinary-namespace name too, so
// `concurrency_strategy::locked` (tag lookup) and a `concurrency_strategy`
// typed variable both spell the same thing.  (The C++11 branch needs no such
// alias: the scoped enum already IS the type of that name.)
typedef concurrency_strategy::value_type concurrency_strategy_value;

#endif  // C++11

// strategy tag types
//   tag-dispatch: concrete types expose one of these as
// `concurrency_strategy_tag` to declare their strategy.
//
//   The seven tag struct definitions live in
// /sync/concurrency_strategy_tags.hpp so that the foundation
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
D_TYPE_TRAIT_DETECTED(has_concurrency_strategy_tag,
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
D_TYPE_TRAIT_DETECTED(has_read_lock_method,
                  decltype(std::declval<const _Type&>().read_lock()))

D_TYPE_TRAIT_DETECTED(has_write_lock_method,
                  decltype(std::declval<_Type&>().write_lock()))


// --- cow: container has a snapshot() returning a copy-handle ---
D_TYPE_TRAIT_DETECTED(has_snapshot_method,
                  decltype(std::declval<const _Type&>().snapshot()))

D_TYPE_TRAIT_DETECTED(has_cow_state_type,
                  typename _Type::cow_state_type)


// --- rcu: container has rcu_read() / rcu_protected member ---
D_TYPE_TRAIT_DETECTED(has_rcu_protected_type,
                  typename _Type::rcu_protected_type)

D_TYPE_TRAIT_DETECTED(has_epoch_type,
                  typename _Type::epoch_counter_type)


// --- atomic: value_type is std::atomic<U>, or has load(i, order) ---
NS_INTERNAL

    template<typename _Type, typename = void>
    struct value_is_atomic_check : std::false_type
    {};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // NOTE: uses djinterp's portable `void_t` (from meta/type_traits.hpp),
    // NOT std::void_t - the latter is C++17 and would break this C++11-guarded
    // branch on C++11/14 toolchains.
    template<typename _Type>
    struct value_is_atomic_check<_Type,
        void_t<typename _Type::value_type>>
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


D_TYPE_TRAIT_DETECTED(has_atomic_load_at,
    decltype(std::declval<const _Type&>().load(
        std::declval<std::size_t>())))


// --- hazard: container has a hazard_domain member type ---

D_TYPE_TRAIT_DETECTED(has_hazard_domain_type,
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


// =============================================================================
// IV.b Vacuous Concurrency and the Sequential Complement
// =============================================================================
//   The monograph's concurrency axis is entailed-INTO by immutability:
// an immutable container is concurrent VACUOUSLY (all its operations are
// queries, so no history has anything to linearize, and any linear extension
// of real-time order witnesses it).  A synchronization strategy is therefore
// SUFFICIENT for concurrency but not NECESSARY.  Conversely, a MUTABLE
// container with no strategy is SEQUENTIAL (non-concurrent): correct only
// under a single agent.

// is_vacuously_concurrent
//   type trait: true if _Type is concurrent for the trivial reason -
// it is immutable, so there is no mutation to order (monograph: "the
// immutable case is trivial").  Carries no synchronization machinery.
template<typename _Type>
struct is_vacuously_concurrent
{
    static constexpr bool value =
        is_immutable_container_v<clean_t<_Type>>;
};

template<typename _Type>
inline constexpr bool is_vacuously_concurrent_v =
    is_vacuously_concurrent<_Type>::value;


// is_concurrent_container
//   type trait: true if the container is concurrent in the monograph's
// sense - either it carries a synchronization strategy (a witness for
// linearizability under mutation) OR it is immutable (concurrent
// vacuously).  This is the trait's alignment with the formal definition:
// concurrency is NOT the same as "has a lock".
template<typename _Type>
struct is_concurrent_container
{
    static constexpr bool value =
        ( is_locked_container_v<_Type>       ||
          is_cow_container_v<_Type>          ||
          is_rcu_container_v<_Type>          ||
          is_atomic_container_v<_Type>       ||
          is_hazard_container_v<_Type>       ||
          is_vacuously_concurrent_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_concurrent_container_v =
    is_concurrent_container<_Type>::value;


// is_synchronized_container
//   type trait: true if the container carries an ACTIVE synchronization
// strategy (locked / cow / rcu / atomic / hazard).  This is the narrower
// question "does it run machinery to stay linearizable under mutation?",
// distinct from is_concurrent_container, which also admits the immutable
// (machinery-free) case.
template<typename _Type>
struct is_synchronized_container
{
    static constexpr bool value =
        ( is_locked_container_v<_Type>  ||
          is_cow_container_v<_Type>     ||
          is_rcu_container_v<_Type>     ||
          is_atomic_container_v<_Type>  ||
          is_hazard_container_v<_Type> );
};

template<typename _Type>
inline constexpr bool is_synchronized_container_v =
    is_synchronized_container<_Type>::value;


// is_sequential_container
//   type trait: true if the container is NON-concurrent - the tacit
// single-agent default of the preceding chapters.  A container is
// sequential exactly when it is neither synchronized nor immutable.
//   (Note: "sequential" here is the monograph's concurrency-axis term for
// "not safe under overlap"; it is unrelated to sequence/ordered shape.)
template<typename _Type>
struct is_sequential_container
{
    static constexpr bool value =
        !is_concurrent_container_v<_Type>;
};

template<typename _Type>
inline constexpr bool is_sequential_container_v =
    is_sequential_container<_Type>::value;


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
// VI.  Monograph Concurrency Signature
// =============================================================================
//   The monograph fixes a container's place on the concurrency axis as a
// tuple: a PROGRESS grade and an ARITY, together with the ITERATION overlap-
// semantics it offers and the RECLAMATION discipline it owes.  Each strategy
// carries a characteristic value of that tuple (the strategy table of the
// "Concurrency by strategy" section).  The per-strategy signature below makes
// that tuple first-class, so a container's realisation can be read off its
// strategy rather than restated by hand.
//
//   The four coordinate enums (concurrency_progress, concurrency_arity,
// iteration_consistency, reclamation_obligation) are the shared concurrency
// VOCABULARY and are defined once in concurrency_strategy_tags.hpp (included
// above), so the lower mechanism layer can name a progress grade without a
// cycle.  This header owns only the strategy -> signature MAPPING and the
// `concurrency_signature` aggregate that bundles them.

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// concurrency_signature
//   struct: the monograph tuple for a realisation.  Progress is split into
// READ and WRITE grades because several strategies differ across the two
// (e.g. copy-on-write and RCU are wait-free for readers while serializing
// writers).
struct concurrency_signature
{
    concurrency_progress   read_progress;
    concurrency_progress   write_progress;
    concurrency_arity      arity;
    iteration_consistency  iteration;
    reclamation_obligation reclamation;
};


NS_INTERNAL

    // concurrency_signature_for
    //   helper: the signature carried by each strategy, taken from the
    // monograph's strategy table.  Primary template covers `none` and any
    // future value (the sequential, non-concurrent default).
    template<concurrency_strategy _S>
    struct concurrency_signature_for
    {
        // none: the single-agent default.  (An immutable container is
        // concurrent vacuously - wait-free / mwmr / snapshot / none - but
        // that is a property of the VALUE, tested by is_vacuously_concurrent,
        // not of this strategy classification.)
        static constexpr concurrency_signature value = {
            concurrency_progress::sequential,
            concurrency_progress::sequential,
            concurrency_arity::swmr,
            iteration_consistency::inconsistent,
            reclamation_obligation::none
        };
    };

    // locked: one lock per operation.  Blocking; MWMR at the interface but
    // serial in execution (a reader-writer policy grants concurrent reads -
    // see supports_concurrent_reads).  A traversal under the lock sees a
    // frozen value (snapshot).  Reclamation is individual: freeing happens
    // under the lock, so no agent can hold a stale reference.
    template<>
    struct concurrency_signature_for<concurrency_strategy::locked>
    {
        static constexpr concurrency_signature value = {
            concurrency_progress::blocking,
            concurrency_progress::blocking,
            concurrency_arity::mwmr,
            iteration_consistency::snapshot,
            reclamation_obligation::individual
        };
    };

    // cow: writers publish a new immutable version by a single atomic swing
    // of the root; readers dereference once and then read an immutable body.
    // Wait-free reads; writers lock-free (CAS on root) or serialized.  A
    // reader's version never changes under it (snapshot by construction).
    // Old versions are freed once no reader holds them (reference count on
    // versions).
    template<>
    struct concurrency_signature_for<concurrency_strategy::cow>
    {
        static constexpr concurrency_signature value = {
            concurrency_progress::wait_free,
            concurrency_progress::lock_free,
            concurrency_arity::swmr,
            iteration_consistency::snapshot,
            reclamation_obligation::reference_count
        };
    };

    // rcu: readers run in a synchronization-free critical section (a single
    // dereference); writers, serialized among themselves, publish with a
    // release and defer reclamation to a grace period.  Wait-free reads;
    // writers serialized (blocking).  Weakly-consistent unless a reader
    // snapshots.
    template<>
    struct concurrency_signature_for<concurrency_strategy::rcu>
    {
        static constexpr concurrency_signature value = {
            concurrency_progress::wait_free,
            concurrency_progress::blocking,
            concurrency_arity::swmr,
            iteration_consistency::weakly_consistent,
            reclamation_obligation::grace_period
        };
    };

    // atomic: lock-free per-element std::atomic<T> over inline storage.
    // Lock-free reads and writes, MWMR, weakly-consistent traversal.  Cells
    // are inline (not separately allocated node cells), so there is no
    // deferred-reclamation obligation.
    template<>
    struct concurrency_signature_for<concurrency_strategy::atomic>
    {
        static constexpr concurrency_signature value = {
            concurrency_progress::lock_free,
            concurrency_progress::lock_free,
            concurrency_arity::mwmr,
            iteration_consistency::weakly_consistent,
            reclamation_obligation::none
        };
    };

    // hazard: lock-free pointer chasing where each reader publishes a hazard
    // and validates before dereferencing; a reclaimer frees an unlinked node
    // only when no hazard slot names it.  Lock-free, MWMR, weakly-consistent;
    // reclamation by hazard scan (bounded un-reclaimed memory per agent).
    template<>
    struct concurrency_signature_for<concurrency_strategy::hazard>
    {
        static constexpr concurrency_signature value = {
            concurrency_progress::lock_free,
            concurrency_progress::lock_free,
            concurrency_arity::mwmr,
            iteration_consistency::weakly_consistent,
            reclamation_obligation::hazard
        };
    };

    // hybrid: two or more strategies combined (e.g. locked metadata over a
    // cow payload).  No single signature is exact; the conservative meet is
    // reported - blocking, MWMR, weakly-consistent, individual - and callers
    // that need precision should inspect the component strategies.
    template<>
    struct concurrency_signature_for<concurrency_strategy::hybrid>
    {
        static constexpr concurrency_signature value = {
            concurrency_progress::blocking,
            concurrency_progress::blocking,
            concurrency_arity::mwmr,
            iteration_consistency::weakly_consistent,
            reclamation_obligation::individual
        };
    };

NS_END  // internal


// concurrency_signature_of
//   type trait: the monograph signature (progress / arity / iteration /
// reclamation) a container carries, deduced from its strategy.
template<typename _Type>
struct concurrency_signature_of
{
    static constexpr concurrency_signature value =
        internal::concurrency_signature_for<
            concurrency_strategy_of_v<_Type>>::value;
};

template<typename _Type>
inline constexpr concurrency_signature concurrency_signature_of_v =
    concurrency_signature_of<_Type>::value;


// --- convenience projections of the signature -------------------------------

// read_progress_of / write_progress_of
//   trait: the per-side progress grade of _Type's realisation.
template<typename _Type>
inline constexpr concurrency_progress read_progress_of_v =
    concurrency_signature_of<_Type>::value.read_progress;

template<typename _Type>
inline constexpr concurrency_progress write_progress_of_v =
    concurrency_signature_of<_Type>::value.write_progress;

// concurrency_arity_of
//   trait: the interface arity (SWMR / MWMR) of _Type's realisation.
template<typename _Type>
inline constexpr concurrency_arity concurrency_arity_of_v =
    concurrency_signature_of<_Type>::value.arity;

// iteration_consistency_of
//   trait: the overlap-semantics of a concurrent traversal of _Type.
template<typename _Type>
inline constexpr iteration_consistency iteration_consistency_of_v =
    concurrency_signature_of<_Type>::value.iteration;

// reclamation_obligation_of
//   trait: the safe-reclamation discipline _Type owes.
template<typename _Type>
inline constexpr reclamation_obligation reclamation_obligation_of_v =
    concurrency_signature_of<_Type>::value.reclamation;

// is_lock_free_container / is_wait_free_reader_container
//   trait: progress-grade shorthands.  A container is "lock-free" here when
// BOTH sides are at least lock-free; "wait-free reader" when reads are
// wait-free (the property COW and RCU buy).
template<typename _Type>
inline constexpr bool is_lock_free_container_v =
    ( read_progress_of_v<_Type>  >= concurrency_progress::lock_free &&
      write_progress_of_v<_Type> >= concurrency_progress::lock_free );

template<typename _Type>
inline constexpr bool is_wait_free_reader_container_v =
    ( read_progress_of_v<_Type> == concurrency_progress::wait_free );

// offers_snapshot_iteration_v
//   trait: true if a concurrent traversal sees one frozen value.
template<typename _Type>
inline constexpr bool offers_snapshot_iteration_v =
    ( iteration_consistency_of_v<_Type> == iteration_consistency::snapshot );

#endif  // C++11


// =============================================================================
// VII. Combined Classification
// =============================================================================

// container_concurrency_class
//   struct: complete concurrency classification.  Combines
// strategy (this file) with safety level (threadsafe
// container traits) and the monograph signature.  All
// members are static constexpr.
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

    // aggregate concurrency (monograph axis)
    static constexpr bool is_concurrent = is_concurrent_container_v<_Type>;
    static constexpr bool is_synchronized = is_synchronized_container_v<_Type>;
    static constexpr bool is_vacuously_concurrent =
        is_vacuously_concurrent_v<_Type>;
    static constexpr bool is_sequential = is_sequential_container_v<_Type>;
    static constexpr bool concurrent_reads =
        supports_concurrent_reads_v<_Type>;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // monograph signature (progress / arity / iteration / reclamation)
    static constexpr concurrency_signature  signature =
        concurrency_signature_of_v<_Type>;
    static constexpr concurrency_progress   read_progress =
        signature.read_progress;
    static constexpr concurrency_progress   write_progress =
        signature.write_progress;
    static constexpr concurrency_arity      arity =
        signature.arity;
    static constexpr iteration_consistency  iteration =
        signature.iteration;
    static constexpr reclamation_obligation reclamation =
        signature.reclamation;
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER
};


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_CONCURRENCY_STRATEGY_TRAITS_
