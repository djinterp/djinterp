/******************************************************************************
* djinterp [testing]                                          dtuple_tests.hpp
*
*   DTest framework demonstration — master header for the dtuple compile-
* time trait test suite.  Collects every per-component test function
* declaration, the detection probes and predicate wrappers shared across
* the semantic groups, the custom event declarations used to drive the
* hierarchical test tree, the setup / teardown helpers, and the
* orchestrator that threads everything together.
*
*   ORGANIZATION:
*   The dtuple tests are split across one header (this file) and a
* fleet of per-group .cpp translation units.  Each .cpp file contains
* the definitions for one cohesive slice of the dtuple surface; all
* declarations live here so any .cpp file can call any test function
* (and so the orchestrator can call them all).
*
*   GROUPS:
*     structural      first_arg, is_tuple, to_tuple, make_tuple_of
*     composition     wrap_all + modifiers, tuple_join
*     indexing        tuple_type_at, tuple_subsequence, tuple_split,
*                     reference-qualified round-trip, instantiation
*                     depth stress
*     transforms      tuple_apply_all, count_and_remove, standalone
*                     tuple_count_type
*     classification  type_selector, homogeneity_and_2d, 2d ∩ homog
*     flatten         flatten and normalize
*     meta            robustness, logical relationships, alias
*                     consistency, zero-size sweep, incomplete types,
*                     SFINAE-friendliness, composition interplay,
*                     concepts demo (C++20)
*
*   FRAMEWORK FEATURES EXERCISED:
*     - test_handler::on / off / enable / disable (mid-run, multiple ids)
*     - test_handler::fire / queue / process / process_all
*     - test_handler::run with a vector of basic_test (flat walk)
*     - test_tree overlay with a minimal n-ary tree backing shim
*     - every built-in lifecycle event (on_session_start/_end,
*       on_module_start/_end, on_test_start/_end, on_test_passed/
*       _failed, on_test_skipped, on_test_error, on_status_change,
*       on_listener_threw)
*     - custom events: on_compile_check, on_dtuple_demo,
*       on_group_start, on_group_end
*     - is_detected / detected_t / detected_or_t / is_detected_exact /
*       is_detected_convertible
*     - pack_all_of / pack_any_of / pack_none_of / pack_count_of /
*       pack_exactly_n_of
*     - trait_cv_stable / trait_ref_stable / trait_cvref_stable
*     - trait_implies_for / trait_equivalent_for / trait_disjoint_for
*     - alias_consistent / alias_consistent_for
*     - type_equal / type_equal_decayed / type_equal_cv_stripped /
*       type_equal_clean
*     - trait_record / trait_suite / trait_suite_object
*     - the full D_TEST_TRAIT_* / D_TEST_TYPE_* macro family
*     - D_TEST_CONCEPT_TRUE / _FALSE (gated on C++20 concepts)
*
*   PORTABILITY:
*   C++11 minimum is inherited from test_handler.hpp; dtuple.hpp itself
* demands C++17 template-template-parameter sugar so tests that drive
* it inherit that floor.  All standard-version gating routes through
* env.h and env_cpp_features.h — no raw __cplusplus comparisons and no
* direct __cpp_* feature-test macros appear in this header or in any
* companion .cpp file.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CUSTOM EVENT DECLARATIONS
II.   TEST FIXTURES AND PROBES
III.  PER-COMPONENT TEST DECLARATIONS
IV.   SETUP / TEARDOWN HELPERS
V.    OUTCOME RECORDING HELPERS
VI.   ORCHESTRATOR
*/

#ifndef DJINTERP_TESTING_META_DTUPLE_
#define DJINTERP_TESTING_META_DTUPLE_ 1


// =========================================================================
// INCLUDES
// =========================================================================

// std
#include <cstddef>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp — environment detection chain FIRST.  The framework
// headers below all route their own portability gates through
// env.h / env_cpp_features.h, so pulling these in explicitly here
// is redundant in a direct-compile scenario but makes the contract
// self-documenting: "this header expects the env chain to be
// available before any feature-flag macro is referenced".
#include "../../../../inc/djinterp/core/env/env.h"
#include "../../../../inc/djinterp/core/env/cpp/env_cpp_features.h"
// djinterp framework
#include "../../../../inc/djinterp/test/test_handler.hpp"
#include "../../../../inc/djinterp/test/test_object.hpp"
#include "../../../../inc/djinterp/test/test_trait.hpp"
#include "../../../../inc/djinterp/test/test_tree.hpp"
// djinterp module under test
#include "../../../../inc/djinterp/core/meta/dtuple.hpp"


