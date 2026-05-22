/******************************************************************************
* djinterp [test]                                              dtuple_tests.hpp
*
*   Unit-test suite for the djinterp tuple metafunction module
* (`dtuple.hpp`).  Tests are split across multiple translation units, one
* per semantic section of the module under test.
*
*   STRUCTURE:
*   The bulk of dtuple is compile-time metaprogramming.  Every section's
* .cpp file places its assertions at file scope as `static_assert`s, so a
* regression in any trait blocks the build with a descriptive diagnostic.
* Each section's `dtuple_tests_*` entry point then mirrors the
* static_assert as a runtime `record_assertion(..., true, ...)` call so
* the assertion appears in the framework's report, and runs genuine
* runtime checks for the four runtime functions in dtuple (`tuple_concat`,
* `tuple_type_at_value`, `tuple_to_pack`, `is_homogeneous`).
*
*   API:
*   Each per-section runner is a free function with the uniform
* signature `void(test_handler&)`.  That signature matches the
* `void(*)(test_handler&)` function-pointer parameter expected by
* `dtuple_tests_runner.cpp::run_module`, so each runner can be passed
* directly to `run_module` without a wrapper.  Assertions are routed
* into the handler via `record_assertion` (an ADL-visible free function
* template declared in `djinterp::test`).
*
*   PORTABILITY:
*   C++11 minimum (matching dtuple.hpp itself).  Variable-template `_v`
* checks are gated on `D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES` at the
* point of use inside each .cpp.
*
*
* TABLE OF CONTENTS
* =================
* I.    INCLUDES
* II.   SHARED HELPER TYPES
* III.  PER-SECTION WORKER DECLARATIONS
*
*
* path:      /inc/djinterp/test/dtuple_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_DTUPLE_TESTS_
#define DJINTERP_DTUPLE_TESTS_ 1

// std
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include <djinterp/core/meta/dtuple.hpp>
#include <djinterp/test/test_defaults.hpp>
#include <djinterp/test/test_handler.hpp>
#include <djinterp/test/test_options.hpp>
#include <djinterp/test/test_object.hpp>


NS_DJINTERP
NS_TESTING

// pull the two framework types this header's declarations need into
// djinterp::testing.  test_handler lives in djinterp::test (see
// test_handler.hpp); test_option lives in djinterp::test
// (see test_options.hpp).  record_assertion is reached via ADL on
// _test_handler inside each section's .cpp, so it does not need a using.
using djinterp::test::test_handler;
using djinterp::test::test_option;


///////////////////////////////////////////////////////////////////////////////
///                II.  SHARED HELPER TYPES                                 ///
///////////////////////////////////////////////////////////////////////////////

// dtuple_test_types
//   namespace: tag types and small metafunction helpers shared across
// every dtuple test section.  Kept inside a nested namespace so the
// individual section translation units can pull them in via a single
// `using namespace ...` directive without polluting `djinterp::testing`.
namespace dtuple_test_types
{

// alpha / bravo / charlie / delta
//   types: empty tag types used as test inputs whose only purpose is
// to be distinct from every standard library type and from each other.
struct alpha   {};
struct bravo   {};
struct charlie {};
struct delta   {};

// identity
//   trait: minimal unary alias template that returns its argument
// unchanged.  Useful as a no-op when a `template<typename> typename`
// argument is required (e.g. `tuple_apply_all<identity, ...>`) but the
// test only cares about the structural machinery, not the per-element
// transformation.
//   NOTE: alias template form (not class template).  `tuple_apply_all`
// in dtuple.hpp stores `_UnaryTrait<_Head>` directly as a type rather
// than extracting `::type`, so the trait argument must itself BE the
// resulting type when instantiated.  See also `wrap_all`, which uses
// the opposite convention (class templates with `::type`).
template<typename _Type>
using identity = _Type;

// add_array_2
//   trait: unary alias template that wraps `_Type` in a fixed-size
// `std::array` of length 2.  Used by tuple_apply_all tests as a
// non-trivial transformation whose output type differs from the input.
template<typename _Type>
using add_array_2 = std::array<_Type, 2>;

// identity_st
//   trait: class-template form of `identity` for the rare test that
// requires a struct-with-::type shape (currently only `wrap_all`
// tests, which call `_Modifier<X>::type`).
template<typename _Type>
struct identity_st
{
    using type = _Type;
};

} // namespace dtuple_test_types


///////////////////////////////////////////////////////////////////////////////
///                III. PER-SECTION WORKER DECLARATIONS                     ///
///////////////////////////////////////////////////////////////////////////////
//   Each `dtuple_tests_*_all` symbol is the entry point implemented in
// the corresponding .cpp file.  Signature matches the function-pointer
// parameter of `dtuple_tests_runner.cpp::run_module`, which is
// `void(*)(test_handler&)`.  Sections that need test-option awareness
// should consume options through fields on the handler rather than
// widening this signature, so the runner-side contract stays stable.
void dtuple_tests_pack_all(test_handler& _test_handler);
void dtuple_tests_construction_all(test_handler& _test_handler);
void dtuple_tests_modifiers_all(test_handler& _test_handler);
void dtuple_tests_transformation_all(test_handler& _test_handler);
void dtuple_tests_access_all(test_handler& _test_handler);
void dtuple_tests_counting_all(test_handler& _test_handler);
void dtuple_tests_splitting_all(test_handler& _test_handler);
void dtuple_tests_utilities_all(test_handler& _test_handler);
void dtuple_tests_selection_all(test_handler& _test_handler);
void dtuple_tests_homogeneity_all(test_handler& _test_handler);
void dtuple_tests_2d_all(test_handler& _test_handler);
void dtuple_tests_relations_all(test_handler& _test_handler);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_DTUPLE_TESTS_