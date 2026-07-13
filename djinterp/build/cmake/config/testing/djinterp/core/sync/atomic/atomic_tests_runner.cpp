// djinterp
#include "atomic_tests.hpp"                                // the suite decls
#include <djinterp/test/output/test_report_runner.hpp>     // report_builder + model


// D_AT_RUN
//   macro: runs ::djinterp::testing::<fn> as a named unit test - recorded in
// the report and echoed to the live console.  Unique letters (AT) so a
// co-compiled runner never collides on the macro name.
#define D_AT_RUN(_fn)   rb.run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    ::djinterp::test::report_builder rb;

    rb.set_title("atomic.hpp unit tests");

    // write a PDF beside the live console output; omit this line for a
    // console-only run (see test_options.hpp for layout / naming knobs).
    rb.use_pdf("atomic_tests.pdf");

    // One module per like-group section of atomic.hpp, so the report's module
    // summary lines up with the header's structure.

    // I. atomic_size
    rb.module("atomic_size",
              "I. atomic_size: element-count wrapper - ctors, load/store, "
              "fetch, increment/decrement, CAS, conversion");
    D_AT_RUN(tests_atomic_size_default_ctor);
    D_AT_RUN(tests_atomic_size_explicit_ctor);
    D_AT_RUN(tests_atomic_size_load_store);
    D_AT_RUN(tests_atomic_size_fetch_add);
    D_AT_RUN(tests_atomic_size_fetch_sub);
    D_AT_RUN(tests_atomic_size_increment);
    D_AT_RUN(tests_atomic_size_decrement);
    D_AT_RUN(tests_atomic_size_cas_strong);
    D_AT_RUN(tests_atomic_size_cas_weak);
    D_AT_RUN(tests_atomic_size_conversion);
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_AT_RUN(tests_atomic_size_wait_returns_on_change);
    D_AT_RUN(tests_atomic_size_notify);
#endif

    // II. atomic_version
    rb.module("atomic_version",
              "II. atomic_version: generation counter - ctors, load/store, "
              "fetch_add, bump, CAS, conversion, full 64-bit width");
    D_AT_RUN(tests_atomic_version_default_ctor);
    D_AT_RUN(tests_atomic_version_explicit_ctor);
    D_AT_RUN(tests_atomic_version_load_store);
    D_AT_RUN(tests_atomic_version_fetch_add);
    D_AT_RUN(tests_atomic_version_bump);
    D_AT_RUN(tests_atomic_version_cas_strong);
    D_AT_RUN(tests_atomic_version_cas_weak);
    D_AT_RUN(tests_atomic_version_conversion);
    D_AT_RUN(tests_atomic_version_large_values);
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_AT_RUN(tests_atomic_version_wait_returns_on_change);
    D_AT_RUN(tests_atomic_version_notify);
#endif

    // III. atomic_flag_guard
    rb.module("atomic_flag_guard",
              "III. atomic_flag_guard: RAII test-and-set - was_set, "
              "set-on-construct, clear-on-destruct, scoped-latch semantics");
    D_AT_RUN(tests_flag_guard_was_set_false_on_clear_flag);
    D_AT_RUN(tests_flag_guard_sets_flag_on_construct);
    D_AT_RUN(tests_flag_guard_clears_on_destruct);
    D_AT_RUN(tests_flag_guard_was_set_true_when_already_held);
    D_AT_RUN(tests_flag_guard_one_shot_init_pattern);

    // IV. atomic_stamped_ptr
    rb.module("atomic_stamped_ptr",
              "IV. atomic_stamped_ptr: ABA-defeating packed pointer - ctors, "
              "store/load, stamp range, weak CAS, ABA defeat, wrap, "
              "sign-extension");
    D_AT_RUN(tests_stamped_default_ctor);
    D_AT_RUN(tests_stamped_explicit_ctor);
    D_AT_RUN(tests_stamped_store_load);
    D_AT_RUN(tests_stamped_stamp_range);
    D_AT_RUN(tests_stamped_null_with_stamp);
    D_AT_RUN(tests_stamped_cas_weak_success);
    D_AT_RUN(tests_stamped_cas_weak_failure_updates_expected);
    D_AT_RUN(tests_stamped_aba_defeat);
    D_AT_RUN(tests_stamped_stamp_wrap);
    D_AT_RUN(tests_stamped_sign_extension);

    // V. type / trait surface (compile-time)
    rb.module("traits",
              "V. compile-time surface - value_type, strategy tag, copy/move "
              "deletion, stamp_type, noexcept guarantees");
    D_AT_RUN(tests_atomic_size_value_type);
    D_AT_RUN(tests_atomic_version_value_type);
    D_AT_RUN(tests_atomic_size_strategy_tag);
    D_AT_RUN(tests_atomic_version_strategy_tag);
    D_AT_RUN(tests_atomic_size_noncopyable);
    D_AT_RUN(tests_atomic_version_noncopyable);
    D_AT_RUN(tests_flag_guard_noncopyable);
    D_AT_RUN(tests_stamped_ptr_noncopyable);
    D_AT_RUN(tests_stamped_stamp_type);
    D_AT_RUN(tests_atomic_size_noexcept);
    D_AT_RUN(tests_atomic_version_noexcept);

    // VI. concurrency (real thread contention)
    rb.module("concurrency",
              "VI. contention - concurrent increment / inc-dec / bump "
              "convergence, lock-free CAS loop, flag-guard churn");
    D_AT_RUN(tests_atomic_size_concurrent_increment);
    D_AT_RUN(tests_atomic_size_concurrent_inc_dec);
    D_AT_RUN(tests_atomic_version_concurrent_bump);
    D_AT_RUN(tests_atomic_size_concurrent_cas);
    D_AT_RUN(tests_atomic_flag_guard_concurrent_churn);

    return rb.finish();
}

#undef D_AT_RUN
