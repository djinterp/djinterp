/******************************************************************************
* djinterp [testing]                                           array_tests.hpp
*
*   Declarations for the djinterp `array` container test suite,
* including the threadsafe wrappers (threadsafe_array, atomic_array,
* and cow_array).
*
*   This header exposes ONLY function declarations; every test is
* defined in one of the suite's .cpp files (currently
* array_core_tests.cpp for the base array container, and
* threadsafe_array_core_tests.cpp for the concurrent wrappers).
* The header / multi-cpp split lets the test machinery be compiled
* once into an object file per translation unit and linked into
* multiple test binaries (e.g. fast smoke, full regression, fuzzing
* harness, race-detection) at the user's discretion.
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
*
*   PART A.  BASE ARRAY CONTAINER  (array_core_tests.cpp)
*
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
*           - fill
*           - member swap
*           - immutable cells must fail SFINAE on mutators
*   VI.   Bulk algorithms
*           - djinterp::equal / copy / swap free functions
*   VII.  Constexpr usability
*           - construction, access, comparison in constant contexts
*           - relaxed-constexpr (C++14+) mutator paths
*   VIII. Iterator interop with the constexpr_iterator algorithms
*           - cx_find / cx_count_if / cx_all_of / cx_equal across
*             iterable cells.
*
*   PART B.  THREADSAFE WRAPPERS  (threadsafe_array_core_tests.cpp)
*
*   IX.   Trait conformance for the wrappers
*           - lifetime / iterability axes preserved across wrapping
*           - strategy classification (locked / atomic / cow)
*           - mutual disjointness of strategy tags
*   X.    threadsafe_array construction & assignment
*           - default, parameter-pack, copy
*           - move-deletion (mutex non-portably-transferable)
*   XI.   threadsafe_array lock-free queries
*           - size_lockfree, version, empty_lockfree
*   XII.  threadsafe_array single-op locked access
*           - size, empty, at, set
*   XIII. threadsafe_array handle-based access
*           - read_access, write_access, locked_ref RAII
*   XIV.  threadsafe_array bulk operations
*           - assign, apply, apply_read, batch_guard
*   XV.   threadsafe_array optimistic read
*           - version-validated reads, fall-through on contention
*   XVI.  threadsafe_array snapshot
*           - lock-released iteration over copy
*   XVII. threadsafe_array convenience aliases
*           - mutex_array, timed_array, shared_array (C++17)
*   XVIII. threadsafe_array lock policy variation
*           - null, exclusive, timed, shared (per language version)
*   XIX.  threadsafe_array concurrent access
*           - reader/writer races, version monotonicity under load
*   XX.   atomic_array construction
*           - default zero-init, fill ctor, deleted copy/move
*   XXI.  atomic_array element atomic access
*           - load, store, exchange (with memory orderings)
*   XXII. atomic_array element atomic updates
*           - fetch_add / fetch_sub / fetch_and / fetch_or / fetch_xor
*   XXIII. atomic_array element CAS
*           - compare_exchange_weak, compare_exchange_strong
*   XXIV. atomic_array bulk operations
*           - size, empty, fill, is_lock_free
*   XXV.  atomic_array iteration
*           - begin/end/cbegin/cend, data, range-based for
*   XXVI. atomic_array concurrent access
*           - per-element races, fetch_add monotonicity under load
*   XXVII. cow_array construction
*           - default, from-array, deleted copy/move
*   XXVIII. cow_array read access
*           - read, size, empty, at
*   XXIX. cow_array snapshot
*           - independence from later mutations, version stamping
*   XXX.  cow_array write access
*           - modify, replace, set, version monotonicity
*   XXXI. cow_array concurrent access
*           - reader-survives-writer races, snapshot consistency
*   XXXII. Cross-cutting wrapper edge cases
*           - zero-extent, single-element, large extents
*
*   PART C.  AGGREGATE BUILDERS AND RUNNERS  (per .cpp file)
*
*   XXXIII. Aggregate subtree builders
*           - make_array_test_subtree            (base, in array_core_tests.cpp)
*           - make_threadsafe_array_subtree      (threadsafe_array module)
*           - make_atomic_array_subtree          (atomic_array module)
*           - make_cow_array_subtree             (cow_array module)
*           - make_threadsafe_array_test_subtree (Part B aggregate)
*   XXXIV. Master-suite runners
*           - run_array_suite            (Part A only)
*           - run_threadsafe_array_suite (Part B aggregate)
*
*
*   PORTABILITY:
*   C++11 baseline - see env.h and env_cpp_features.h for the
* feature gates.  Concept-based assertions inside the bodies are
* gated on D_ENV_LANG_IS_CPP20_OR_HIGHER &&
* D_ENV_CPP_FEATURE_LANG_CONCEPTS in the .cpp files; concurrent
* tests are gated on D_ENV_LANG_IS_CPP11_OR_HIGHER (std::thread);
* shared_array tests are gated on D_ENV_LANG_IS_CPP17_OR_HIGHER
* (std::shared_mutex).  At the declaration level all functions
* have the same signature regardless of language version, so
* callers don't need to gate their own use; the bodies stub out
* gracefully where required features are unavailable.
*
*
* TABLE OF CONTENTS
* =================
* I.       SUITE TYPE ALIASES
*
* PART A:  BASE ARRAY
* II.      CATEGORY: TRAIT CONFORMANCE
* III.     CATEGORY: CONSTRUCTION
* IV.      CATEGORY: ELEMENT ACCESS
* V.       CATEGORY: ITERATION
* VI.      CATEGORY: MUTATION
* VII.     CATEGORY: BULK ALGORITHMS
* VIII.    CATEGORY: CONSTEXPR USABILITY
* IX.      CATEGORY: ITERATOR ALGORITHM INTEROP
*
* PART B:  THREADSAFE WRAPPERS
* X.       CATEGORY: WRAPPER TRAIT CONFORMANCE
* XI.      CATEGORY: THREADSAFE_ARRAY CONSTRUCTION
* XII.     CATEGORY: THREADSAFE_ARRAY LOCK-FREE QUERIES
* XIII.    CATEGORY: THREADSAFE_ARRAY SINGLE-OP ACCESS
* XIV.     CATEGORY: THREADSAFE_ARRAY HANDLE-BASED ACCESS
* XV.      CATEGORY: THREADSAFE_ARRAY BULK OPERATIONS
* XVI.     CATEGORY: THREADSAFE_ARRAY OPTIMISTIC READ
* XVII.    CATEGORY: THREADSAFE_ARRAY SNAPSHOT
* XVIII.   CATEGORY: THREADSAFE_ARRAY CONVENIENCE ALIASES
* XIX.     CATEGORY: THREADSAFE_ARRAY POLICY VARIATION
* XX.      CATEGORY: THREADSAFE_ARRAY CONCURRENT ACCESS
* XXI.     CATEGORY: ATOMIC_ARRAY CONSTRUCTION
* XXII.    CATEGORY: ATOMIC_ARRAY ELEMENT ACCESS
* XXIII.   CATEGORY: ATOMIC_ARRAY ELEMENT UPDATES
* XXIV.    CATEGORY: ATOMIC_ARRAY ELEMENT CAS
* XXV.     CATEGORY: ATOMIC_ARRAY BULK OPERATIONS
* XXVI.    CATEGORY: ATOMIC_ARRAY ITERATION
* XXVII.   CATEGORY: ATOMIC_ARRAY CONCURRENT ACCESS
* XXVIII.  CATEGORY: COW_ARRAY CONSTRUCTION
* XXIX.    CATEGORY: COW_ARRAY READ ACCESS
* XXX.     CATEGORY: COW_ARRAY SNAPSHOT
* XXXI.    CATEGORY: COW_ARRAY WRITE ACCESS
* XXXII.   CATEGORY: COW_ARRAY CONCURRENT ACCESS
* XXXIII.  CATEGORY: WRAPPER EDGE CASES
*
* PART C:  AGGREGATE BUILDERS AND RUNNERS
* XXXIV.   AGGREGATE SUBTREE BUILDERS
* XXXV.    MASTER-SUITE RUNNERS
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
#include "../../../../../inc/djinterp/test/test_session.hpp"
#include "../../../../../inc/djinterp/test/test_tree.hpp"
#include "../../../../../inc/djinterp/core/container/array/array.hpp"
#include "../../../../../inc/djinterp/core/container/array/threadsafe_array.hpp"
#include "../../../../../inc/djinterp/core/container/array/atomic_array.hpp"
#include "../../../../../inc/djinterp/core/container/array/cow_array.hpp"


// feature gates
//   the array test suite uses C++11-baseline features in its
// declarations and a wider set in its definitions.  the
// per-cpp gates (concepts, variable templates, std::thread,
// shared_mutex) live next to their use sites; only the absolute
// minimum is checked here so that consumers of the header get a
// clean diagnostic at the include site rather than in their own
// translation unit's expansion.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "array_tests.hpp requires C++11 or higher (the threadsafe "
           "wrappers depend on <atomic>, <mutex>, <thread>, and "
           "rvalue references)"
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
// default n-ary tree as the backing storage.  Every category
// function returns one of these, and the suite-level helpers
// assemble them into a single root tree representing each
// part's module.
//   The rank-validation flag is left at its default (true);
// the array suite's hierarchy never violates the rank
// invariant (assertions inside tests inside blocks inside the
// module).
using array_test_tree = djinterp::test::test_tree<
                            array_test_obj,
                            djinterp::nary_tree<array_test_obj>>;


// array_test_callable_table
//   type: the suite's deferred-evaluation callable table.
// Test functions register expensive closures (concurrent
// loops, I/O, anything that should NOT run during eager
// subtree construction) into a table of this type; the
// closures fire during the handler's tree walk, so the
// printer streams output as each test executes rather
// than only after the whole suite has finished building.
//   The table type matches array_test_obj's specialization
// so the closure signature is consistent across the suite.
using array_test_callable_table =
    djinterp::test::basic_callable_table;


// array_node_alias
//   type: a pointer to a node in the suite's tree.  Used
// by append_leaf / append_lazy_leaf to identify the
// parent under which a new leaf goes.
using array_node_alias =
    djinterp::nary_tree<array_test_obj>::node_type;


// append_leaf
//   helper: appends an assertion-kind leaf under _parent
// carrying the already-evaluated _passed result.  This is
// the EAGER path: the assertion's value is computed before
// the call (typically a trait check or arithmetic
// comparison) and the framework simply records it.
//
//   Use this for trait checks, type predicates, simple
// constant comparisons - anything that costs no measurable
// time and has no setup.
inline array_node_alias*
append_leaf(
    array_test_tree&  _tree,
    array_node_alias* _parent,
    bool              _passed,
    const char*       _name
)
{
    return _tree.underlying().append_child(
        _parent,
        djinterp::test::make_assert(_passed, _name));
}


// append_lazy_leaf
//   helper: registers _fn into _table and appends an
// assertion-kind leaf under _parent that carries the
// returned callable id.  The leaf's result is not
// evaluated by this call - it is evaluated by the handler
// during the tree walk, when it invokes the registered
// closure on the leaf's mutable test_object.
//
//   The closure must mutate the supplied test_object's
// m_result and m_status fields to reflect the test's
// outcome, exactly as the framework's evaluate(_result)
// path does for the eager case.
//
//   Use this for any test whose body does measurable
// runtime work: thread spawns, lock-protected loops,
// long arithmetic, file I/O, anything that would block
// the eager subtree-construction phase.
//
// Example:
//   append_lazy_leaf(tree, root, table,
//       "concurrent readers see consistent snapshots",
//       [](array_test_obj& self) {
//           ts_mtx_arr<int, 4> a(10, 20, 30, 40);
//           // ... thread spawns, loops, joins ...
//           bool ok = (mismatches == 0);
//           self.m_result = ok;
//           self.m_status = ok
//               ? array_test_obj::status_passed
//               : array_test_obj::status_failed;
//       });
template<typename _Fn>
inline array_node_alias*
append_lazy_leaf(
    array_test_tree&           _tree,
    array_node_alias*          _parent,
    array_test_callable_table& _table,
    const char*                _name,
    _Fn                        _fn
)
{
    djinterp::test::test_callable_id id =
        _table.register_callable(
            array_test_callable_table::callable_type{
                static_cast<_Fn&&>(_fn) });

    // start the leaf in the pending state; the closure
    // will overwrite m_result / m_status during the walk.
    array_test_obj obj(djinterp::test::D_TEST_KIND_ASSERT,
                       false,
                       _name);
    obj.set_status(array_test_obj::status_pending);
    obj.set_callable_id(id);

    return _tree.underlying().append_child(_parent, obj);
}


// =========================================================================
// PART A:  BASE ARRAY CONTAINER
// =========================================================================
//   The tests in this part exercise djinterp::array<T, N, ...>
// directly without any concurrency wrapper.  All defined in
// array_core_tests.cpp.


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
// PART B:  THREADSAFE WRAPPERS
// =========================================================================
//   The tests in this part exercise the three concurrency
// wrappers around djinterp::array:
//
//     threadsafe_array<T, N, L, I, Policy>  - lock-policy-protected
//                                             whole-array access
//     atomic_array<T, N, L, I>              - lock-free per-element
//                                             std::atomic<T> storage
//     cow_array<T, N, L, I, Policy>         - copy-on-write with
//                                             snapshot semantics
//
// All defined in threadsafe_array_core_tests.cpp.


// =========================================================================
// X.   CATEGORY: WRAPPER TRAIT CONFORMANCE
// =========================================================================
//   Verifies that every wrapper preserves the structural axes
// of the underlying array (contiguity, iterability, static
// extent, lifetime) and adds the correct concurrency strategy
// classification.  Failures here indicate a regression in the
// trait detectors, in the wrappers' specializations, or in the
// strategy tag exports.

array_test_tree test_threadsafe_array_traits_axis_preservation(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_traits_strategy_locked(
    test::test_type_id _kind);
array_test_tree test_atomic_array_traits_strategy_atomic(
    test::test_type_id _kind);
array_test_tree test_cow_array_traits_strategy_cow(
    test::test_type_id _kind);
array_test_tree test_threadsafe_traits_strategy_disjointness(
    test::test_type_id _kind);


// =========================================================================
// XI.  CATEGORY: THREADSAFE_ARRAY CONSTRUCTION
// =========================================================================
//   Default construction, parameter-pack construction, copy
// construction, plus verification that move-construction and
// move-assignment are correctly deleted (mutex non-portably-
// transferable).

array_test_tree test_threadsafe_array_default_construction(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_pack_construction(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_copy_construction(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_copy_assignment(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_move_deletion_sfinae(
    test::test_type_id _kind);


// =========================================================================
// XII. CATEGORY: THREADSAFE_ARRAY LOCK-FREE QUERIES
// =========================================================================
//   size_lockfree, version, empty_lockfree route through the
// atomic_state and must not require a lock.  These tests
// verify their values track mutations correctly even with
// null_lock_policy (the lock-acquisition cost is zero but the
// atomic_state still tracks).

array_test_tree test_threadsafe_array_size_lockfree(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_version_query(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_empty_lockfree(
    test::test_type_id _kind);


// =========================================================================
// XIII. CATEGORY: THREADSAFE_ARRAY SINGLE-OP ACCESS
// =========================================================================
//   Per-call locked accessors: size(), empty(), at(i),
// set(i,v).  Each acquires its own lock; tests verify that
// values agree with the underlying array and that mutations
// are visible across calls.

array_test_tree test_threadsafe_array_size_locked(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_at_returns_value(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_set_visible(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_set_bumps_version(
    test::test_type_id _kind);


// =========================================================================
// XIV. CATEGORY: THREADSAFE_ARRAY HANDLE-BASED ACCESS
// =========================================================================
//   read_access() and write_access() yield RAII handles that
// hold a lock for the lifetime of the handle.  Tests verify
// that handles dereference correctly, the underlying array
// is reachable through operator-> and operator*, and that the
// lock is released at scope exit.

array_test_tree test_threadsafe_array_read_access(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_write_access(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_handle_lifetime(
    test::test_type_id _kind);


// =========================================================================
// XV.  CATEGORY: THREADSAFE_ARRAY BULK OPERATIONS
// =========================================================================
//   assign() replaces contents wholesale; apply() and
// apply_read() invoke a callable under a write or read lock
// respectively; batch_guard holds a write lock across multiple
// statements.  All paths must bump the version on mutation and
// leave it untouched on read.

array_test_tree test_threadsafe_array_assign(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_apply_write(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_apply_read(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_batch_guard(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);


// =========================================================================
// XVI. CATEGORY: THREADSAFE_ARRAY OPTIMISTIC READ
// =========================================================================
//   optimistic() takes a callable, runs it without a lock,
// and validates the version stamp.  If the version changed,
// the read is retried.  Tests verify the protocol correctly
// completes when uncontested and falls through to a real
// read lock when contention exhausts the retry budget.

array_test_tree test_threadsafe_array_optimistic_uncontested(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_optimistic_fallback(
    test::test_type_id _kind);


// =========================================================================
// XVII. CATEGORY: THREADSAFE_ARRAY SNAPSHOT
// =========================================================================
//   snapshot() copies the array under a read lock and returns
// a snapshot_view that iterates without holding any lock.
// Tests verify the snapshot's content matches the source at
// the moment of capture, and that the snapshot is independent
// of subsequent mutations.

array_test_tree test_threadsafe_array_snapshot_content(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_snapshot_independence(
    test::test_type_id _kind);


// =========================================================================
// XVIII. CATEGORY: THREADSAFE_ARRAY CONVENIENCE ALIASES
// =========================================================================
//   Verifies the alias templates resolve to the expected
// concrete instantiations and that each carries the correct
// lock-policy capability flags.

array_test_tree test_threadsafe_array_alias_mutex(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_alias_timed(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_alias_shared_cpp17(
    test::test_type_id _kind);


// =========================================================================
// XIX. CATEGORY: THREADSAFE_ARRAY POLICY VARIATION
// =========================================================================
//   Same logical operations across all available lock
// policies — null, exclusive, timed, shared — verifying that
// behavior is uniform and that the policy-specific
// capabilities (supports_shared, supports_timed) are reported
// correctly through the CRTP base.

array_test_tree test_threadsafe_array_policy_null(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_policy_exclusive(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_policy_timed(
    test::test_type_id _kind);
array_test_tree test_threadsafe_array_policy_shared_cpp17(
    test::test_type_id _kind);


// =========================================================================
// XX.  CATEGORY: THREADSAFE_ARRAY CONCURRENT ACCESS
// =========================================================================
//   Multi-threaded smoke and stress tests: concurrent
// readers do not corrupt state, concurrent writers
// serialize correctly, version is monotonically
// non-decreasing under writer load.

array_test_tree test_threadsafe_array_concurrent_readers(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);
array_test_tree test_threadsafe_array_concurrent_writers(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);
array_test_tree test_threadsafe_array_concurrent_mixed(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);
array_test_tree test_threadsafe_array_concurrent_version_monotonic(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);


// =========================================================================
// XXI. CATEGORY: ATOMIC_ARRAY CONSTRUCTION
// =========================================================================
//   Default construction zero-initializes via store(); fill
// constructor stores the supplied value to every slot;
// copy/move are deleted because std::atomic<T> is non-
// copyable.

array_test_tree test_atomic_array_default_construction(
    test::test_type_id _kind);
array_test_tree test_atomic_array_fill_construction(
    test::test_type_id _kind);
array_test_tree test_atomic_array_copy_move_deletion_sfinae(
    test::test_type_id _kind);


// =========================================================================
// XXII. CATEGORY: ATOMIC_ARRAY ELEMENT ACCESS
// =========================================================================
//   load(i), store(i, v), exchange(i, v) — each accepts an
// optional memory_order.  Tests verify round-trip values,
// independence of distinct slots, and that exchange()
// returns the prior value.

array_test_tree test_atomic_array_load_store(
    test::test_type_id _kind);
array_test_tree test_atomic_array_exchange(
    test::test_type_id _kind);
array_test_tree test_atomic_array_memory_orderings(
    test::test_type_id _kind);
array_test_tree test_atomic_array_slot_independence(
    test::test_type_id _kind);


// =========================================================================
// XXIII. CATEGORY: ATOMIC_ARRAY ELEMENT UPDATES
// =========================================================================
//   fetch_* family: returns the prior value and applies the
// arithmetic / bitwise update atomically.

array_test_tree test_atomic_array_fetch_add(
    test::test_type_id _kind);
array_test_tree test_atomic_array_fetch_sub(
    test::test_type_id _kind);
array_test_tree test_atomic_array_fetch_and(
    test::test_type_id _kind);
array_test_tree test_atomic_array_fetch_or(
    test::test_type_id _kind);
array_test_tree test_atomic_array_fetch_xor(
    test::test_type_id _kind);


// =========================================================================
// XXIV. CATEGORY: ATOMIC_ARRAY ELEMENT CAS
// =========================================================================
//   compare_exchange_weak / compare_exchange_strong:
// success returns true and leaves _expected unchanged;
// failure returns false and updates _expected to the
// observed value.

array_test_tree test_atomic_array_cas_strong_success(
    test::test_type_id _kind);
array_test_tree test_atomic_array_cas_strong_failure(
    test::test_type_id _kind);
array_test_tree test_atomic_array_cas_weak_loop(
    test::test_type_id _kind);


// =========================================================================
// XXV. CATEGORY: ATOMIC_ARRAY BULK OPERATIONS
// =========================================================================
//   size, empty, fill, is_lock_free.  fill is per-element
// atomic, NOT a coherent whole-array operation — tests
// verify only that all slots end up at the supplied value.

array_test_tree test_atomic_array_size_empty(
    test::test_type_id _kind);
array_test_tree test_atomic_array_fill(
    test::test_type_id _kind);
array_test_tree test_atomic_array_is_lock_free(
    test::test_type_id _kind);


// =========================================================================
// XXVI. CATEGORY: ATOMIC_ARRAY ITERATION
// =========================================================================
//   begin/end/cbegin/cend yield std::atomic<T>* — callers
// operate on each slot through its atomic interface.  data()
// returns the underlying atomic_value_type pointer.

array_test_tree test_atomic_array_begin_end(
    test::test_type_id _kind);
array_test_tree test_atomic_array_data_pointer(
    test::test_type_id _kind);
array_test_tree test_atomic_array_range_based_for(
    test::test_type_id _kind);


// =========================================================================
// XXVII. CATEGORY: ATOMIC_ARRAY CONCURRENT ACCESS
// =========================================================================
//   Multi-threaded fetch_add stress test: every thread
// increments N times; the final sum must equal threads * N.
// Verifies the per-element atomicity guarantee under
// genuine contention.

array_test_tree test_atomic_array_concurrent_fetch_add(
    test::test_type_id _kind);
array_test_tree test_atomic_array_concurrent_disjoint_slots(
    test::test_type_id _kind);
array_test_tree test_atomic_array_concurrent_cas_loop(
    test::test_type_id _kind);


// =========================================================================
// XXVIII. CATEGORY: COW_ARRAY CONSTRUCTION
// =========================================================================
//   Default construction yields an empty-state cow_state;
// from-array construction copies the supplied array into
// the state; copy/move are deleted (mutex inside).

array_test_tree test_cow_array_default_construction(
    test::test_type_id _kind);
array_test_tree test_cow_array_from_array_construction(
    test::test_type_id _kind);
array_test_tree test_cow_array_copy_move_deletion_sfinae(
    test::test_type_id _kind);


// =========================================================================
// XXIX. CATEGORY: COW_ARRAY READ ACCESS
// =========================================================================
//   read() returns a reference to the current array; size /
// empty / at(i) are convenience wrappers.

array_test_tree test_cow_array_read_returns_value(
    test::test_type_id _kind);
array_test_tree test_cow_array_size_empty(
    test::test_type_id _kind);
array_test_tree test_cow_array_at_returns_copy(
    test::test_type_id _kind);


// =========================================================================
// XXX. CATEGORY: COW_ARRAY SNAPSHOT
// =========================================================================
//   snapshot() returns an immutable_snapshot whose lifetime
// outlives subsequent writes.  Tests verify content fidelity
// at capture time, independence from later mutations, and
// version stamping behavior.

array_test_tree test_cow_array_snapshot_content(
    test::test_type_id _kind);
array_test_tree test_cow_array_snapshot_survives_write(
    test::test_type_id _kind);
array_test_tree test_cow_array_snapshot_version(
    test::test_type_id _kind);
array_test_tree test_cow_array_multiple_snapshots(
    test::test_type_id _kind);


// =========================================================================
// XXXI. CATEGORY: COW_ARRAY WRITE ACCESS
// =========================================================================
//   modify(fn) clones the array, applies fn, and atomically
// publishes the result; replace() swaps in a new array;
// set(i, v) is the single-element shortcut.  Every write
// bumps the version monotonically.

array_test_tree test_cow_array_modify(
    test::test_type_id _kind);
array_test_tree test_cow_array_replace(
    test::test_type_id _kind);
array_test_tree test_cow_array_set_single(
    test::test_type_id _kind);
array_test_tree test_cow_array_version_monotonic(
    test::test_type_id _kind);


// =========================================================================
// XXXII. CATEGORY: COW_ARRAY CONCURRENT ACCESS
// =========================================================================
//   Concurrent snapshots vs writers: snapshots are
// self-consistent and writers serialize correctly under
// the configured lock policy.

array_test_tree test_cow_array_concurrent_snapshots(
    test::test_type_id _kind);
array_test_tree test_cow_array_concurrent_writers(
    test::test_type_id _kind);


// =========================================================================
// XXXIII. CATEGORY: WRAPPER EDGE CASES
// =========================================================================
//   Cross-cutting cases for all three wrappers: zero-extent,
// single-element, large extents, and (where supported)
// non-trivially-copyable element types.

array_test_tree test_threadsafe_edge_zero_extent(
    test::test_type_id _kind);
array_test_tree test_threadsafe_edge_single_element(
    test::test_type_id _kind);
array_test_tree test_threadsafe_edge_large_extent(
    test::test_type_id _kind);


// =========================================================================
// PART C:  AGGREGATE BUILDERS AND RUNNERS
// =========================================================================


// =========================================================================
// XXXIV. AGGREGATE SUBTREE BUILDERS
// =========================================================================
//   Each builder invokes every category function in its part
// in declaration order and assembles the returned subtrees as
// children of a single module-level root, returning the
// resulting tree by value.
//   The returned tree is the part's "test module" - a rooted
// tree whose root carries the module's type_id, whose
// immediate children are category-level subtrees, and whose
// leaves are the assertion-level outcomes produced inside
// each category function.
//
//   The functions neither know about nor require a
// test_handler or test_printer.  The caller hands the
// returned tree to whichever handler is configured (see
// section XXXV for the master runners) and the handler's
// bound listeners drive the output.
//
//   Five builders are exposed.  The two top-level builders
// cover whole parts of the suite:
//     - make_array_test_subtree            : Part A only
//     - make_threadsafe_array_test_subtree : Part B only
//   Three per-container sub-builders let callers drive any
// single threadsafe-wrapper module in isolation:
//     - make_threadsafe_array_subtree : threadsafe_array only
//                                       (also includes the
//                                       cross-cutting axis
//                                       preservation, strategy
//                                       disjointness, and edge
//                                       case tests)
//     - make_atomic_array_subtree     : atomic_array only
//     - make_cow_array_subtree        : cow_array only
//   make_threadsafe_array_test_subtree is the suite-wide
// aggregate; it calls the three sub-builders and grafts each
// under one suite root.
//   Callers wanting a combined Part A + Part B run can graft
// both top-level subtrees under their own root (see
// test_tree::graft) or simply invoke the two runners back-to-
// back.
//
// Parameter(s):
//   _kind:  the test_type_id stamped on the module root.
//           Defaults to the framework's MODULE kind constant
//           (D_TEST_KIND_MODULE) so that simple users don't
//           need to learn the kind taxonomy on day one.

// make_array_test_subtree
//   builds the subtree for Part A (base array container).
array_test_tree
make_array_test_subtree(
    test::test_type_id _kind = test::D_TEST_KIND_MODULE);

// make_threadsafe_array_subtree
//   builds the subtree for the threadsafe_array container, plus
// the cross-cutting trait and edge-case tests.
//   Takes a callable table because the threadsafe_array module
// includes runtime-heavy concurrent tests whose work is wrapped
// in closures registered into the table; those closures fire
// during the handler walk, not during this builder's call.
array_test_tree
make_threadsafe_array_subtree(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);

// make_atomic_array_subtree
//   builds the subtree for the atomic_array container.
array_test_tree
make_atomic_array_subtree(
    test::test_type_id _kind = test::D_TEST_KIND_MODULE);

// make_cow_array_subtree
//   builds the subtree for the cow_array container.
array_test_tree
make_cow_array_subtree(
    test::test_type_id _kind = test::D_TEST_KIND_MODULE);

// make_threadsafe_array_test_subtree
//   builds the suite-wide aggregate for Part B (all three
// threadsafe wrappers combined under one suite root).
//   Takes a callable table forwarded to make_threadsafe_array_
// subtree (the only sub-builder that currently has lazy leaves).
array_test_tree
make_threadsafe_array_test_subtree(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);

// make_combined_test_subtree
//   builds a single tree containing both Part A (base array
// container) and Part B (threadsafe wrappers) grafted under
// one suite root.  Convenient when a caller wants one
// run that prints both modules' results in document order
// against a single handler / printer.
//   Equivalent in coverage to running the two part-level
// runners back-to-back, but emits a single rooted tree so
// that any printer or post-processor sees the whole suite
// as one structural unit.
//   Takes a callable table forwarded into Part B's builder.
array_test_tree
make_combined_test_subtree(
    test::test_type_id          _kind,
    array_test_callable_table&  _table);


// =========================================================================
// XXXV. MASTER-SUITE RUNNERS
// =========================================================================
//   Drivers: each builds its part's subtree via the matching
// builder above, passes it to the supplied handler's run()
// method, and returns the three-way verdict from the
// handler's session_result.
//   The handler's listener bundle (lifecycle + any value-
// tagged listeners) determines what gets emitted to a
// printer, log, or other sink - the runners do not touch
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
//                 root.  Forwarded to the matching builder.
//   _out_seconds: optional; receives the wall-clock duration
//                 of the run if non-null.
// Return:
//   The session_verdict for this run.

// run_array_suite
//   drives Part A (base array container).
test::session_verdict
run_array_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind        = test::D_TEST_KIND_MODULE,
    double*             _out_seconds = nullptr);

// run_threadsafe_array_suite
//   drives Part B (threadsafe wrappers).
test::session_verdict
run_threadsafe_array_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind        = test::D_TEST_KIND_MODULE,
    double*             _out_seconds = nullptr);

// run_combined_suite
//   drives both Part A and Part B in one walk against the
// supplied handler.  Builds via make_combined_test_subtree
// and returns the unified verdict.  Useful when wiring up
// CI or a developer-facing one-shot runner that should print
// every module's results together.
test::session_verdict
run_combined_suite(
    test::test_handler& _handler,
    test::test_type_id  _kind        = test::D_TEST_KIND_MODULE,
    double*             _out_seconds = nullptr);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_ARRAY_TESTS_
