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
*   Each top-level test category is exposed as a free function
* that takes a `std::vector<basic_test>&` output sink and a
* `test_type_id` to stamp on every produced test_object.  The
* functions append zero or more basic_test values to the sink
* and never throw.  All failures are encoded as basic_test
* objects with status_failed; the caller decides whether to
* surface them via test_session, write them to a CSV, or treat a
* non-empty failure set as a non-zero process exit code.
*
*   The full driver `make_array_test_objects()` runs every
* category in order and returns the aggregated vector.
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
*   C++11 baseline.  Concept-based assertions are gated on
* D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
* in the .cpp file; at the declaration level all functions have
* the same signature regardless of language version, so callers
* don't need to gate their own use.
*
*
* path:      /tests/djinterp/core/container/array/array_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.25
******************************************************************************/

#ifndef DJINTERP_TEST_ARRAY_TESTS_
#define DJINTERP_TEST_ARRAY_TESTS_ 1

// std
#include <chrono>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../../inc/djinterp/core/djinterp.hpp"
#include "../../../../../inc/djinterp/core/env/cpp/env_cpp_features.h"
#include "../../../../../inc/djinterp/test/test_common.hpp"
#include "../../../../../inc/djinterp/test/test_object.hpp"
#include "../../../../../inc/djinterp/test/test_printer.hpp"
#include "../../../../../inc/djinterp/test/test_handler.hpp"
#include "../../../../../inc/djinterp/test/test_defaults.hpp"
#include "../../../../../inc/djinterp/core/container/array/array.hpp"
#include "../../../../../inc/djinterp/core/container/array/array_traits.hpp"
#include "../../../../../inc/djinterp/core/container/array/array_iterator.hpp"
#include "../../../../../inc/djinterp/core/container/iterator/iterator_traits.hpp"
#include "../../../../../inc/djinterp/core/container/iterator/constexpr_iterator.hpp"
#include "../../../../../inc/djinterp/core/container/iterator/constexpr_iterator_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/constexpr_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/runtime_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/mutable_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/iterable_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/bounded_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/sorted_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/flat_hierarchical_container_traits.hpp"
#include "../../../../../inc/djinterp/core/container/meta/container_storage_traits.hpp"


NS_DJINTERP
NS_TESTING


using djinterp::test::basic_test;
using djinterp::test::test_type_id;
using djinterp::test::test_printer;
using djinterp::test::test_handler;
using djinterp::test::session_result;
using djinterp::test::unit_test_tally;

// ===========================================================================
// I.   Compile-time trait conformance
// ===========================================================================
//   Verifies that every cell of the lifetime × iterability cube
// reports the trait flags that array_traits.hpp + the
// classification_traits modules say it should.  Failures here
// indicate a regression in the trait detectors OR in the array's
// public interface (an unexpected method exposed/hidden).
void test_array_axis_constexpr_runtime(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_axis_mutable_immutable(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_axis_iterable_non_iterable(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_axis_bounded(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_axis_sorted_unsorted(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_axis_flat_hierarchical(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_axis_storage_kind(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_lifetime_taxonomy(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// II.  Core construction and destruction
// ===========================================================================
//   Default construction, parameter-pack construction, copy / move,
// plus the two extent edge cases (extent=0 and extent=1).  These
// tests trigger constructor selection for every cell of the cube.
void test_array_default_construction(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_pack_construction(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_copy_construction(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_move_construction(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_zero_extent_edge_case(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_single_extent_edge_case(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// III. Element access
// ===========================================================================
//   Verifies operator[], at(), front(), back(), and data() across
// const and non-const overloads.  Edge case: a one-element array
// has front() == back().
void test_array_subscript_access(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_at_access(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_front_back_access(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_data_access(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_const_access_paths(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// IV.  Iteration
// ===========================================================================
//   begin/end round-trips, range-based for, reverse iteration, and
// SFINAE absence of begin() on non-iterable cells.
void test_array_begin_end(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_const_iteration(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_reverse_iteration(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_range_based_for(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_non_iterable_sfinae(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// V.   Mutation
// ===========================================================================
//   Mutators on mutable cells; SFINAE absence on immutable cells.
void test_array_subscript_assignment(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_fill(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_member_swap(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_immutable_sfinae(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// VI.  Free-function bulk algorithms
// ===========================================================================
void test_array_equal_function(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_copy_function(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_swap_function(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// VII. Constexpr usability
// ===========================================================================
//   Demonstrates that an array can be constructed, accessed, and
// compared inside a constant expression.  C++14+ also exercises
// the relaxed-constexpr mutator path.
void test_array_constexpr_construction(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_constexpr_access(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);
void test_array_constexpr_mutation_cpp14(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// VIII. Iterator algorithm interop
// ===========================================================================
void test_array_constexpr_iterator_algorithms(std::vector<basic_test>& _out, test_handler& _handler, test_type_id _kind);


// ===========================================================================
// IX.  Aggregate driver
// ===========================================================================

// make_array_test_objects
//   driver: invokes every category above in declaration order and
// returns the aggregated vector of basic_test results.  `_kind`
// is propagated to every produced test_object so the suite can be
// distinguished from peers in a multi-suite test_session.
std::vector<basic_test> make_array_test_objects(test_handler&     _handler,
                                                unit_test_tally&  _tally,
                                                test_type_id      _kind = 0);


// ===========================================================================
// X.   Master-suite runner
// ===========================================================================
//   Executes the array test suite against a caller-supplied
// printer and returns true iff every produced test_object passed.
// The optional `_result` output parameter receives leaf-only
// assertion tallies; pass nullptr if those tallies aren't needed.
//
//   The runner does NOT configure the printer's sink — that is
// the caller's choice (string, stdout, file, custom sink).  This
// keeps the runner reusable in both interactive (stdout) and
// captured (string-buffered, then templated into a master report)
// contexts.
bool run_array_suite(test_printer&    _printer,
                     session_result*  _out_totals  = nullptr,
                     unit_test_tally* _out_units   = nullptr,
                     double*          _out_seconds = nullptr);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_ARRAY_TESTS_