/*******************************************************************************
* djinterp [core]                                 lock_policy_c_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_c_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  lock_policy_c_tests_runner.cpp
//   Entry point for the lock_policy_c.hpp suite: registers every section's tests
//   with report_builder, one module per section.

// djinterp
#include "lock_policy_c_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- lock_policy_c.hpp Test Suite");
    rb.set_subtitle("sync : C-backed lock policies -- platform mutexes, an "
                    "rwlock, a recursive mutex, and an atomic spinlock");
    rb.set_author("teer");
    rb.set_description(
        "lock_policy_c.hpp is the C-backed sibling of lock_policy.hpp: its "
        "policies wrap the platform's own primitives -- pthreads on POSIX "
        "(pthread_mutex, a recursive pthread_mutex, pthread_rwlock) and "
        "CRITICAL_SECTION / SRWLOCK on Windows -- plus a std::atomic_flag "
        "spinlock. Unlike the base header, which is pure compile-time "
        "vocabulary, this one has real run-time behaviour, so the suite has two "
        "halves: static checks on the policy vocabulary (aliases, descriptors, "
        "levels) and genuine CONCURRENCY tests built from std::thread with a "
        "rendezvous. To stay portable the suite drives everything through the "
        "policy typedefs (c_exclusive::mutex_type and friends), which name the "
        "platform's type on any OS; a single platform-guarded test names the "
        "concrete pthread/Win32 classes. It pins several sharp edges. The guards "
        "c_lock_guard and c_shared_guard are NON-MOVABLE (deleted copy plus a "
        "declared destructor, the same trap no_op_guard springs), and the C "
        "mutex classes are non-copyable the C++98 way -- a private, undeclared "
        "copy constructor, not '= delete' -- though the suite checks the effect, "
        "not the mechanism. c_recursive is genuinely REENTRANT: a thread may lock "
        "or even try_lock its mutex while already holding it, which would "
        "deadlock a plain c_exclusive; the concurrency section shows recursion "
        "and cross-thread exclusion holding at once (nested guards on every "
        "increment, exact total). The spinlock is NOT reentrant: try_lock on a "
        "held spinlock returns false. THREE distinct policies -- c_exclusive, "
        "c_recursive and c_spinlock -- all report level 'exclusive' (the enum has "
        "no recursive or spin rung), so the level does not distinguish them; only "
        "behaviour does. c_shared is the one C policy whose read and write locks "
        "differ (a shared reader guard vs an exclusive writer guard), and its "
        "readers overlap while a writer excludes them. One banner-vs-code "
        "discrepancy is recorded: the versioning banner lists c_shared as a C++17 "
        "addition, but the code gates it only by platform, so on POSIX it is "
        "present from C++11 (verified) -- the spinlock, by contrast, really is "
        "C++11+. Unlike lock_policy.hpp, this header is self-sufficient: it "
        "includes the core and the base lock_policy itself, so it can be the "
        "first djinterp header in a TU. Verified under C++11, C++14, C++17 and "
        "C++20, ThreadSanitizer-clean.");

    rb.use_pdf("lock_policy_c_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "self-sufficiency, platform facilities, gating, and a banner-vs-code note");
    rb.run("the include guard is defined, and is 1",       &tests_header_include_guard);
    rb.run("including it twice is a no-op",                &tests_header_is_idempotent);
    rb.run("it is self-sufficient (can be included first)", &tests_header_is_self_sufficient);
    rb.run("it pulls in the base lock_policy",             &tests_header_pulls_in_the_base_lock_policy);
    rb.run("it provides the platform threading facilities", &tests_header_provides_the_platform_threading_facilities);
    rb.run("the policies live flat in djinterp",           &tests_entities_live_in_the_djinterp_namespace);
    rb.run("no stray macros (D_HAS_PTHREADS aside)",       &tests_header_defines_no_stray_macros);
    rb.run("the spinlock is gated to C++11",               &tests_the_spinlock_is_gated_to_cpp11);
    rb.run("c_shared is present at this standard",         &tests_c_shared_is_available_at_this_standard);

    // -- I. the platform mutex classes -------------------------------------
    rb.module("I. platform mutex classes",
              "the raw primitives; reentrancy is what sets recursive apart");
    rb.run("an exclusive mutex constructs and destructs",  &tests_exclusive_mutex_constructs_and_destructs);
    rb.run("it locks then unlocks",                        &tests_exclusive_mutex_lock_then_unlock);
    rb.run("try_lock succeeds on a free mutex",            &tests_exclusive_mutex_try_lock_when_free);
    rb.run("a C mutex is not copyable",                    &tests_exclusive_mutex_is_not_copyable);
    rb.run("a C mutex is not movable",                     &tests_exclusive_mutex_is_not_movable);
    rb.run("the recursive mutex allows reentrant lock",    &tests_recursive_mutex_allows_reentrant_locking);
    rb.run("recursive try_lock is reentrant too",          &tests_recursive_mutex_try_lock_is_reentrant);
    rb.run("the rwlock's shared ops are callable",         &tests_rwlock_shared_ops_are_callable);
    rb.run("rwlock write try_lock succeeds when free",     &tests_rwlock_try_lock_when_free);
    rb.run("the concrete classes back the policies",       &tests_the_concrete_classes_back_the_policies);

    // -- II. the C-backed RAII guards --------------------------------------
    rb.module("II. the C-backed RAII guards",
              "acquire/release; non-copyable and (the sharp edge) non-movable");
    rb.run("c_lock_guard acquires on construction",        &tests_lock_guard_acquires_on_construction);
    rb.run("c_lock_guard releases on destruction",         &tests_lock_guard_releases_on_destruction);
    rb.run("c_lock_guard is not copyable",                 &tests_lock_guard_is_not_copyable);
    rb.run("c_lock_guard is NOT movable",                  &tests_lock_guard_is_not_movable);
    rb.run("c_shared_guard acquires a shared lock",        &tests_shared_guard_acquires_shared_on_construction);
    rb.run("c_shared_guard releases on destruction",       &tests_shared_guard_releases_on_destruction);
    rb.run("c_shared_guard is not copyable",               &tests_shared_guard_is_not_copyable);
    rb.run("c_shared_guard is NOT movable",                &tests_shared_guard_is_not_movable);
    rb.run("the guards are generic over any C mutex",      &tests_the_guards_are_templates_over_any_c_mutex);

    // -- III. the C-backed policy structs ----------------------------------
    rb.module("III. the C-backed policy structs",
              "aliases, descriptors, levels; three policies share one level");
    rb.run("c_exclusive names a c_lock_guard",             &tests_c_exclusive_aliases);
    rb.run("c_exclusive read and write are identical",     &tests_c_exclusive_read_and_write_are_identical);
    rb.run("c_exclusive descriptors",                      &tests_c_exclusive_descriptors);
    rb.run("c_exclusive level is exclusive",               &tests_c_exclusive_level_is_exclusive);
    rb.run("c_shared names shared read + excl write",      &tests_c_shared_aliases);
    rb.run("c_shared read and write DIFFER",               &tests_c_shared_read_and_write_differ);
    rb.run("c_shared descriptors (is_shared true)",        &tests_c_shared_descriptors);
    rb.run("c_shared level is shared",                     &tests_c_shared_level_is_shared);
    rb.run("c_recursive aliases and descriptors",          &tests_c_recursive_aliases_and_descriptors);
    rb.run("c_recursive level is exclusive (no rec rung)", &tests_c_recursive_level_is_exclusive);
    rb.run("three policies share the exclusive level",     &tests_three_policies_share_the_exclusive_level);

    // -- IV. the spinlock --------------------------------------------------
    rb.module("IV. the spinlock (mutex + policy)",
              "atomic_flag test-and-set; noexcept; NOT reentrant (C++11+)");
    rb.run("spinlock constructs (starts unlocked)",        &tests_spinlock_mutex_constructs);
    rb.run("it locks then unlocks",                        &tests_spinlock_mutex_lock_then_unlock);
    rb.run("try_lock succeeds when free",                  &tests_spinlock_mutex_try_lock_when_free);
    rb.run("try_lock on a held spinlock FAILS (no re-entry)", &tests_spinlock_mutex_try_lock_when_held_fails);
    rb.run("its operations are noexcept",                  &tests_spinlock_mutex_operations_are_noexcept);
    rb.run("spinlock_mutex is not copyable",               &tests_spinlock_mutex_is_not_copyable);
    rb.run("spinlock_mutex is not movable",                &tests_spinlock_mutex_is_not_movable);
    rb.run("c_spinlock names spinlock_mutex + guard",      &tests_c_spinlock_aliases);
    rb.run("c_spinlock descriptors and level",             &tests_c_spinlock_descriptors_and_level);

    // -- Concurrency -------------------------------------------------------
    rb.module("Concurrency",
              "what the locks are for -- exclusion, reader overlap, recursion, spin");
    rb.run("c_exclusive serializes writers (exact total)", &tests_c_exclusive_serializes_writers);
    rb.run("c_shared admits concurrent readers",           &tests_c_shared_admits_concurrent_readers);
    rb.run("c_shared writer excludes readers",             &tests_c_shared_writer_excludes_readers);
    rb.run("c_recursive is exclusive across threads",      &tests_c_recursive_is_exclusive_across_threads);
    rb.run("c_spinlock serializes writers (exact total)",  &tests_c_spinlock_serializes_writers);
    rb.run("c_exclusive try_lock excludes a contender",    &tests_c_exclusive_try_lock_excludes_a_contending_thread);

    return rb.finish();
}
