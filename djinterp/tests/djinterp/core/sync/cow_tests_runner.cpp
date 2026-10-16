/*******************************************************************************
* djinterp [core]                                           cow_tests_runner.cpp
*
*
* path:      /tests/djinterp/core/sync/cow_tests_runner.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test]  cow_tests_runner.cpp
//   Entry point for the cow.hpp suite: registers every section's tests with
//   report_builder, one module per section.

// djinterp
#include "cow_tests.hpp"
#include "../../../../inc/djinterp/test/output/test_report_runner.hpp"


int
main()
{
    using namespace djinterp::testing;
    djinterp::test::report_builder rb;

    rb.set_title("djinterp -- cow.hpp Test Suite");
    rb.set_subtitle("sync : copy-on-write and immutable snapshots -- cow_ptr, "
                    "immutable_snapshot, cow_state");
    rb.set_author("teer");
    rb.set_description(
        "Copy-on-write is a claim about WHEN A COPY HAPPENS, and a value read "
        "back cannot tell you that -- the value is the same whether or not a "
        "clone occurred. So the suite instruments the payload: a tracked type "
        "counts every construction, copy, move and destruction, and the tests "
        "assert on those counts and on object ADDRESSES rather than on values. "
        "That is how 'read() never clones', 'write() clones exactly once and only "
        "when shared', 'snapshot() is zero-copy', and 'nothing leaks' become "
        "measurements instead of hopes. The suite pins several sharp edges the "
        "header does not mention. cow_ptr's read(), write(), operator* and "
        "operator-> DEREFERENCE AN UNCHECKED POINTER, so on an empty cow_ptr all "
        "four are undefined behaviour -- the suite exercises the null-safe "
        "queries and never dereferences an empty one. cow_ptr<T>::write() and "
        "detach() instantiate a copy of the payload whether or not a clone is "
        "needed, so a move-only T compiles for make() and the readers and stops "
        "at the first write(). And it reports two genuine bugs. cow.hpp cannot be "
        "the first djinterp header in a translation unit: its first include is "
        "lock_policy.hpp, which #errors without env.h, and cow.hpp never includes "
        "the core that would define it -- one missing line that condvar.hpp, next "
        "door, gets right. cow_state::write() and modify() BUMP THE VERSION "
        "BEFORE they clone, so when the clone throws the value is unchanged but "
        "the version has still advanced, and every reader watching it believes in "
        "a mutation that never happened -- while replace(), five lines away, "
        "bumps AFTER and is strongly safe. The exception-safety section shows "
        "four of the five clone sites are strongly safe and this one is not; the "
        "concurrency section drives real threads through a deterministic "
        "rendezvous -- readers that genuinely overlap a writer, never a race that "
        "happens to pass -- and checks the refcount comes out exact and nothing "
        "leaks. Verified under C++11, C++14, C++17 and C++20.");

    rb.use_pdf("cow_tests_report.pdf");

    // -- 0. the header contract --------------------------------------------
    rb.module("0. the header contract",
              "the guard, the include order (a bug), the dependencies, the dead gates");
    rb.run("the include guard is defined, and is 1",   &tests_header_include_guard);
    rb.run("including it twice is a no-op",            &tests_header_is_idempotent);
    rb.run("it CANNOT be included first (a bug)",      &tests_header_cannot_be_included_first);
    rb.run("its four djinterp dependencies are in",    &tests_header_pulls_its_dependencies);
    rb.run("lock_guard is included but never used",    &tests_header_takes_lock_guard_but_never_uses_it);
    rb.run("its guards and C++11 gate are commented out", &tests_header_gates_are_commented_out);
    rb.run("it supplies <cstdint> and <mutex>",        &tests_header_provides_the_std_facilities);
    rb.run("public in djinterp, block in internal",    &tests_entities_live_in_the_djinterp_namespace);
    rb.run("no stray macros",                          &tests_header_defines_no_stray_macros);

    // -- I. the control block ----------------------------------------------
    rb.module("I. the intrusive control block",
              "one allocation: the refcount and the value together");
    rb.run("born at a refcount of one",               &tests_control_block_starts_at_one);
    rb.run("add_ref raises the count",                &tests_control_block_add_ref);
    rb.run("release reports the LAST reference",      &tests_control_block_release_reports_the_last_reference);
    rb.run("is_unique is 'count == 1'",               &tests_control_block_is_unique);
    rb.run("the refcount ops are noexcept",           &tests_control_block_refcount_ops_are_noexcept);
    rb.run("the value lives INSIDE the block",        &tests_control_block_holds_the_value_inline);
    rb.run("the variadic ctor builds in place",       &tests_control_block_constructs_the_value_in_place);
    rb.run("an lvalue is copied",                     &tests_control_block_copies_an_lvalue);
    rb.run("an rvalue is moved",                      &tests_control_block_moves_an_rvalue);
    rb.run("it is an internal detail (re-exported)",  &tests_control_block_lives_in_internal);

    // -- II. cow_ptr - lifetime --------------------------------------------
    rb.module("II. cow_ptr -- lifetime",
              "who owns the block, and when it dies");
    rb.run("the default is empty",                    &tests_cow_ptr_default_is_empty);
    rb.run("make() starts unique",                    &tests_cow_ptr_make_starts_unique);
    rb.run("make() constructs in place",              &tests_cow_ptr_make_constructs_in_place);
    rb.run("make() copies an lvalue",                 &tests_cow_ptr_make_copies_an_lvalue);
    rb.run("make() moves an rvalue",                  &tests_cow_ptr_make_moves_an_rvalue);
    rb.run("a copy SHARES the block",                 &tests_cow_ptr_copy_shares_the_block);
    rb.run("a move STEALS the block",                 &tests_cow_ptr_move_steals_the_block);
    rb.run("copy assignment shares",                  &tests_cow_ptr_copy_assignment_shares);
    rb.run("move assignment steals",                  &tests_cow_ptr_move_assignment_steals);
    rb.run("self-assignment is safe",                 &tests_cow_ptr_self_assignment_is_safe);
    rb.run("assigning an ALIASED block is safe",      &tests_cow_ptr_assigning_an_aliased_block_is_safe);
    rb.run("the destructor releases the last ref",    &tests_cow_ptr_destructor_releases_the_last_reference);
    rb.run("the empty pointer's safe half works",     &tests_cow_ptr_empty_operations_are_safe);
    rb.run("sharing is noexcept, writing is not",     &tests_cow_ptr_lifetime_operations_are_noexcept);

    // -- II. cow_ptr - access and the clone --------------------------------
    rb.module("II. cow_ptr -- access and the clone",
              "read() never clones; write() clones iff shared -- THE type");
    rb.run("read() returns the value (const ref)",    &tests_cow_ptr_read_returns_the_value);
    rb.run("operator-> and operator* reach it",       &tests_cow_ptr_arrow_and_star_reach_the_value);
    rb.run("const access NEVER clones",               &tests_cow_ptr_const_access_never_clones);
    rb.run("write when unique does NOT clone",        &tests_cow_ptr_write_when_unique_does_not_clone);
    rb.run("write when shared CLONES",                &tests_cow_ptr_write_when_shared_clones);
    rb.run("the write leaves other holders alone",    &tests_cow_ptr_write_leaves_the_other_holder_untouched);
    rb.run("it clones exactly once (then in place)",  &tests_cow_ptr_write_clones_exactly_once);
    rb.run("the clone is one copy construction",      &tests_cow_ptr_write_copies_the_value_exactly_once);
    rb.run("is_unique and use_count",                 &tests_cow_ptr_is_unique_and_use_count);
    rb.run("operator bool (explicit)",                &tests_cow_ptr_operator_bool);
    rb.run("the empty pointer must NOT be deref'd",   &tests_the_empty_cow_ptr_must_not_be_dereferenced);
    rb.run("it carries a move-only payload",          &tests_cow_ptr_carries_a_move_only_payload);

    // -- II. cow_ptr - detach and swap -------------------------------------
    rb.module("II. cow_ptr -- detach and swap",
              "detach clones even when unique; swap touches no refcount");
    rb.run("detach clones even when UNIQUE",          &tests_detach_clones_even_when_unique);
    rb.run("...at a cost of one copy",                &tests_detach_copies_the_value_exactly_once);
    rb.run("detach when shared leaves others alone",  &tests_detach_when_shared_leaves_the_others_alone);
    rb.run("detach on empty is a no-op",              &tests_detach_on_an_empty_pointer_is_a_no_op);
    rb.run("member swap exchanges the blocks",        &tests_member_swap_exchanges_the_blocks);
    rb.run("free swap is found by ADL",               &tests_free_swap_is_found_by_adl);
    rb.run("swap touches no refcounts",               &tests_swap_touches_no_refcounts);
    rb.run("both swaps are noexcept",                 &tests_swap_is_noexcept);

    // -- III. immutable_snapshot -------------------------------------------
    rb.module("III. immutable_snapshot",
              "a value ctor COPIES, a cow_ptr ctor SHARES -- and they look alike");
    rb.run("from a value: it COPIES",                 &tests_snapshot_from_a_value_copies_it);
    rb.run("from a cow_ptr: ZERO copies",             &tests_snapshot_from_a_cow_ptr_is_zero_copy);
    rb.run("the version defaults to zero",            &tests_snapshot_version_defaults_to_zero);
    rb.run("...or carries the stamp it was given",    &tests_snapshot_carries_the_version_it_was_given);
    rb.run("get(), operator-> and operator*",         &tests_snapshot_get_arrow_and_star);
    rb.run("only CONST access to the data",           &tests_snapshot_exposes_only_const_access);
    rb.run("a copy shares the block",                 &tests_snapshot_copy_shares_the_block);
    rb.run("a move steals it (source -> empty)",      &tests_snapshot_move_steals_the_block);
    rb.run("it is NOT default-constructible",         &tests_snapshot_is_not_default_constructible);
    rb.run("it OUTLIVES its source",                  &tests_snapshot_outlives_its_source);
    rb.run("from an empty cow_ptr: falsy",            &tests_snapshot_from_an_empty_cow_ptr_is_falsy);
    rb.run("use_count reports the block's",           &tests_snapshot_use_count);

    // -- IV. cow_state - basics --------------------------------------------
    rb.module("IV. cow_state -- basics",
              "the shape: aliases, non-copy/non-move, the strategy tag");
    rb.run("it default-constructs its value",         &tests_state_default_constructs_its_value);
    rb.run("from an lvalue (copies, explicit)",       &tests_state_constructs_from_an_lvalue);
    rb.run("from an rvalue (moves)",                  &tests_state_constructs_from_an_rvalue);
    rb.run("not copyable (holds a mutex)",            &tests_state_is_not_copyable);
    rb.run("...and NOT movable either",               &tests_state_is_not_movable);
    rb.run("it publishes its policy aliases",         &tests_state_exposes_its_policy_aliases);
    rb.run("it self-marks with cow_state_type",       &tests_state_marks_itself_with_cow_state_type);
    rb.run("it declares the COW strategy tag",        &tests_state_declares_the_cow_strategy_tag);
    rb.run("read() returns the value",                &tests_state_read_returns_the_value);
    rb.run("the version starts at zero",              &tests_state_version_starts_at_zero);
    rb.run("it exposes its (mutable) mutex",          &tests_state_exposes_its_mutex);
    rb.run("cow_state<T> defaults its policy",        &tests_state_defaults_to_the_default_policy);

    // -- IV. cow_state - the protocol --------------------------------------
    rb.module("IV. cow_state -- the protocol",
              "snapshot then write clones; the reference hazard at the centre");
    rb.run("write mutates and bumps the version",     &tests_state_write_mutates_and_bumps_the_version);
    rb.run("write without a snapshot does not clone", &tests_state_write_without_a_snapshot_does_not_clone);
    rb.run("snapshot shares the block",               &tests_state_snapshot_shares_the_block);
    rb.run("write AFTER a snapshot CLONES",           &tests_state_write_after_a_snapshot_clones);
    rb.run("the snapshot is isolated from writes",    &tests_state_snapshot_is_isolated_from_later_writes);
    rb.run("the snapshot pins its version",           &tests_state_snapshot_pins_its_version);
    rb.run("modify runs under the write lock",        &tests_state_modify_runs_under_the_write_lock);
    rb.run("modify returns the callable's result",    &tests_state_modify_returns_the_callables_result);
    rb.run("modify accepts a void callable",          &tests_state_modify_accepts_a_void_callable);
    rb.run("modify bumps the version",                &tests_state_modify_bumps_the_version);
    rb.run("replace swaps the whole value",           &tests_state_replace_swaps_the_whole_value);
    rb.run("replace leaves old snapshots valid",      &tests_state_replace_leaves_old_snapshots_valid);
    rb.run("read/write return UNGUARDED references",  &tests_state_read_and_write_return_unguarded_references);

    // -- IV. cow_state - the lock policies ---------------------------------
    rb.module("IV. cow_state -- the lock policies",
              "the same source, locked or not, exclusive or shared");
    rb.run("under the null policy (no mutex)",        &tests_state_under_the_null_policy);
    rb.run("under the exclusive policy",              &tests_state_under_the_exclusive_policy);
    rb.run("under the timed policy",                  &tests_state_under_the_timed_policy);
    rb.run("under the shared policy",                 &tests_state_under_the_shared_policy);
    rb.run("under the shared_timed policy",           &tests_state_under_the_shared_timed_policy);
    rb.run("write excludes other agents",             &tests_state_write_excludes_other_agents);
    rb.run("the null policy costs no mutex",          &tests_state_null_policy_costs_no_mutex);
    rb.run("every policy yields a usable state",      &tests_every_policy_yields_a_usable_state);

    // -- exception safety --------------------------------------------------
    rb.module("Exception safety",
              "what survives a throwing clone -- four sites strong, one not");
    rb.run("make propagates a throwing ctor",         &tests_make_propagates_a_throwing_constructor);
    rb.run("...and a failed make leaks nothing",      &tests_a_failed_make_leaks_nothing);
    rb.run("write on shared is STRONGLY safe",        &tests_write_on_a_shared_pointer_is_strongly_safe);
    rb.run("detach is STRONGLY safe",                 &tests_detach_is_strongly_safe);
    rb.run("replace is STRONGLY safe (version too)",  &tests_state_replace_is_strongly_safe);
    rb.run("write BUMPS THE VERSION on throw (bug)",  &tests_state_write_bumps_the_version_even_when_the_clone_throws);
    rb.run("modify bumps the version on throw (bug)", &tests_state_modify_bumps_the_version_even_when_the_callable_throws);
    rb.run("modify releases the lock on throw",       &tests_state_modify_releases_the_lock_when_the_callable_throws);
    rb.run("a snapshot survives a failed write",      &tests_a_snapshot_survives_a_failed_write);

    // -- concurrency -------------------------------------------------------
    rb.module("Concurrency",
              "atomic refcount, serialized writers -- deterministic, never a race");
    rb.run("concurrent copies keep the count exact",  &tests_concurrent_copies_keep_the_refcount_exact);
    rb.run("concurrent snapshots never tear",         &tests_concurrent_snapshots_never_see_a_torn_value);
    rb.run("a held snapshot is stable under writes",  &tests_a_held_snapshot_is_stable_under_writes);
    rb.run("concurrent writers serialize (no loss)",  &tests_concurrent_writers_serialize);
    rb.run("readers + writers leak nothing",          &tests_concurrent_readers_and_writers_leak_nothing);
    rb.run("the shared policy admits 2 readers",       &tests_the_shared_policy_admits_concurrent_readers);
    rb.run("shared-policy read after snapshot is unlocked", &tests_shared_policy_read_after_snapshot_is_unlocked);

    return rb.finish();
}
