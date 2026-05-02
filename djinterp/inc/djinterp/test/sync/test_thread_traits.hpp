/******************************************************************************
* djinterp [test]                                       test_thread_traits.hpp
*
*   Structural SFINAE detection for types participating in the DTest
* multithreading harness.  These traits classify types along axes that
* matter for concurrent testing: lock interface, snapshot/COW interface,
* atomic interface, and high-level strategy classification.
*
*   This header DOES NOT redefine the canonical detection probes that
* already exist in the project's container metaprogramming graph
* (concurrency_strategy_traits.hpp and threadsafe_container_traits.hpp).
* Instead it RE-EXPORTS those probes into the
* `djinterp::test::traits` namespace and adds the test-suite-specific
* composites and the few probes that are not present upstream.  This
* avoids two slightly different detection probes for the same concept
* drifting out of sync over time.
*
*   ALIASED FROM djinterp::traits (concurrency_strategy_traits.hpp):
*     has_concurrency_strategy_tag
*     has_read_lock_method, has_write_lock_method
*     has_snapshot_method, has_cow_state_type
*     has_rcu_protected_type, has_epoch_type
*     has_hazard_domain_type, has_atomic_load_at
*
*   ALIASED FROM djinterp (threadsafe_container_traits.hpp):
*     has_lock_policy_type, has_mutex_type_alias (renamed has_mutex_type)
*     has_atomic_size_type, has_atomic_version_type
*
*   DEFINED HERE (test-suite-specific):
*     has_thread_safety_level    - nested-alias detection upstream
*                                  exposes only a *value* extractor,
*                                  not a *presence* probe
*     has_try_lock_method        - convenience for try-lock tests
*     has_full_lock_interface    - composite (read + write)
*     has_publish_method         - COW publish() detection
*     has_cow_interface          - composite (snapshot + publish)
*     has_atomic_load_method     - no-arg load() (distinct from
*                                  upstream has_atomic_load_at)
*     has_atomic_store_method
*     has_atomic_compare_exchange_method
*     has_atomic_interface       - composite (load + store)
*     is_locked_testable, is_cow_testable, is_rcu_testable,
*       is_hazard_testable, is_lock_free_testable,
*       is_threadsafe_testable  - strategy-level composites
*     thread_test_class          - full structural classification
*
*
* TABLE OF CONTENTS
* =================
* I.    RE-EXPORTED TRAITS FROM djinterp::traits
* II.   RE-EXPORTED TRAITS FROM djinterp
* III.  TEST-SPECIFIC NESTED TYPE DETECTION
* IV.   TEST-SPECIFIC LOCK INTERFACE DETECTION
* V.    TEST-SPECIFIC SNAPSHOT / COW DETECTION
* VI.   TEST-SPECIFIC ATOMIC INTERFACE DETECTION
* VII.  STRATEGY-LEVEL COMPOSITES
* VIII. COMBINED CLASSIFICATION
* IX.   VARIABLE TEMPLATES
*
*
* path:      /inc/djinterp/test/test_thread_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.27
******************************************************************************/

#ifndef DJINTERP_TEST_THREAD_TRAITS_
#define DJINTERP_TEST_THREAD_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../core/djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "../container/meta/threadsafe_container_traits.hpp"
#include "../container/meta/concurrency_strategy_traits.hpp"
#include "./test_common.hpp"


NS_DJINTERP
NS_TEST
NS_TRAITS


///////////////////////////////////////////////////////////////////////////////
///                I.   RE-EXPORTED TRAITS FROM djinterp::traits            ///
///////////////////////////////////////////////////////////////////////////////
//   These traits are defined canonically in
// concurrency_strategy_traits.hpp.  Bringing them into
// the test::traits namespace lets test-side code spell
// them with a short qualified name while guaranteeing
// the test classification stays in lockstep with the
// container classification.

using ::djinterp::has_concurrency_strategy_tag;
using ::djinterp::has_read_lock_method;
using ::djinterp::has_write_lock_method;
using ::djinterp::has_snapshot_method;
using ::djinterp::has_cow_state_type;
using ::djinterp::has_rcu_protected_type;
using ::djinterp::has_epoch_type;
using ::djinterp::has_hazard_domain_type;
using ::djinterp::has_atomic_load_at;