// feature gates
//   test_handler.hpp already enforces the C++11 / variadic-
// templates / rvalue-references / lambdas floor.  dtuple.hpp
// enforces its own C++17 gate for the template-template sugar
// it uses internally.  This header adds no new floors — it is
// usable anywhere the framework headers it includes are.


NS_DJINTERP
NS_TESTING


// =========================================================================
// FRAMEWORK NAME IMPORTS
// =========================================================================
//   The DTest framework types live in `djinterp::test::`, but this
// test code lives in the sibling namespace `djinterp::testing::`.
// Because name lookup from a given namespace only searches that
// namespace, its parents, and global, unqualified references like
// `test_handler& _handler` below would otherwise never reach
// `djinterp::test::test_handler`.
//
//   A single using-directive pulls framework names into the local
// `djinterp::testing::` namespace so the rest of the file can read
// naturally without a forest of `test::` qualifications.  Using-
// directive form (as opposed to per-name using-declarations) is
// used because the dtuple test suite touches a wide range of
// framework names (test_handler, session_result, listener_id,
// event_context, test_status, basic_test, trait_suite, trait_record,
// trait_suite_object, pack_* / trait_* / alias_* / is_detected*)
// and the full import is less fragile under framework evolution
// than an enumerated list.
//
//   `events` is a sub-namespace (holds on_session_start,
// on_test_passed, and friends), not a type, so it is brought in
// with a namespace alias rather than a using-directive.
using namespace djinterp::test;

namespace events = djinterp::test::events;


// =========================================================================
// I.   CUSTOM EVENT DECLARATIONS
// =========================================================================

// on_compile_check
//   event: fired by each test function as it announces an
// individual compile-time check by name.  The orchestrator
// binds a verbose listener that prints "  .. <name>" for
// debugging; the listener is registered disabled and is
// enable()-d around the first test as a demo of mid-run
// enable/disable.
D_EVENT(on_compile_check,
        const char*);   // _check_name

// on_dtuple_demo
//   event: a no-payload notification fired once at the start of
// the dtuple demo block.  Demonstrates an empty custom event
// alongside the parameterized one above.
D_EVENT_EMPTY(on_dtuple_demo);

// on_group_start
//   event: fired by the orchestrator as it enters one of the
// semantic test groups (structural, composition, indexing,
// transforms, classification, flatten, meta).  Feeds the
// hierarchical test-tree visualization.
D_EVENT(on_group_start,
        const char*);   // _group_name

// on_group_end
//   event: fired by the orchestrator as it leaves a semantic
// group.  Carries the per-group pass / total counters so
// group-level sinks can render a mini-banner without polling
// the handler's counters.
D_EVENT(on_group_end,
        const char*,    // _group_name
        std::size_t,    // _passed
        std::size_t);   // _total


// =========================================================================
// II.  TEST FIXTURES AND PROBES
// =========================================================================
//   Every probe alias template and predicate wrapper lives here
// so each per-group .cpp can use them without duplicating the
// definitions.  Alias templates and class templates at namespace
// scope have no linkage or ODR issues across translation units
// that include this header.

// incomplete_demo_type
//   type: deliberate forward declaration (no definition) for
// the "incomplete type" robustness tests.  Traits must compile
// against incomplete types as long as they do not need size or
// layout — this fixture pins that contract.
struct incomplete_demo_type;


