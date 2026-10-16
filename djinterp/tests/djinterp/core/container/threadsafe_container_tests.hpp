/*******************************************************************************
* djinterp [test]                                 threadsafe_container_tests.hpp
*
*   DTest suite for /core/container/threadsafe_container.hpp - the runtime
* building blocks every threadsafe_<container> composes from: the CRTP mutex
* base, RAII locked accessors, atomic container state, the optimistic-read
* (seqlock) protocol, the CAS retry infrastructure, snapshot views, batch
* guards, and locked ranges.
*
*   Unlike a traits header, this module has real runtime behaviour, so the
* checks here CONSTRUCT objects, acquire and release locks, mutate under them,
* and assert on observed state.  Each tests_* returns true iff every check
* passed; a failing check prints its location and early-returns false through
* the D_TC_CHECK macro so the runner reports a pass/fail row rather than
* aborting the build.  The suite exercises the C++11-and-up path (the shipped
* path); shared-mutex fixtures are additionally gated for C++17.  The final
* block drives genuine multi-threaded contention with std::thread and asserts
* deterministic invariants (no lost updates, mutual exclusion), so it needs a
* threads-enabled build (the test helper links the thread runtime the same way
* the framework's own concurrency modules require).
*
* -----------------------------------------------------------------------------
* BUILD PREREQUISITE
* -----------------------------------------------------------------------------
*   The header under test pulls in the sync aggregate and the container traits;
* the suite compiles once these framework pieces are present in the tree (all
* ordinary headers, merely outside this component):
*
*     - /core/sync/threadsafe.hpp and everything it aggregates (lock_policy,
*       lock_guard, lock_policy_c, atomic, condvar, cow, hazard_pointer, rcu).
*     - /core/container/traits/threadsafe_container_traits.hpp and
*       container_traits.hpp (included by the module under test).
*
* -----------------------------------------------------------------------------
* PINNED BEHAVIOUR (guide 7.4 - reported, module NOT patched)
* -----------------------------------------------------------------------------
*   Layer 1 documents threadsafe_container_base as "movable: mutex is
* default-initialized in the moved-to object".  That intent is not met for a
* real-mutex policy: std::mutex is non-movable, and the base's
* `= default` move constructor is therefore defined as DELETED rather than
* fresh-constructing the mutex, so a container is move-constructible only under
* null_lock_policy (no_op_mutex is movable) and NOT under exclusive/shared
* policies.  tests_base_copy_move pins this as it stands; the atomic-state
* container tests_base_copy_move assertion for exclusive is expected to INVERT
* if the base is given a user-defined move constructor that value-initialises a
* fresh mutex instead of defaulting it.
*
*
* path:      /tests/djinterp/core/container/threadsafe_container_tests.hpp
* link(s):   TBA
* author(s): djinterp test generator                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_THREADSAFE_CONTAINER_TESTS_
#define DJINTERP_TESTS_THREADSAFE_CONTAINER_TESTS_ 1

// std
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>
// djinterp
// -- (part 1) mode-gated includes ------------------------------------------
// djinterp core is ALWAYS first and unconditional (NS_*, D_* qualifiers,
// language gates), so both faces have it.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/core/container/threadsafe_container.hpp"             // the header under test
#endif
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"      // module_spec + run_module
#endif


// ensure the ::djinterp::test namespace name exists for the unconditional dt
// alias below, even in the normal face where test_defaults.hpp is not pulled
// in (re-opening an existing namespace is a no-op in the spec face).
namespace djinterp { namespace test {} }


NS_DJINTERP
NS_TESTING

// dt names the entities under test (djinterp::test). Declared UNCONDITIONALLY,
// because the spec provider (spec mode) needs dt::module_spec too.
namespace dt = ::djinterp::test;


// D_TC_CHECK
//   check: evaluate the (possibly comma-bearing) expression once; on failure
// print it with its location and early-return false.  Variadic so a top-level
// comma in a template argument passes through whole.  Unique two-letter suffix
// (TC) so co-compiled suites never collide.
#define D_TC_CHECK(...)                                                    \
    do                                                                    \
    {                                                                     \
        if (!(__VA_ARGS__))                                               \
        {                                                                 \
            std::printf("    check failed: %s\n    at: %s:%d\n",          \
                        #__VA_ARGS__, __FILE__, __LINE__);                \
            return false;                                                 \
        }                                                                 \
    }                                                                     \
    while (0)


#ifndef DTEST_SPEC_MODE  // (part 1 cont.) fixtures - normal mode only

// -- policy shorthands ------------------------------------------------------
using tc_null = null_lock_policy;        // no-op mutex (single agent)
using tc_excl = exclusive_lock_policy;   // std::mutex
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
using tc_shared = shared_lock_policy;    // std::shared_mutex (reader/writer)
#endif

// -- tc_vec ----------------------------------------------------------------
// a minimal threadsafe container over a std::vector<int>.  Derives from the
// CRTP base so it carries the mutex + policy machinery, and exposes the UNSAFE
// payload operations (to be called under a lock in the tests) plus the const
// iteration surface snapshot_view / locked_range consume.
template<typename _Policy>
class tc_vec
    : public threadsafe_container_base<tc_vec<_Policy>, _Policy>
{
public:
    using value_type = int;

    tc_vec() = default;

    void push_(int _v) { m_data.push_back(_v); }
    void clear_()      { m_data.clear(); }

    std::size_t size() const { return m_data.size(); }

    const int& operator[](std::size_t _i) const { return m_data[_i]; }

    std::vector<int>::const_iterator begin() const { return m_data.begin(); }
    std::vector<int>::const_iterator end()   const { return m_data.end(); }

private:
    std::vector<int> m_data;
};

// -- tc_counter ------------------------------------------------------------
// a threadsafe container holding a PLAIN (non-atomic) counter, mutated only
// under the write lock.  Used to witness mutual exclusion: under a real policy
// the write lock serialises the increments, so the total is exact.
template<typename _Policy>
class tc_counter
    : public threadsafe_container_base<tc_counter<_Policy>, _Policy>
{
public:
    tc_counter() : m_n(0) {}

    void bump_()       { ++m_n; }
    long value() const { return m_n; }

private:
    long m_n;
};

#endif  // !DTEST_SPEC_MODE  (fixtures)


// -- (part 2) declarations - visible in BOTH modes -------------------------
// One flat list, grouped by section; each banner names the .cpp that defines
// the group.

// I.    base            (threadsafe_container_tests_base.cpp)
bool tests_base_types_and_tag();
bool tests_base_lock_acquisition();
bool tests_base_try_write_lock();
bool tests_base_policy_queries_null();
bool tests_base_policy_queries_locking();
bool tests_base_monograph_queries();
bool tests_base_copy_move();

// II.   locked_ref      (threadsafe_container_tests_locked_ref.cpp)
bool tests_locked_ref_write();
bool tests_const_locked_ref_read();
bool tests_locked_ref_move_noncopy();
bool tests_locked_apply();

// III.  atomic_state    (threadsafe_container_tests_atomic_state.cpp)
bool tests_atomic_state_size();
bool tests_atomic_state_version();
bool tests_atomic_state_snapshot();

// IV.   optimistic      (threadsafe_container_tests_optimistic.cpp)
bool tests_optimistic_read_fast_path();
bool tests_optimistic_read_fallback();

// V.    cas             (threadsafe_container_tests_cas.cpp)
bool tests_cas_exponential_off();
bool tests_cas_loop_success();
bool tests_cas_loop_update();

// VI.   snapshot_view   (threadsafe_container_tests_snapshot_view.cpp)
bool tests_snapshot_view_copy();
bool tests_snapshot_view_independent();
bool tests_snapshot_view_empty();

// VII.  batch_guard     (threadsafe_container_tests_batch_guard.cpp)
bool tests_batch_guard_count();
bool tests_batch_guard_raii();

// VIII. locked_range    (threadsafe_container_tests_locked_range.cpp)
bool tests_locked_range_iterate();
bool tests_locked_range_empty();

// integration           (threadsafe_container_tests_concurrent.cpp)
bool tests_concurrent_atomic_size();
bool tests_concurrent_cas_loop();
bool tests_concurrent_write_lock();


// -- (part 3) the spec provider - spec mode only ---------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
threadsafe_container_spec()
{
    return dt::module_spec{
        "threadsafe_container",
        "runtime building blocks for threadsafe containers: the CRTP mutex "
        "base, locked accessors, atomic state, optimistic reads, CAS retry, "
        "snapshot views, batch guards, and locked ranges",
        {
            dt::block_spec{
                "base",
                "threadsafe_container_base: types, lock acquisition, the "
                "constexpr policy and monograph queries, and copy/move",
                {
                    { "tests_base_types_and_tag",
                      "the exposed aliases and the locked "
                      "concurrency-strategy tag are what the base declares",
                      &tests_base_types_and_tag },
                    { "tests_base_lock_acquisition",
                      "read_lock / write_lock / mutex return usable handles "
                      "and release cleanly on scope exit",
                      &tests_base_lock_acquisition },
                    { "tests_base_try_write_lock",
                      "try_write_lock acquires uncontended under both the "
                      "null and the exclusive policy",
                      &tests_base_try_write_lock },
                    { "tests_base_policy_queries_null",
                      "under null_lock_policy the container is not "
                      "threadsafe, not shared, not timed, level none",
                      &tests_base_policy_queries_null },
                    { "tests_base_policy_queries_locking",
                      "exclusive and shared policies report threadsafe with "
                      "the expected shared/timed flags and level",
                      &tests_base_policy_queries_locking },
                    { "tests_base_monograph_queries",
                      "is_blocking / permits_concurrent_readers track the "
                      "policy flags; snapshot and individual reclamation are "
                      "always on",
                      &tests_base_monograph_queries },
                    { "tests_base_copy_move",
                      "the container is never copyable, movable under the "
                      "null policy, and (pinned) NOT movable under a real "
                      "mutex",
                      &tests_base_copy_move },
                }
            },
            dt::block_spec{
                "locked_ref",
                "the RAII locked accessors and the locked_apply helpers",
                {
                    { "tests_locked_ref_write",
                      "locked_ref grants mutable access via operator-> / "
                      "operator* / get and its writes are observed",
                      &tests_locked_ref_write },
                    { "tests_const_locked_ref_read",
                      "const_locked_ref grants const access and reads back "
                      "the container contents",
                      &tests_const_locked_ref_read },
                    { "tests_locked_ref_move_noncopy",
                      "locked_ref is move-constructible and non-copyable",
                      &tests_locked_ref_move_noncopy },
                    { "tests_locked_apply",
                      "locked_apply and locked_apply_read invoke the callable "
                      "under a lock and return its result",
                      &tests_locked_apply },
                }
            },
            dt::block_spec{
                "atomic_state",
                "the bundled atomic size + version metadata",
                {
                    { "tests_atomic_state_size",
                      "load / store / fetch_add / fetch_sub on the size field "
                      "with the documented return values",
                      &tests_atomic_state_size },
                    { "tests_atomic_state_version",
                      "load and increment on the version field return the "
                      "prior value and advance by one",
                      &tests_atomic_state_version },
                    { "tests_atomic_state_snapshot",
                      "take_snapshot captures size+version; validate_snapshot "
                      "is true until the version advances, then false",
                      &tests_atomic_state_snapshot },
                }
            },
            dt::block_spec{
                "optimistic",
                "the optimistic-read (seqlock) protocol",
                {
                    { "tests_optimistic_read_fast_path",
                      "with no interleaved writer the read validates on the "
                      "first attempt and returns the callable's result",
                      &tests_optimistic_read_fast_path },
                    { "tests_optimistic_read_fallback",
                      "a callable that bumps the version each call exhausts "
                      "the retries, falls back to the read lock, and still "
                      "returns",
                      &tests_optimistic_read_fallback },
                }
            },
            dt::block_spec{
                "cas",
                "the CAS retry infrastructure: exponential backoff and "
                "cas_loop",
                {
                    { "tests_cas_exponential_off",
                      "exponential_off drives its spin-then-yield schedule "
                      "and reset without error across many calls",
                      &tests_cas_exponential_off },
                    { "tests_cas_loop_success",
                      "an uncontended cas_loop succeeds and lands the value "
                      "the update function computed",
                      &tests_cas_loop_success },
                    { "tests_cas_loop_update",
                      "the update function receives the current value and the "
                      "result reflects the transformation it applied",
                      &tests_cas_loop_update },
                }
            },
            dt::block_spec{
                "snapshot_view",
                "the read-locked copy that iterates without holding the lock",
                {
                    { "tests_snapshot_view_copy",
                      "the view copies the elements and exposes size / empty "
                      "/ operator[] / iteration / data over the copy",
                      &tests_snapshot_view_copy },
                    { "tests_snapshot_view_independent",
                      "mutating the source after the snapshot does not change "
                      "the view (it is an independent copy)",
                      &tests_snapshot_view_independent },
                    { "tests_snapshot_view_empty",
                      "a snapshot of an empty container is empty with size "
                      "zero",
                      &tests_snapshot_view_empty },
                }
            },
            dt::block_spec{
                "batch_guard",
                "the write-lock-held multi-operation transaction guard",
                {
                    { "tests_batch_guard_count",
                      "record advances the operation count and count reports "
                      "it",
                      &tests_batch_guard_count },
                    { "tests_batch_guard_raii",
                      "a batch_guard acquires and releases the write lock "
                      "over a real mutex without deadlocking",
                      &tests_batch_guard_raii },
                }
            },
            dt::block_spec{
                "locked_range",
                "the read-lock-held iteration range",
                {
                    { "tests_locked_range_iterate",
                      "the range exposes size and begin/end over the "
                      "container while holding the read lock",
                      &tests_locked_range_iterate },
                    { "tests_locked_range_empty",
                      "an empty container yields a range of size zero whose "
                      "begin equals end",
                      &tests_locked_range_empty },
                }
            },
            dt::block_spec{
                "concurrent",
                "genuine multi-threaded contention with deterministic "
                "invariants (std::thread)",
                {
                    { "tests_concurrent_atomic_size",
                      "N threads each fetch_add the size M times land exactly "
                      "N*M, and fetch_sub returns to zero (atomic, no loss)",
                      &tests_concurrent_atomic_size },
                    { "tests_concurrent_cas_loop",
                      "N threads each cas_loop-increment M times land exactly "
                      "N*M (no lost updates)",
                      &tests_concurrent_cas_loop },
                    { "tests_concurrent_write_lock",
                      "under the exclusive policy N threads incrementing a "
                      "plain counter under write_lock land exactly N*M "
                      "(exclusion)",
                      &tests_concurrent_write_lock },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_THREADSAFE_CONTAINER_TESTS_
