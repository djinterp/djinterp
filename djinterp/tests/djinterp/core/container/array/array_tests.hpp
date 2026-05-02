/******************************************************************************
* djinterp [testing]                                           array_tests.hpp
*
*   Declarations for the djinterp `array` container test suite.
*
*   This header exposes ONLY function declarations; every test is
* defined in array_core_tests.cpp (and any additional .cpp files
* in the same suite).  This split lets the test machinery be
* compiled once into an object file and linked into multiple test
* binaries (e.g. fast smoke, full regression, fuzzing harness) at
* the user's discretion.
*
*   RETURN-VALUE PROTOCOL:
*   Every test category function is a pure value producer: it
* takes a `test_type_id` and returns an `array_test_tree`
* representing the subtree of that category's results.  Tests
* take no `test_handler&`, no `test_printer&`, and no output
* sink.  The framework dispatches by walking the assembled tree
* through `test_handler::run(tree)`; the handler's bound
* listeners (lifecycle and value-tagged) drive whatever printer,
* logger, or alternative reporter the caller has wired up.
*   Leaves carry status (passed / failed / skipped / error /
* pending) directly; subtrees aggregate them by structure.
*
*   COVERAGE INDEX
*   ==============
*   I.    Compile-time trait conformance
*           - constexpr / runtime / mutable / iterable axes
*           - bounded / sorted / flat-vs-hierarchical axes
*           - storage kind classification
*           - lifetime + iterability cell taxonomy from array_traits.hpp
*   II.   Core construction and destruction
*           - default construction of every cell
*           - parameter-pack construction
*           - copy / move construction
*           - zero-extent and single-extent edge cases
*   III.  Element access
*           - operator[]   (bounds-of-cell)
*           - at()         (matching the project's at-protocol)
*           - front() / back()
*           - data()
*           - constness preservation
*   IV.   Iteration
*           - begin()/end() round trips
*           - cbegin()/cend()
*           - rbegin()/rend()  (reverse iteration)
*           - range-based-for traversal
*           - non-iterable cells must fail SFINAE on begin()
*   V.    Mutation (mutable cells only)
*           - subscript assignment
*           - fill()
*           - swap()
*           - immutable cells must fail SFINAE on the above
*   VI.   Free-function bulk algorithms
*           - array_equal
*           - array_copy
*           - array_swap
*   VII.  Constexpr usability
*           - construct, access, and compare in constant evaluation
*           - relaxed-constexpr (C++14+) mutator paths
*   VIII. Iterator interop with the constexpr_iterator algorithms
*           - cx_find / cx_count_if / cx_all_of / cx_equal across
*             iterable cells.
*
*   PORTABILITY:
*   C++11 baseline - see env.h and env_cpp_features.h for the
* feature gates.  Concept-based assertions inside the bodies
* are gated on D_ENV_LANG_IS_CPP20_OR_HIGHER &&
* D_ENV_CPP_FEATURE_LANG_CONCEPTS in the .cpp file; at the
* declaration level all functions have the same signature
* regardless of language version, so callers don't need to
* gate their own use.
*
*
* TABLE OF CONTENTS
* =================
* I.    SUITE TYPE ALIASES
* II.   CATEGORY: TRAIT CONFORMANCE
* III.  CATEGORY: CONSTRUCTION
* IV.   CATEGORY: ELEMENT ACCESS
* V.    CATEGORY: ITERATION
* VI.   CATEGORY: MUTATION
* VII.  CATEGORY: BULK ALGORITHMS
* VIII. CATEGORY: CONSTEXPR USABILITY
* IX.   CATEGORY: ITERATOR ALGORITHM INTEROP
* X.    AGGREGATE SUBTREE BUILDER
* XI.   MASTER-SUITE RUNNER
*
*
* path:      /tests/djinterp/core/container/array/array_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_TEST_ARRAY_TESTS_
#define DJINTERP_TEST_ARRAY_TESTS_ 1


// =========================================================================
// PORTABILITY CHECKS
// =========================================================================

#ifndef __cplusplus
    #error "array_tests.hpp can only be used in C++ compilation mode"
#endif


// std
#include <cstddef>
// djinterp  --  pull the environment header chain FIRST so that the
// feature-flag macros below are defined before we test them.
#include "../../../../../inc/djinterp/core/djinterp.hpp"
#include "../../../../../inc/djinterp/core/container/tree/nary/nary_tree.hpp"
#include "../../../../../inc/djinterp/core/container/traits/sorted_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/traits/flat_hierarchical_container_traits.hpp"
#include "../../../../../inc/djinterp/test/test_common.hpp"
#include "../../../../../inc/djinterp/test/test_defaults.hpp"
#include "../../../../../inc/djinterp/test/test_event.hpp"
#include "../../../../../inc/djinterp/test/test_handler.hpp"
#include "../../../../../inc/djinterp/test/test_object.hpp"
#include "../../../../../inc/djinterp/test/test_tree.hpp"
#include "../../../../../inc/djinterp/core/container/array/array.hpp"


// feature gates
//   the array_tests suite uses C++11-baseline features in its
// declarations and a wider set in its definitions.  the
// per-cpp gates (concepts, variable templates) live next to
// their use sites; only the absolute minimum is checked here
// so that consumers of the header get a clean diagnostic at
// the include site rather than in their own translation
// unit's expansion.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "array_tests.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   SUITE TYPE ALIASES
// =========================================================================

// array_test_obj
//   type: per-suite alias for the framework's element type.
// All array-suite test functions produce nodes of this type;
// using a suite-local name keeps grep affinity per-suite while
// the underlying instantiation remains the framework default.
//   At present, this resolves to djinterp::test::basic_test
// (the default test_object<> instantiation).
using array_test_obj  = djinterp::test::basic_test;

// array_test_tree
//   type: the project's test_tree overlay (from test_tree.hpp)
// instantiated for array_test_obj, with the framework's
// default n-ary tree as the backing storage.  Every array-suite
// category function returns one of these, and the suite-level
// helper assembles them into a single root tree representing
// the full module.
//   The rank-validation flag is left at its default (true);
// the array suite's hierarchy never violates the rank
// invariant (assertions inside tests inside blocks inside the
// module).
using array_test_tree = djinterp::test::test_tree<
                            array_test_obj,
                            djinterp::nary_tree<array_test_obj>>;


// =========================================================================
// II.  CATEGORY: TRAIT CONFORMANCE
// =========================================================================
//   Verifies that every cell of the lifetime x iterability cube
// reports the trait flags that array_traits.hpp + the
// classification_traits modules say it should.  Failures here
// indicate a regression in the trait detectors OR in the array's
// public interface (an unexpected method exposed/hidden).

array_test_tree test_array_axis_constexpr_runtime(test::test_type_id _kind);
array_test_tree test_array_axis_mutable_immutable(test::test_type_id _kind);
array_test_tree test_array_axis_iterable_non_iterable(test::test_type_id _kind);
array_test_tree test_array_axis_bounded(test::test_type_id _kind);
array_test_tree test_array_axis_sorted_unsorted(test::test_type_id _kind);
array_test_tree test_array_axis_flat_hierarchical(test::test_type_id _kind);
array_test_tree test_array_axis_storage_kind(test::test_type_id _kind);
array_test_tree test_array_lifetime_taxonomy(test::test_type_id _kind);


// =========================================================================
// III. CATEGORY: CONSTRUCTION
// =========================================================================
//   Default construction, parameter-pack construction, copy / move,
// plus the two extent edge cases (extent=0 and extent=1).  These
// tests trigger constructor selection for every cell of the cube.

array_test_tree test_array_default_construction(test::test_type_id _kind);
array_test_tree test_array_pack_construction(test::test_type_id _kind);
array_test_tree test_array_copy_construction(test::test_type_id _kind);
array_test_tree test_array_move_construction(test::test_type_id _kind);
array_test_tree test_array_zero_extent_edge_case(test::test_type_id _kind);
array_test_tree test_array_single_extent_edge_case(test::test_type_id _kind);


// =========================================================================
// IV.  CATEGORY: ELEMENT ACCESS
// =========================================================================
//   Verifies operator[], at(), front(), back(), and data() across
// const and non-const overloads.  Edge case: a one-element array
// has front() == back().

array_test_tree test_array_subscript_access(test::test_type_id _kind);
array_test_tree test_array_at_access(test::test_type_id _kind);
array_test_tree test_array_front_back_access(test::test_type_id _kind);
array_test_tree test_array_data_access(test::test_type_id _kind);
array_test_tree test_array_const_access_paths(test::test_type_id _kind);


// =========================================================================
// V.   CATEGORY: ITERATION
// =========================================================================
//   begin/end round-trips, range-based for, reverse iteration, and
// SFINAE absence of begin() on non-iterable cells.

array_test_tree test_array_begin_end(test::test_type_id _kind);
array_test_tree test_array_const_iteration(test::test_type_id _kind);
array_test_tree test_array_reverse_iteration(test::test_type_id _kind);
array_test_tree test_array_range_based_for(test::test_type_id _kind);
array_test_tree test_array_non_iterable_sfinae(test::test_type_id _kind);


// =========================================================================
// VI.  CATEGORY: MUTATION
// =========================================================================
//   Mutators on mutable cells; SFINAE absence on immutable cells.

array_test_tree test_array_subscript_assignment(test::test_type_id _kind);
array_test_tree test_array_fill(test::test_type_id _kind);
array_test_tree test_array_member_swap(test::test_type_id _kind);
array_test_tree test_array_immutable_sfinae(test::test_type_id _kind);


// =========================================================================
// VII. CATEGORY: BULK ALGORITHMS
// =========================================================================

array_test_tree test_array_equal_function(test::test_type_id _kind);
array_test_tree test_array_copy_function(test::test_type_id _kind);
array_test_tree test_array_swap_function(test::test_type_id _kind);


// =========================================================================
// VIII. CATEGORY: CONSTEXPR USABILITY
// =========================================================================
//   Demonstrates that an array can be constructed, accessed, and
// compared inside a constant expression.  C++14+ also exercises
// the relaxed-constexpr mutator path.

array_test_tree test_array_constexpr_construction(test::test_type_id _kind);
array_test_tree test_array_constexpr_access(test::test_type_id _kind);
array_test_tree test_array_constexpr_mutation_cpp14(test::test_type_id _kind);


// =========================================================================
// IX.  CATEGORY: ITERATOR ALGORITHM INTEROP
// =========================================================================

array_test_tree test_array_constexpr_iterator_algorithms(test::test_type_id _kind);


// =========================================================================
// X.   AGGREGATE SUBTREE BUILDER
// =========================================================================

// make_array_test_subtree
//   helper: invokes every category function in declaration
// order and assembles the returned subtrees as children of a
// single module-level root, returning the resulting tree by
// value.
//   The returned tree is the array suite's "test module" - a
// rooted tree whose root carries the module's type_id, whose
// immediate children are category-level subtrees, and whose
// leaves are the assertion-level outcomes produced inside
// each category function.
//
//   The function neither knows about nor requires a
// test_handler or test_printer.  The caller hands the
// returned tree to whichever handler is configured (see
// section XI for the suite's master runner) and the
// handler's bound listeners drive the output.
//
// Parameter(s):
//   _kind:  the test_type_id stamped on the module root.
//           Defaults to the framework's MODULE kind constant
//           (D_TEST_KIND_MODULE) so that simple users don't
//           need to learn the kind taxonomy on day one.
array_test_tree
make_array_test_subtree(
    test::test_type_id _kind = test::D_TEST_KIND_MODULE);


// =========================================================================
// XI.  MASTER-SUITE RUNNER
// =========================================================================

// run_array_suite
//   driver: builds the array suite's subtree via
// make_array_test_subtree(), passes it to the supplied
// handler's run() method, and returns the three-way
// verdict from the handler's session_result.
//   The handler's listener bundle (lifecycle + any value-
// tagged listeners) determines what gets emitted to a
// printer, log, or other sink - the runner does not touch
// any of that.  Any test_handler subclass is acceptable;
// default_test_handler from test_defaults.hpp installs the
// framework's standard threshold-filtered printer bundle
// when its set_printer() is called.
//
//   The returned verdict distinguishes:
//     - session_verdict::passed  : every leaf passed
//     - session_verdict::pending : no failures, but at
//                                  least one leaf is
//                                  pending (not yet
//                                  implemented)
//     - session_verdict::failed  : at least one leaf
//                                  failed or errored
//     - session_verdict::empty   : no leaves observed
//
//   Callers that only need a bool can apply the policy
// they prefer: `verdict == session_verdict::passed` for
// strict "all green," `verdict != session_verdict::failed`
// for permissive "nothing broken yet."
//
// Parameter(s):
//   _handler:     the test_handler driving the walk.  Listener
//                 bundle (printer, logger, etc.) must already
//                 be attached if any output is desired.
//   _kind:        the test_type_id stamped on the module
//                 root.  Forwarded to make_array_test_subtree.
//   _out_seconds: optional; receives the wall-clock duration
//                 of the run if non-null.
// Return:
//   The session_verdict for this run.
test::session_verdict
run_array_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind        = test::D_TEST_KIND_MODULE,
    double*             _out_seconds = nullptr);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_ARRAY_TESTS_