///////////////////////////////////////////////////////////////////////////////
///                II.  RE-EXPORTED TRAITS FROM djinterp                    ///
///////////////////////////////////////////////////////////////////////////////
//   These traits live at namespace djinterp (no
// `traits` sub-namespace) in threadsafe_container_traits.hpp.
// They are re-exported here under their canonical names,
// with one rename: has_mutex_type_alias is exposed as
// has_mutex_type for symmetry with the rest of this
// header's nested-alias detection family.

using ::djinterp::has_lock_policy_type;
using ::djinterp::has_atomic_size_type;
using ::djinterp::has_atomic_version_type;

// has_mutex_type
//   alias: presence-detection for a `mutex_type` nested
// alias.  Same predicate as upstream
// `djinterp::has_mutex_type_alias`, exposed here under
// a shorter name.
template<typename _Type>
using has_mutex_type = ::djinterp::has_mutex_type_alias<_Type>;


///////////////////////////////////////////////////////////////////////////////
///                III. TEST-SPECIFIC NESTED TYPE DETECTION                 ///
///////////////////////////////////////////////////////////////////////////////
//   These probes have no upstream counterpart.

// has_thread_safety_level
//   trait: true if _Type exposes a `thread_safety_level`
// nested alias.  Distinct from upstream
// `container_thread_safety_level<T>` which is a value
// extractor, not a presence probe.
template<typename _Type,
         typename = void>
struct has_thread_safety_level : std::false_type
{};

template<typename _Type>
struct has_thread_safety_level<_Type, void_t<
    typename _Type::thread_safety_level
>> : std::true_type
{};


///////////////////////////////////////////////////////////////////////////////
///                IV.  TEST-SPECIFIC LOCK INTERFACE DETECTION              ///
///////////////////////////////////////////////////////////////////////////////
//   read_lock / write_lock are aliased from upstream;
// these add try_lock and the composite.

// has_try_lock_method
//   trait: true if _Type exposes try_lock() returning an
// optional / pointer guard.
template<typename _Type,
         typename = void>
struct has_try_lock_method : std::false_type
{};

template<typename _Type>
struct has_try_lock_method<_Type, void_t<
    decltype(std::declval<_Type&>().try_lock())
>> : std::true_type
{};