NS_INTERNAL

    // -----------------------------------------------------------------
    //  detection probes
    // -----------------------------------------------------------------
    // Each probe is a one-line alias whose ill-formedness is the
    // trait being tested; `is_detected<probe, args...>` then yields
    // a clean true/false answer at compile time.

    // probe_first_arg
    //   probe: yields the first type in a pack via dtuple's
    // first_arg_t.  Detection succeeds iff the pack is non-empty.
    template<typename... _Ts>
    using probe_first_arg = first_arg_t<_Ts...>;

    // probe_tuple_type_at
    //   probe: yields the type at the supplied index of a tuple.
    // Detection succeeds iff the index is in range.  An out-of-
    // range index SHOULD yield SFINAE failure (not a hard error)
    // if the trait is to be usable in concepts / enable_if — the
    // friendliness test in the meta group pins this contract.
    template<typename _IndexConst,
             typename _Tuple>
    using probe_tuple_type_at = tuple_type_at_t<_IndexConst::value, _Tuple>;

    // probe_make_tuple_of
    //   probe: yields a tuple of N copies of T.  Detection
    // succeeds for every non-negative N (including 0).
    template<typename _T,
             typename _CountConst>
    using probe_make_tuple_of =
        make_tuple_of_t<_T, _CountConst::value>;

    // probe_tuple_subsequence
    //   probe: yields the [start, end) subsequence of a tuple.
    // Detection succeeds iff start <= end <= size.
    template<typename _StartConst,
             typename _EndConst,
             typename _Tuple>
    using probe_tuple_subsequence =
        tuple_subsequence_t<_StartConst::value,
                            _EndConst::value,
                            _Tuple>;

    // probe_is_tuple
    //   probe: yields the ::value member of is_tuple.  Used to
    // verify is_tuple accepts incomplete types as template args
    // (since is_tuple does not inspect the type's size or layout).
    template<typename _T>
    using probe_is_tuple =
        std::integral_constant<bool, is_tuple<_T>::value>;


    // -----------------------------------------------------------------
    //  predicate wrappers
    // -----------------------------------------------------------------
    // The pack quantifiers in test_trait.hpp expect predicates of
    // the form `template<typename> class P`.  Several dtuple traits
    // take additional non-type or template-template parameters and
    // must be one-line wrapped before they can be quantified across
    // a type pack.  Other predicates are defined for logical-
    // relationship tests (e.g. "is this NOT a tuple").

    // is_dtuple_pred
    //   trait: unary predicate forwarding to djinterp::is_tuple.
    template<typename _T>
    struct is_dtuple_pred
    {
        static constexpr bool value = is_tuple<_T>::value;
    };

    // is_not_dtuple_pred
    //   trait: unary predicate forwarding to the logical negation
    // of djinterp::is_tuple.  Used in disjointness tests.
    template<typename _T>
    struct is_not_dtuple_pred
    {
        static constexpr bool value = !is_tuple<_T>::value;
    };

    // is_dtuple_homogeneous_pred
    //   trait: unary predicate forwarding to is_tuple_homogeneous.
    template<typename _T>
    struct is_dtuple_homogeneous_pred
    {
        static constexpr bool value =
            is_tuple_homogeneous<_T>::value;
    };

    // is_dtuple_2d_pred
    //   trait: unary predicate forwarding to is_2d_tuple.
    template<typename _T>
    struct is_dtuple_2d_pred
    {
        static constexpr bool value = is_2d_tuple<_T>::value;
    };

    // is_empty_tuple_pred
    //   trait: unary predicate true iff _T is exactly
    // std::tuple<>.  Demonstrates building a custom unary
    // predicate from std traits for composition with pack
    // quantifiers and implication / equivalence machinery.
    template<typename _T>
    struct is_empty_tuple_pred
    {
        static constexpr bool value =
            std::is_same<_T, std::tuple<>>::value;
    };


    // -----------------------------------------------------------------
    //  identity modifier (for wrap_all / tuple_apply_all tests)
    // -----------------------------------------------------------------

    // identity_trait
    //   trait: unary type identity in trait form (exposes ::type).
    // Used to exercise wrap_all's identity-composition contract.
    // std::type_identity exists only from C++20 onward; this is a
    // hand-rolled equivalent that works on every supported
    // standard.
    template<typename _T>
    struct identity_trait
    {
        using type = _T;
    };

NS_END  // internal


// =========================================================================
// III. PER-COMPONENT TEST DECLARATIONS
// =========================================================================
//   each function below executes a cohesive group of compile-time
// trait checks for one dtuple component (or one facet of one
// component).  successful return indicates every static_assert
// inside compiled cleanly.
//
//   declarations are grouped here by the semantic group they belong
// to, matching the .cpp file that holds their definitions.  the
// orchestrator calls them group-by-group with on_group_start /
// on_group_end boundaries so the resulting test tree is self-
// describing.

// structural group — dtuple_tests_structural.cpp
bool tests_dtuple_first_arg                  (test_handler& _handler);
bool tests_dtuple_is_tuple                   (test_handler& _handler);
bool tests_dtuple_to_tuple                   (test_handler& _handler);
bool tests_dtuple_make_tuple_of              (test_handler& _handler);

// composition group — dtuple_tests_composition.cpp
bool tests_dtuple_wrap_all_and_modifiers     (test_handler& _handler);
bool tests_dtuple_tuple_join                 (test_handler& _handler);

// indexing group — dtuple_tests_indexing.cpp
bool tests_dtuple_tuple_type_at              (test_handler& _handler);
bool tests_dtuple_tuple_subsequence          (test_handler& _handler);
bool tests_dtuple_tuple_split                (test_handler& _handler);
bool tests_dtuple_indexing_refqual_roundtrip (test_handler& _handler);
bool tests_dtuple_indexing_stress_depth      (test_handler& _handler);

