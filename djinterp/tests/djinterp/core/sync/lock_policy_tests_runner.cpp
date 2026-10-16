/*******************************************************************************
* djinterp [core]                                   lock_policy_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/lock_policy_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  lock_policy_tests_runner.cpp
//   Entry point for the lock_policy.hpp suite: registers every section's tests
//   with report_builder, one module per section.

// djinterp
#include "lock_policy_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- lock_policy.hpp Test Suite");
    rb.set_subtitle("sync : compile-time lock policies -- the level enum, the "
                    "policy structs, the no-op null policy, and the selector");
    rb.set_author("teer");
    rb.set_description(
        "lock_policy.hpp is compile-time vocabulary: a level enum, a family of "
        "policy structs that each name a mutex and its read/write lock types and "
        "carry a few boolean descriptors, a zero-cost no_op_mutex/no_op_guard "
        "pair for the null policy, and a metaprogramming SELECTOR mapping a level "
        "to the right policy. There is almost no run-time surface (only "
        "no_op_mutex's no-op operations), so the suite is overwhelmingly static: "
        "it asserts on type identities and compile-time constants with "
        "static_assert and returns the same fact at run time. It pins several "
        "sharp edges. The level enum is MISSPELLED -- its name is "
        "thread_safety_leavel (sic), and the policies' level members are typed by "
        "that misspelling; a correctly spelled alias, thread_safety_level, names "
        "the same type. atomic_only maps to null_lock_policy -- two DISTINCT "
        "levels (none and atomic_only) select the SAME policy, because lock-free "
        "atomic code needs no mutex. The selector FALLS BACK by standard: "
        "select_lock_policy<shared> is shared_lock_policy at C++17 and "
        "exclusive_lock_policy below it, and select_lock_policy<shared_timed> is "
        "shared_timed_lock_policy from C++14 and timed_lock_policy below it -- so "
        "the fallback tests check the mapping that holds for THIS build, not the "
        "C++17 answer. no_op_mutex::try_lock() always returns true (the null "
        "policy gives no real exclusion), and no_op_guard is NON-MOVABLE (deleted "
        "copy plus a declared destructor), which is the root cause of the "
        "lock_guard suite's finding that scoped_read_lock<null_lock_policy> is "
        "non-movable. Unlike cow.hpp and lock_guard.hpp, this header's 'include "
        "env.h first' guard is a LIVE #error and it does not include the "
        "environment itself, so it genuinely cannot stand first in a TU. The "
        "suite floors at C++11; every mention of shared_lock_policy is gated to "
        "C++17 and shared_timed_lock_policy to C++14, because below those "
        "standards those names DO NOT EXIST -- naming one is a hard error, which "
        "is exactly the trap the selector's fallbacks exist to avoid. Verified "
        "under C++11, C++14, C++17 and C++20. No threads: this is pure "
        "vocabulary, so the suite links no threading library.");

    rb.use_pdf("lock_policy_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "the guard, the LIVE env precondition, the internal-vs-public split, per-standard gating");
    rb.run("the include guard is defined, and is 1",       &tests_header_include_guard);
    rb.run("including it twice is a no-op",                &tests_header_is_idempotent);
    rb.run("it requires env.h first (a LIVE #error)",      &tests_header_requires_the_environment_first);
    rb.run("it supplies <mutex> and <shared_mutex>",       &tests_header_provides_the_std_facilities);
    rb.run("the policies live flat in djinterp",           &tests_entities_live_in_the_djinterp_namespace);
    rb.run("the selector impl lives in internal::",        &tests_the_selector_impl_lives_in_the_internal_namespace);
    rb.run("no stray macros",                              &tests_header_defines_no_stray_macros);
    rb.run("the policies are gated by standard",           &tests_the_policies_are_gated_by_standard);

    // -- I. the level enum -------------------------------------------------
    rb.module("I. thread_safety_leavel (the level enum)",
              "six ordered levels; a misspelled name with a corrected alias");
    rb.run("it has the six named values (0..5)",           &tests_level_has_the_six_named_values);
    rb.run("the ordering is a strict hierarchy",           &tests_level_ordering_is_a_strict_hierarchy);
    rb.run("it is a scoped enum over int",                 &tests_level_is_a_scoped_enum_over_int);
    rb.run("the corrected alias names the same enum",      &tests_the_correctly_spelled_alias_names_the_same_enum);
    rb.run("a level is usable in a constant expression",   &tests_level_is_usable_in_a_constant_expression);

    // -- II. the null policy -----------------------------------------------
    rb.module("II. null policy (no_op_mutex, no_op_guard, null_lock_policy)",
              "zero-cost scaffolding; try_lock always succeeds; guard non-movable");
    rb.run("no_op_mutex operations are callable",          &tests_no_op_mutex_operations_are_callable);
    rb.run("no_op_mutex try_lock ALWAYS succeeds",         &tests_no_op_mutex_try_lock_always_succeeds);
    rb.run("no_op_guard is constructible from the mutex",  &tests_no_op_guard_is_constructible_from_the_mutex);
    rb.run("no_op_guard is not copyable",                  &tests_no_op_guard_is_not_copyable);
    rb.run("no_op_guard is NOT movable (root cause)",      &tests_no_op_guard_is_not_movable);
    rb.run("null policy names the no-op types",            &tests_null_policy_aliases);
    rb.run("null policy read and write are identical",     &tests_null_policy_read_and_write_are_the_same_guard);
    rb.run("null policy descriptors are all false",        &tests_null_policy_descriptors_are_all_false);
    rb.run("null policy level is none",                    &tests_null_policy_level_is_none);

    // -- III. exclusive_lock_policy ----------------------------------------
    rb.module("III. exclusive_lock_policy",
              "std::mutex; read and write are the same exclusive lock");
    rb.run("it names std::mutex and unique_lock",          &tests_exclusive_aliases);
    rb.run("read and write locks are identical",           &tests_exclusive_read_and_write_are_identical);
    rb.run("descriptors: threadsafe, not shared/timed",    &tests_exclusive_descriptors);
    rb.run("its level is exclusive",                       &tests_exclusive_level_is_exclusive);

    // -- IV. timed_lock_policy ---------------------------------------------
    rb.module("IV. timed_lock_policy",
              "std::timed_mutex; exclusive, with timeout support");
    rb.run("it names std::timed_mutex and unique_lock",    &tests_timed_aliases);
    rb.run("read and write locks are identical",           &tests_timed_read_and_write_are_identical);
    rb.run("descriptors: threadsafe, timed, not shared",   &tests_timed_descriptors);
    rb.run("its level is timed (above exclusive)",         &tests_timed_level_is_timed);

    // -- V. shared_lock_policy ---------------------------------------------
    rb.module("V. shared_lock_policy (C++17+)",
              "std::shared_mutex; shared read, exclusive write");
    rb.run("it names shared_mutex, shared/unique locks",   &tests_shared_aliases);
    rb.run("read and write lock types DIFFER",             &tests_shared_read_and_write_differ);
    rb.run("descriptors: threadsafe, shared, not timed",   &tests_shared_descriptors);
    rb.run("its level is shared (above timed)",            &tests_shared_level_is_shared);
    rb.run("the read lock is a std::shared_lock",          &tests_shared_read_lock_is_a_shared_lock);

    // -- VI. shared_timed_lock_policy --------------------------------------
    rb.module("VI. shared_timed_lock_policy (C++14+)",
              "std::shared_timed_mutex; the most capable policy");
    rb.run("it names shared_timed_mutex and its locks",    &tests_shared_timed_aliases);
    rb.run("read and write lock types DIFFER",             &tests_shared_timed_read_and_write_differ);
    rb.run("descriptors: threadsafe, shared AND timed",    &tests_shared_timed_descriptors);
    rb.run("its level is shared_timed (the top)",          &tests_shared_timed_level_is_shared_timed);
    rb.run("it is the most capable (shared + timed)",      &tests_shared_timed_is_the_most_capable);

    // -- VII. the policy selector ------------------------------------------
    rb.module("VII. the policy selector",
              "level -> policy, with per-standard fallbacks");
    rb.run("none maps to the null policy",                 &tests_selector_none_maps_to_null);
    rb.run("atomic_only ALSO maps to null",                &tests_selector_atomic_only_also_maps_to_null);
    rb.run("exclusive maps to the exclusive policy",       &tests_selector_exclusive_maps_to_exclusive);
    rb.run("timed maps to the timed policy",               &tests_selector_timed_maps_to_timed);
    rb.run("shared maps correctly for this standard",      &tests_selector_shared_maps_correctly_for_this_standard);
    rb.run("shared_timed maps correctly for this standard", &tests_selector_shared_timed_maps_correctly_for_this_standard);
    rb.run("select_lock_policy aliases the impl",          &tests_select_lock_policy_is_the_public_alias_over_the_impl);
    rb.run("the selector resolves every level",            &tests_selector_resolves_every_level);

    // -- VIII. default_lock_policy -----------------------------------------
    rb.module("VIII. default_lock_policy",
              "the project default, and its invariant with the selector");
    rb.run("the default is right for this standard",       &tests_default_policy_for_this_standard);
    rb.run("the default is a real (threadsafe) policy",    &tests_default_is_a_real_policy);
    rb.run("the default matches select<its own level>",    &tests_default_matches_the_selector);

    return rb.finish();
}