// has_full_lock_interface
//   trait: composite - both read_lock() and write_lock()
// are present.
template<typename _Type>
struct has_full_lock_interface
{
    static D_CONSTEXPR bool value =
        ( has_read_lock_method<_Type>::value &&
          has_write_lock_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST-SPECIFIC SNAPSHOT / COW DETECTION              ///
///////////////////////////////////////////////////////////////////////////////
//   snapshot() is aliased from upstream; this adds
// publish() and the composite.

// has_publish_method
//   trait: true if _Type exposes publish(...) for COW-style
// state replacement.
template<typename _Type,
         typename = void>
struct has_publish_method : std::false_type
{};

template<typename _Type>
struct has_publish_method<_Type, void_t<
    decltype(std::declval<_Type&>().publish())
>> : std::true_type
{};

// has_cow_interface
//   trait: composite - snapshot() and publish() both
// present, indicating COW-style lifecycle.
template<typename _Type>
struct has_cow_interface
{
    static D_CONSTEXPR bool value =
        ( has_snapshot_method<_Type>::value &&
          has_publish_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VI.  TEST-SPECIFIC ATOMIC INTERFACE DETECTION            ///
///////////////////////////////////////////////////////////////////////////////
//   Upstream provides has_atomic_load_at (load by index);
// these probe the unindexed atomic interface used by
// scalar atomic wrappers.

// has_atomic_load_method
//   trait: true if _Type exposes load() as a const member.
template<typename _Type,
         typename = void>
struct has_atomic_load_method : std::false_type
{};

template<typename _Type>
struct has_atomic_load_method<_Type, void_t<
    decltype(std::declval<const _Type&>().load())
>> : std::true_type
{};

// has_atomic_store_method
//   trait: true if _Type exposes store(value) as a mutable
// member.
template<typename _Type,
         typename = void>
struct has_atomic_store_method : std::false_type
{};

template<typename _Type>
struct has_atomic_store_method<_Type, void_t<
    decltype(std::declval<_Type&>().store(
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_atomic_compare_exchange_method
//   trait: true if _Type exposes
// compare_exchange_weak(expected, desired) as a mutable
// member.
template<typename _Type,
         typename = void>
struct has_atomic_compare_exchange_method : std::false_type
{};

template<typename _Type>
struct has_atomic_compare_exchange_method<_Type, void_t<
    decltype(std::declval<_Type&>().compare_exchange_weak(
        std::declval<typename _Type::value_type&>(),
        std::declval<typename _Type::value_type>()))
>> : std::true_type
{};

// has_atomic_interface
//   trait: composite - load() and store() both present.
template<typename _Type>
struct has_atomic_interface
{
    static D_CONSTEXPR bool value =
        ( has_atomic_load_method<_Type>::value &&
          has_atomic_store_method<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VII. STRATEGY-LEVEL COMPOSITES                           ///
///////////////////////////////////////////////////////////////////////////////
//   Test-suite-friendly composites built on the aliased
// upstream probes plus the test-specific atomic probes.

// is_locked_testable
//   trait: true if _Type appears to use lock-based
// synchronization (lock_policy_type alias OR full
// read_lock/write_lock interface).
template<typename _Type>
struct is_locked_testable
{
    static D_CONSTEXPR bool value =
        ( has_lock_policy_type<_Type>::value     ||
          has_full_lock_interface<_Type>::value );
};

// is_cow_testable
//   trait: true if _Type appears to use copy-on-write
// (cow_state_type alias OR snapshot()/publish() interface).
template<typename _Type>
struct is_cow_testable
{
    static D_CONSTEXPR bool value =
        ( has_cow_state_type<_Type>::value      ||
          has_cow_interface<_Type>::value );
};

// is_rcu_testable
//   trait: true if _Type exposes RCU markers.
template<typename _Type>
struct is_rcu_testable
{
    static D_CONSTEXPR bool value =
        has_rcu_protected_type<_Type>::value;
};

// is_hazard_testable
//   trait: true if _Type exposes hazard-pointer markers.
template<typename _Type>
struct is_hazard_testable
{
    static D_CONSTEXPR bool value =
        has_hazard_domain_type<_Type>::value;
};

// is_lock_free_testable
//   trait: true if _Type appears to be lock-free (atomic
// interface OR hazard / RCU strategy).
template<typename _Type>
struct is_lock_free_testable
{
    static D_CONSTEXPR bool value =
        ( has_atomic_interface<_Type>::value ||
          is_hazard_testable<_Type>::value   ||
          is_rcu_testable<_Type>::value );
};


///////////////////////////////////////////////////////////////////////////////
///                VIII. COMBINED CLASSIFICATION                            ///
///////////////////////////////////////////////////////////////////////////////

// is_threadsafe_testable
//   trait: true if _Type is recognized as threadsafe under
// any supported strategy.
template<typename _Type>
struct is_threadsafe_testable
{
    static D_CONSTEXPR bool value =
        ( is_locked_testable<_Type>::value          ||
          is_cow_testable<_Type>::value             ||
          is_rcu_testable<_Type>::value             ||
          is_hazard_testable<_Type>::value          ||
          has_atomic_interface<_Type>::value        ||
          has_concurrency_strategy_tag<_Type>::value ||
          has_thread_safety_level<_Type>::value );
};

// thread_test_class
//   struct: comprehensive structural classification of a
// type for the threadsafe testing harness.  Every flag
// is independent and may be queried in isolation.
template<typename _Type>
struct thread_test_class
{
    // nested type aliases
    static D_CONSTEXPR bool has_safety_level =
        has_thread_safety_level<_Type>::value;
    static D_CONSTEXPR bool has_strategy_tag =
        has_concurrency_strategy_tag<_Type>::value;
    static D_CONSTEXPR bool has_lock_policy =
        has_lock_policy_type<_Type>::value;
    static D_CONSTEXPR bool has_mutex =
        has_mutex_type<_Type>::value;
    static D_CONSTEXPR bool has_cow_state =
        has_cow_state_type<_Type>::value;
    static D_CONSTEXPR bool has_rcu_protected =
        has_rcu_protected_type<_Type>::value;
    static D_CONSTEXPR bool has_hazard_domain =
        has_hazard_domain_type<_Type>::value;

    // lock interface
    static D_CONSTEXPR bool has_read_lock =
        has_read_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_write_lock =
        has_write_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_try_lock =
        has_try_lock_method<_Type>::value;
    static D_CONSTEXPR bool has_full_lock =
        has_full_lock_interface<_Type>::value;

    // cow interface
    static D_CONSTEXPR bool has_snapshot =
        has_snapshot_method<_Type>::value;
    static D_CONSTEXPR bool has_publish =
        has_publish_method<_Type>::value;
    static D_CONSTEXPR bool has_cow =
        has_cow_interface<_Type>::value;

    // atomic interface
    static D_CONSTEXPR bool has_load =
        has_atomic_load_method<_Type>::value;
    static D_CONSTEXPR bool has_store =
        has_atomic_store_method<_Type>::value;
    static D_CONSTEXPR bool has_compare_exchange =
        has_atomic_compare_exchange_method<_Type>::value;
    static D_CONSTEXPR bool has_atomic =
        has_atomic_interface<_Type>::value;

    // strategy classification
    static D_CONSTEXPR bool is_locked =
        is_locked_testable<_Type>::value;
    static D_CONSTEXPR bool is_cow =
        is_cow_testable<_Type>::value;
    static D_CONSTEXPR bool is_rcu =
        is_rcu_testable<_Type>::value;
    static D_CONSTEXPR bool is_hazard =
        is_hazard_testable<_Type>::value;
    static D_CONSTEXPR bool is_lock_free =
        is_lock_free_testable<_Type>::value;
    static D_CONSTEXPR bool is_threadsafe =
        is_threadsafe_testable<_Type>::value;
};


///////////////////////////////////////////////////////////////////////////////
///                IX.  VARIABLE TEMPLATES                                  ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // re-exported _v variants from djinterp::traits
    using ::djinterp::has_concurrency_strategy_tag_v;
    using ::djinterp::has_read_lock_method_v;
    using ::djinterp::has_write_lock_method_v;
    using ::djinterp::has_snapshot_method_v;
    using ::djinterp::has_cow_state_type_v;
    using ::djinterp::has_rcu_protected_type_v;
    using ::djinterp::has_epoch_type_v;
    using ::djinterp::has_hazard_domain_type_v;
    using ::djinterp::has_atomic_load_at_v;

    // re-exported _v variants from djinterp
    using ::djinterp::has_lock_policy_type_v;
    using ::djinterp::has_atomic_size_type_v;
    using ::djinterp::has_atomic_version_type_v;

    // has_mutex_type_v: same predicate as upstream
    // djinterp::has_mutex_type_alias_v.
    template<typename _Type>
    D_CONSTEXPR bool has_mutex_type_v =
        ::djinterp::has_mutex_type_alias_v<_Type>;

    // test-specific
    template<typename _Type>
    D_CONSTEXPR bool has_thread_safety_level_v =
        has_thread_safety_level<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_try_lock_method_v =
        has_try_lock_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_full_lock_interface_v =
        has_full_lock_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_publish_method_v =
        has_publish_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_cow_interface_v =
        has_cow_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_load_method_v =
        has_atomic_load_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_store_method_v =
        has_atomic_store_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_compare_exchange_method_v =
        has_atomic_compare_exchange_method<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool has_atomic_interface_v =
        has_atomic_interface<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_locked_testable_v =
        is_locked_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_cow_testable_v =
        is_cow_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_rcu_testable_v =
        is_rcu_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_hazard_testable_v =
        is_hazard_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_lock_free_testable_v =
        is_lock_free_testable<_Type>::value;

    template<typename _Type>
    D_CONSTEXPR bool is_threadsafe_testable_v =
        is_threadsafe_testable<_Type>::value;

#endif  // variable templates


NS_END  // traits
NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_THREAD_TRAITS_