// transforms group — dtuple_tests_transforms.cpp
bool tests_dtuple_tuple_apply_all            (test_handler& _handler);
bool tests_dtuple_count_and_remove           (test_handler& _handler);
bool tests_dtuple_tuple_count_type           (test_handler& _handler);

// classification group — dtuple_tests_classification.cpp
bool tests_dtuple_type_selector              (test_handler& _handler);
bool tests_dtuple_homogeneity_and_2d         (test_handler& _handler);
bool tests_dtuple_is_2d_intersect_homog      (test_handler& _handler);

// flatten group — dtuple_tests_flatten.cpp
bool tests_dtuple_flatten_and_normalize      (test_handler& _handler);

// meta group — dtuple_tests_meta.cpp
bool tests_dtuple_robustness                 (test_handler& _handler);
bool tests_dtuple_logical_relationships      (test_handler& _handler);
bool tests_dtuple_alias_consistency          (test_handler& _handler);
bool tests_dtuple_zero_size_sweep            (test_handler& _handler);
bool tests_dtuple_incomplete_types           (test_handler& _handler);
bool tests_dtuple_sfinae_friendliness        (test_handler& _handler);
bool tests_dtuple_composition_interplay      (test_handler& _handler);

// concepts demo (C++20 only; a stub always exists so linker calls
// remain well-formed across standards)
bool tests_dtuple_concepts_demo              (test_handler& _handler);


// =========================================================================
// IV.  SETUP / TEARDOWN HELPERS
// =========================================================================

// listener_handle_set
//   struct: aggregates the listener_id values returned from
// setup_dtuple_listeners so the orchestrator can later enable /
// disable individual listeners (e.g. flip on the verbose
// compile-check sink around one test function) and
// teardown_dtuple_listeners can unbind every listener in one
// call.
struct listener_handle_set
{
    listener_id session_start;
    listener_id session_end;
    listener_id module_start;
    listener_id module_end;
    listener_id test_passed;
    listener_id test_failed;
    listener_id test_skipped;
    listener_id test_error;
    listener_id status_change;
    listener_id listener_threw;
    listener_id compile_check;
    listener_id dtuple_demo;
    listener_id group_start;
    listener_id group_end;
};

// setup_dtuple_listeners
//   helper: binds one listener for every built-in lifecycle
// event and every custom event declared above.  The compile-
// check listener is bound DISABLED so it does not flood the
// output; callers enable it around whichever test function
// they want verbose reporting for.
listener_handle_set
setup_dtuple_listeners(
    test_handler& _handler
);

// teardown_dtuple_listeners
//   helper: unbinds every listener in the supplied handle set.
// Called at the end of run_dtuple_tests so the handler is
// returned to its pre-run state for any caller that intends
// to reuse it.
void
teardown_dtuple_listeners(
    test_handler&               _handler,
    const listener_handle_set&  _handles
);


// =========================================================================
// V.   OUTCOME RECORDING HELPERS
// =========================================================================

// record_outcome
//   helper: fires the appropriate built-in lifecycle event
// with a stable const char* name, then updates the handler's
// pass/fail counters.  Constructs a transient basic_test
// solely so the standard lifecycle events have a nameable
// payload to carry.
void
record_outcome(
    test_handler& _handler,
    const char*   _name,
    bool          _result
);

// record_group_boundary
//   helper: fires on_group_start or on_group_end with the
// supplied payload.  Separated out so the orchestrator's
// per-group block reads as a single call rather than two
// fire-site literals.
void
record_group_boundary(
    test_handler& _handler,
    const char*   _group_name,
    bool          _is_start,
    std::size_t   _passed,
    std::size_t   _total
);


// =========================================================================
// VI.  ORCHESTRATOR
// =========================================================================

// run_dtuple_tests
//   entry: binds every lifecycle + custom listener via
// setup_dtuple_listeners, walks each semantic test group in
// sequence with on_group_start / on_group_end boundaries,
// demonstrates mid-run enable/disable of multiple listener
// ids, drives a compile-time aggregate trait_suite through
// the runtime adapter (trait_suite_object), constructs a
// demonstration test_tree<basic_test, shim> and queries its
// count_passed / all_passed methods, unbinds every listener
// via teardown_dtuple_listeners, and returns the accumulated
// session_result for consumption by the suite printer.
session_result
run_dtuple_tests(
    test_handler& _handler
);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_META_DTUPLE_
