/******************************************************************************
* djinterp [test]                                            atomic_tests.hpp
*
*   Unit-test declarations for core/sync/atomic.hpp.  Every test is a niladic
* predicate returning true on pass (the report_builder::run signature); all
* live FLAT in namespace djinterp::testing.  The definitions are split across
* section translation units, one per like-group semantic section of the
* header under test:
*
*     atomic_tests_size.cpp        - I.   atomic_size
*     atomic_tests_version.cpp     - II.  atomic_version
*     atomic_tests_flag_guard.cpp  - III. atomic_flag_guard
*     atomic_tests_stamped_ptr.cpp - IV.  atomic_stamped_ptr<T>
*     atomic_tests_traits.cpp      -      the compile-time type/trait surface
*                                         (value_type, strategy tag, copy /
*                                         move deletion, noexcept guarantees)
*     atomic_tests_concurrency.cpp -      behavior under real thread contention
*
*   There are NO shared helper types: every atomic type is exercised directly.
* The C++20-only wait/notify surface is gated behind
* D_ENV_LANG_IS_CPP20_OR_HIGHER exactly as it is in atomic.hpp, so this suite
* also builds (minus those two-per-type cases) on a C++11/14/17 configuration.
*
*
* path:      /tests/djinterp/core/sync/atomic_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.13
******************************************************************************/

#ifndef DJINTERP_CORE_SYNC_ATOMIC_TESTS_
#define DJINTERP_CORE_SYNC_ATOMIC_TESTS_ 1

// djinterp
#include "atomic.hpp"   // the unit under test (also supplies NS_* macros and
                        //   the D_ENV_LANG_* feature gates via djinterp.hpp)


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   ATOMIC SIZE
// =========================================================================

bool tests_atomic_size_default_ctor();
bool tests_atomic_size_explicit_ctor();
bool tests_atomic_size_load_store();
bool tests_atomic_size_fetch_add();
bool tests_atomic_size_fetch_sub();
bool tests_atomic_size_increment();
bool tests_atomic_size_decrement();
bool tests_atomic_size_cas_strong();
bool tests_atomic_size_cas_weak();
bool tests_atomic_size_conversion();

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
bool tests_atomic_size_wait_returns_on_change();
bool tests_atomic_size_notify();
#endif


// =========================================================================
// II.  ATOMIC VERSION
// =========================================================================

bool tests_atomic_version_default_ctor();
bool tests_atomic_version_explicit_ctor();
bool tests_atomic_version_load_store();
bool tests_atomic_version_fetch_add();
bool tests_atomic_version_bump();
bool tests_atomic_version_cas_strong();
bool tests_atomic_version_cas_weak();
bool tests_atomic_version_conversion();
bool tests_atomic_version_large_values();

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
bool tests_atomic_version_wait_returns_on_change();
bool tests_atomic_version_notify();
#endif


// =========================================================================
// III. ATOMIC FLAG GUARD
// =========================================================================

bool tests_flag_guard_was_set_false_on_clear_flag();
bool tests_flag_guard_sets_flag_on_construct();
bool tests_flag_guard_clears_on_destruct();
bool tests_flag_guard_was_set_true_when_already_held();
bool tests_flag_guard_one_shot_init_pattern();


// =========================================================================
// IV.  ATOMIC STAMPED POINTER
// =========================================================================

bool tests_stamped_default_ctor();
bool tests_stamped_explicit_ctor();
bool tests_stamped_store_load();
bool tests_stamped_stamp_range();
bool tests_stamped_null_with_stamp();
bool tests_stamped_cas_weak_success();
bool tests_stamped_cas_weak_failure_updates_expected();
bool tests_stamped_aba_defeat();
bool tests_stamped_stamp_wrap();
bool tests_stamped_sign_extension();


// =========================================================================
// V.   TYPE / TRAIT SURFACE  (compile-time)
// =========================================================================

bool tests_atomic_size_value_type();
bool tests_atomic_version_value_type();
bool tests_atomic_size_strategy_tag();
bool tests_atomic_version_strategy_tag();
bool tests_atomic_size_noncopyable();
bool tests_atomic_version_noncopyable();
bool tests_flag_guard_noncopyable();
bool tests_stamped_ptr_noncopyable();
bool tests_stamped_stamp_type();
bool tests_atomic_size_noexcept();
bool tests_atomic_version_noexcept();


// =========================================================================
// VI.  CONCURRENCY  (real thread contention)
// =========================================================================

bool tests_atomic_size_concurrent_increment();
bool tests_atomic_size_concurrent_inc_dec();
bool tests_atomic_version_concurrent_bump();
bool tests_atomic_size_concurrent_cas();
bool tests_atomic_flag_guard_concurrent_churn();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_CORE_SYNC_ATOMIC_TESTS_
