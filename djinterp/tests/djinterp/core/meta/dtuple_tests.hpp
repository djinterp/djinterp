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
* signature `void(test_handler&)`.  Assertions are routed into the handler
* via `record_assertion` (an ADL-visible free function template declared in
* `djinterp::test`).  The runner does not drive these workers directly:
* `dtuple_spec()` (Section IV) exposes the whole suite as a `module_spec`
* -- plain data, one `{ name, descriptor, bool() }` unit test per section --
* and `run_module` (test_defaults.hpp) lowers that into both the six-kind
* test tree and the report / PDF.  Each unit test's `bool()` is a thin
* adapter (`dtuple_detail::run_section`) over the matching worker.
*
*   DTEST_SPEC_MODE:
*   A runner that consumes the suite as data defines DTEST_SPEC_MODE before
* including this header (mirroring the other DTest suites).  In that mode
* the shared fixtures (Section II) are dropped -- the runner needs only the
* worker declarations and `dtuple_spec()`.  The section .cpp files are
* compiled WITHOUT the macro and supply both the fixtures they use and the
* worker definitions the spec's function pointers resolve to.
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
* II.   SHARED HELPER TYPES            (dropped under DTEST_SPEC_MODE)
* III.  PER-SECTION WORKER DECLARATIONS
* IV.   MODULE SPEC                    (dtuple_spec, run_section adapter)
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
//   These are the section .cpp files' fixtures.  A runner that consumes the
// suite as data (see DTEST_SPEC_MODE below) does not need them -- only the
// worker declarations and the spec provider -- so they are compiled out in
// that mode.  The section .cpp files are compiled WITHOUT the macro and get
// them.
#ifndef DTEST_SPEC_MODE

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

#endif  // !DTEST_SPEC_MODE


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


///////////////////////////////////////////////////////////////////////////////
///                IV.  MODULE SPEC  (data view for the runner)             ///
///////////////////////////////////////////////////////////////////////////////
//   The framework's one-call runner (`run_module`, in test_defaults.hpp)
// consumes a `module_spec`: plain data describing a module as blocks of
// unit tests, each unit test a `{ name, descriptor, bool() }` triple.
// run_module lowers that spec into the six-kind test tree AND projects it
// onto the console report / PDF, so the suite is authored once as data and
// feeds both views.  `dtuple_spec()` below is what the runner hands over
// (mirroring the other DTest suites, whose providers live in
// `djinterp::testing`).
//
//   Each `dtuple_tests_*_all` worker is one aggregate unit test: it records
// many assertions but represents a single pass/fail, which is exactly what
// a `test_spec` wants.  The signatures differ, though -- the spec needs
// `bool()`, the workers are `void(test_handler&)` -- so `run_section`
// bridges them: it runs one worker against a private handler and returns
// whether that run added no failures or errors.  A function-pointer non-type
// template parameter yields one distinct thunk per worker without twelve
// hand-written wrappers.
//
//   The `module_spec` / `block_spec` / `test_spec` types and
// `default_test_handler` live in `djinterp::test`; they are named with
// explicit qualification here so the surrounding `djinterp::testing` stays
// uncluttered.

namespace dtuple_detail
{

// run_section
//   adapter: presents one `void(test_handler&)` worker as a nullary
// `bool()` predicate.  Runs the worker against a fresh default_test_handler
// and returns true iff it recorded no new failures and no new errors.  The
// worker still pushes its per-assertion leaves into that private handler,
// so nothing about a section's own recording changes -- only the verdict is
// surfaced, which is all a test_spec consumes.
template<void (*_Worker)(test_handler&)>
inline bool
run_section()
{
    djinterp::test::default_test_handler _handler;

    _Worker(_handler);

    return ( (_handler.failed() == 0) &&
             (_handler.errors() == 0) );
}

}  // namespace dtuple_detail


// dtuple_spec
//   provider: the whole dtuple suite as one module_spec -- a single block
// whose twelve unit tests are the per-section workers, in the same declared
// order as above.  Hand this to run_module (see the runner) to drive both
// the six-kind tree and the report / PDF.
//   Kept inline in the header so the runner needs only this file: the
// section .cpp translation units supply the worker definitions the
// function-pointer template arguments resolve to at link time.
inline djinterp::test::module_spec
dtuple_spec()
{
    djinterp::test::module_spec _module;

    _module.name       = "dtuple";
    _module.descriptor =
        "Compile-time tuple metafunction module: parameter-pack utilities, "
        "tuple construction / modification / transformation, element access, "
        "counting and filtering, splitting, selection, homogeneity, and the "
        "2D / jagged-tuple surface, plus the four runtime helpers.";

    djinterp::test::block_spec _block;

    _block.name       = "dtuple";
    _block.descriptor =
        "One unit test per semantic section; each aggregates its section's "
        "compile-time asserts and runtime checks into a single verdict.";

    _block.tests = std::vector<djinterp::test::test_spec>{
        { "pack",
          "II   parameter-pack utilities: is_tuple_single_arg",
          &dtuple_detail::run_section<&dtuple_tests_pack_all> },
        { "construction",
          "V    tuple construction: to_tuple, make_tuple_of",
          &dtuple_detail::run_section<&dtuple_tests_construction_all> },
        { "modifiers",
          "VI   type modifiers: wrap_all, to_lvalue/rvalue_reference, "
          "to_pointer, to_type, tuple_join",
          &dtuple_detail::run_section<&dtuple_tests_modifiers_all> },
        { "transformation",
          "II   tuple transformation: tuple_apply_all",
          &dtuple_detail::run_section<&dtuple_tests_transformation_all> },
        { "access",
          "III  element access: tuple_type_at[_value], tuple_concat",
          &dtuple_detail::run_section<&dtuple_tests_access_all> },
        { "counting",
          "IV   counting / filtering: tuple_consolidate_types, "
          "tuple_count_and_remove, tuple_count_type",
          &dtuple_detail::run_section<&dtuple_tests_counting_all> },
        { "splitting",
          "V    tuple splitting: tuple_split, tuple_subsequence",
          &dtuple_detail::run_section<&dtuple_tests_splitting_all> },
        { "utilities",
          "VI   tuple utilities: tuple_to_pack",
          &dtuple_detail::run_section<&dtuple_tests_utilities_all> },
        { "selection",
          "VII  type selection: type_case, type_selector, type_select_t, "
          "type_matched_v",
          &dtuple_detail::run_section<&dtuple_tests_selection_all> },
        { "homogeneity",
          "VIII tuple homogeneity: is_tuple_homogeneous, is_homogeneous",
          &dtuple_detail::run_section<&dtuple_tests_homogeneity_all> },
        { "2d",
          "IX   2D / jagged tuples: is_2d_tuple, is_jagged_tuple, "
          "is_uniform_2d_tuple, make_2d_tuple_of, tuple_flatten_types, "
          "tuple_inner_sizes / outer_size, tuple_row_type / size, "
          "tuple_total_elements, tuple_common_element_type",
          &dtuple_detail::run_section<&dtuple_tests_2d_all> },
        { "relations",
          "IX   tuple relations: normalize_tuple, tuple_all_elements_same_as, "
          "all_inner_tuple_elements_one_type",
          &dtuple_detail::run_section<&dtuple_tests_relations_all> }
    };

    _module.blocks.push_back(_block);

    return _module;
}


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_DTUPLE_TESTS_