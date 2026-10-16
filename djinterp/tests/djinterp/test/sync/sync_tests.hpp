/*******************************************************************************
* djinterp [tests]                                                sync_tests.hpp
*
*   Aggregate suite header for the whole djinterp/test/sync subframework.
*
*   Unlike a per-module suite header this one declares no tests of its own and
* has no fixtures: it exists only to gather the eight module providers into a
* single vector for run_suite.  It is therefore SPEC-MODE ONLY - there is
* nothing for a normal-mode section TU to consume, and no section TU includes
* it.
*
*   Every module suite header is included in spec mode, so each contributes
* only its provider face (declarations plus the inline spec function); none of
* them pulls in a header under test.  DTEST_SPEC_MODE is asserted below rather
* than assumed, because including these headers in normal mode here would drag
* the entire subframework into this TU for no purpose.
*
*   ORDERING: the modules are listed in dependency order - the foundational
* thread and coordination modules first, then the harnesses layered on them,
* then the umbrella.  A suite report therefore reads bottom-up, which is the
* order in which a failure is most usefully diagnosed: if test_thread is
* broken, everything downstream of it is expected to be noisy too.
*
*   The conditional factory blocks inside the module suites (gated on
* D_HC_/D_HS_/D_HR_/D_HD_FACTORY_RETURNABLE) are handled entirely by those
* headers; this file needs no gating of its own, since it references only the
* provider functions, which exist unconditionally.
*
*
* path:      /tests/djinterp/test/sync/sync_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SYNC_TESTS_
#define DJINTERP_TESTS_SYNC_TESTS_ 1

#ifndef DTEST_SPEC_MODE
#error "sync_tests.hpp is an aggregate provider: define DTEST_SPEC_MODE first."
#endif

// std
#include <vector>
// djinterp
#include <djinterp/djinterp.hpp>
#include "djinterp/test/test_defaults.hpp"
// the eight module suites, each contributing its provider face
#include "test_thread_tests.hpp"
#include "test_sync_tests.hpp"
#include "test_concurrent_tests.hpp"
#include "test_stress_tests.hpp"
#include "test_race_tests.hpp"
#include "test_invariant_tests.hpp"
#include "test_deadlock_tests.hpp"
#include "test_threadsafe_tests.hpp"


NS_DJINTERP
NS_TESTING

// NOTE: `namespace dt = ::djinterp::test;` is already established by every
// module suite header above (they declare it unconditionally, in both modes),
// so it is deliberately not repeated here - a second alias declaration in the
// same namespace is legal only because it names the same entity, and relying
// on that is needless.


// sync_all_specs
//   returns every module spec in the subframework, in dependency order.
inline std::vector<dt::module_spec>
sync_all_specs()
{
    std::vector<dt::module_spec> specs;
    specs.reserve(8);

    // foundation: threads and the coordination primitives
    specs.push_back(thread_spec());
    specs.push_back(sync_spec());

    // harnesses layered on the foundation
    specs.push_back(concurrent_spec());
    specs.push_back(stress_spec());
    specs.push_back(race_spec());
    specs.push_back(invariant_spec());
    specs.push_back(deadlock_spec());

    // the umbrella that ties them together
    specs.push_back(threadsafe_spec());

    return specs;
}


NS_END  // testing
NS_END  // djinterp

#endif  // DJINTERP_TESTS_SYNC_TESTS_
