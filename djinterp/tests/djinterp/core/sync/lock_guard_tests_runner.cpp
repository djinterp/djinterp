/*******************************************************************************
* djinterp [core]                                    lock_guard_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_guard_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  lock_guard_tests_runner.cpp
//   Entry point for the lock_guard.hpp suite: registers every section's tests
//   with report_builder, one module per section.

// djinterp
#include "lock_guard_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- lock_guard.hpp Test Suite");
    rb.set_subtitle("sync : policy-aware RAII lock guards -- read, write, try, "
                    "timed, and read-to-write upgrade");
    rb.set_author("teer");
    rb.set_description(
        "lock_guard.hpp is the RAII layer over lock_policy.hpp: guards that take "
        "a policy's lock on construction and give it back on destruction, so "
        "call sites never name a concrete mutex or lock type. Each guard's whole "
        "contract is 'hold the right lock for exactly this scope', so the suite "
        "tests, for each, that it acquires the lock it claims (observed by trying "
        "to take the underlying mutex and failing) and releases it on scope exit "
        "(observed by then taking the mutex and succeeding). The genuinely "
        "concurrent claims get their own section, built from std::thread with a "
        "rendezvous so threads truly race: a write guard serializes writers to an "
        "exact counter total, a try guard and a timed guard exclude a contending "
        "thread (the try fails, the timed one times out after roughly its "
        "deadline), a read guard under a shared policy lets two readers overlap, "
        "and an upgrade_lock holds its write lock exclusively against another "
        "thread. The suite pins several sharp edges. scoped_read_lock and "
        "scoped_write_lock declare '= default' move members, but under "
        "null_lock_policy those moves are IMPLICITLY DELETED, because the guard's "
        "no_op_guard member is non-movable -- so a guard that moves fine for "
        "every real policy will not compile if moved under the null policy, a "
        "portability trap for generic code. scoped_try_lock and scoped_timed_lock "
        "are move-CONSTRUCTIBLE but not move-ASSIGNABLE, and a move transfers "
        "ownership so the moved-from guard does not unlock. The null policy gives "
        "NO real exclusion: no_op_mutex::try_lock() always returns true, so a "
        "scoped_try_lock<null_lock_policy> always owns the lock, even a second "
        "one. upgrade_lock is deliberately NOT atomic -- it unlocks the read lock "
        "and then takes the write lock, with a window in between where no lock is "
        "held -- and the suite pins that the end state is nonetheless an "
        "exclusive write lock. release() (on the try and timed guards) is "
        "idempotent. A NON-bug worth recording: lock_guard.hpp's own 'include "
        "env.h first' guards are commented out, exactly as cow.hpp's are, but it "
        "is harmless here because lock_guard.hpp includes ../djinterp.hpp BEFORE "
        "./lock_policy.hpp -- so unlike cow.hpp it is self-sufficient and can be "
        "the first djinterp header in a TU. The suite floors at C++11 (its "
        "concurrency section needs std::thread); scoped_timed_lock is exercised "
        "from C++11, and upgrade_lock and the shared-policy paths are gated to "
        "C++17. Verified under C++11, C++14, C++17 and C++20, ThreadSanitizer-"
        "clean.");

    rb.use_pdf("lock_guard_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "the guard, self-sufficiency despite commented-out env guards, per-standard gating");
    rb.run("the include guard is defined, and is 1",       &tests_header_include_guard);
    rb.run("including it twice is a no-op",                &tests_header_is_idempotent);
    rb.run("it is self-sufficient (can be included first)", &tests_header_is_self_sufficient);
    rb.run("it pulls its two djinterp dependencies",       &tests_header_pulls_its_dependencies);
    rb.run("it supplies <mutex> and <chrono>",             &tests_header_provides_the_std_facilities);
    rb.run("its env-first guards are commented out (inert)", &tests_header_precondition_guards_are_commented_out);
    rb.run("the guards live flat in djinterp",             &tests_entities_live_in_the_djinterp_namespace);
    rb.run("no stray macros",                              &tests_header_defines_no_stray_macros);
    rb.run("the guards are gated by standard",             &tests_the_guards_are_gated_by_standard);

    // -- I. scoped_read_lock -----------------------------------------------
    rb.module("I. scoped_read_lock",
              "a read lock for one scope; shared under a shared policy");
    rb.run("it republishes the policy's lock aliases",     &tests_read_exposes_its_policy_aliases);
    rb.run("it acquires the lock on construction",         &tests_read_lock_acquires_on_construction);
    rb.run("it releases the lock on destruction",          &tests_read_lock_releases_on_destruction);
    rb.run("a read guard is not copyable",                 &tests_read_lock_is_not_copyable);
    rb.run("it is movable for a real policy",              &tests_read_lock_is_movable_for_a_real_policy);
    rb.run("it is NOT movable for the null policy",        &tests_read_lock_is_not_movable_for_the_null_policy);
    rb.run("under the null policy it is a no-op",          &tests_read_lock_under_the_null_policy_is_a_no_op);
    rb.run("under a shared policy the read lock is shared", &tests_read_lock_is_shared_under_the_shared_policy);

    // -- II. scoped_write_lock ---------------------------------------------
    rb.module("II. scoped_write_lock",
              "an exclusive lock for one scope; exclusive under every policy");
    rb.run("it republishes the policy's lock aliases",     &tests_write_exposes_its_policy_aliases);
    rb.run("it acquires the lock exclusively",             &tests_write_lock_acquires_exclusively);
    rb.run("it releases the lock on destruction",          &tests_write_lock_releases_on_destruction);
    rb.run("a write guard is not copyable",                &tests_write_lock_is_not_copyable);
    rb.run("it is movable for a real policy",              &tests_write_lock_is_movable_for_a_real_policy);
    rb.run("it is NOT movable for the null policy",        &tests_write_lock_is_not_movable_for_the_null_policy);
    rb.run("it excludes readers even under a shared policy", &tests_write_lock_is_exclusive_even_under_the_shared_policy);
    rb.run("under the null policy it is a no-op",          &tests_write_lock_under_the_null_policy_is_a_no_op);

    // -- III. scoped_try_lock ----------------------------------------------
    rb.module("III. scoped_try_lock",
              "a non-blocking attempt; move-ctor-only, idempotent release");
    rb.run("it republishes the policy's mutex alias",      &tests_try_exposes_its_mutex_alias);
    rb.run("it acquires a free mutex",                     &tests_try_acquires_a_free_mutex);
    rb.run("it fails on a held mutex (no block)",          &tests_try_fails_on_a_held_mutex);
    rb.run("it releases on destruction",                   &tests_try_releases_on_destruction);
    rb.run("release() unlocks early",                      &tests_try_release_unlocks_early);
    rb.run("release() is idempotent",                      &tests_try_release_is_idempotent);
    rb.run("a try guard is not copyable",                  &tests_try_is_not_copyable);
    rb.run("it is move-constructible only",                &tests_try_is_move_constructible_only);
    rb.run("a move transfers ownership",                   &tests_try_move_transfers_ownership);
    rb.run("under the null policy it ALWAYS succeeds",     &tests_try_under_the_null_policy_always_succeeds);

    // -- IV. scoped_timed_lock ---------------------------------------------
    rb.module("IV. scoped_timed_lock",
              "an exclusive lock with a deadline (C++11+)");
    rb.run("it republishes the policy's mutex alias",      &tests_timed_exposes_its_mutex_alias);
    rb.run("it acquires a free mutex within its deadline", &tests_timed_acquires_a_free_mutex);
    rb.run("it releases on destruction",                   &tests_timed_releases_on_destruction);
    rb.run("release() unlocks early",                      &tests_timed_release_unlocks_early);
    rb.run("release() is idempotent",                      &tests_timed_release_is_idempotent);
    rb.run("a timed guard is not copyable",                &tests_timed_is_not_copyable);
    rb.run("it is move-constructible only",                &tests_timed_is_move_constructible_only);
    rb.run("a move transfers ownership",                   &tests_timed_move_transfers_ownership);

    // -- V. upgrade_lock ---------------------------------------------------
    rb.module("V. upgrade_lock",
              "read-to-write promotion, non-atomic by design (C++17+)");
    rb.run("it republishes the policy's aliases",          &tests_upgrade_exposes_its_aliases);
    rb.run("it acquires the write lock (exclusive)",       &tests_upgrade_acquires_the_write_lock);
    rb.run("it empties the source read lock",              &tests_upgrade_empties_the_source_read_lock);
    rb.run("downgrade() releases the write lock",          &tests_upgrade_downgrade_releases_the_write_lock);
    rb.run("it releases on destruction",                   &tests_upgrade_releases_on_destruction);
    rb.run("an upgrade guard is not copyable",             &tests_upgrade_is_not_copyable);
    rb.run("it is move-constructible",                     &tests_upgrade_is_move_constructible);

    // -- Concurrency -------------------------------------------------------
    rb.module("Concurrency",
              "what the locks are for -- exclusion, timeout, shared overlap, exclusive upgrade");
    rb.run("write guard serializes writers (exact total)", &tests_write_lock_serializes_writers);
    rb.run("read guard admits concurrent readers",         &tests_read_lock_admits_concurrent_readers);
    rb.run("try guard excludes a contending thread",       &tests_try_lock_excludes_a_contending_thread);
    rb.run("timed guard times out under contention",       &tests_timed_lock_times_out_under_contention);
    rb.run("upgrade holds the write lock exclusively",     &tests_upgrade_holds_the_write_lock_exclusively);

    return rb.finish();
}
