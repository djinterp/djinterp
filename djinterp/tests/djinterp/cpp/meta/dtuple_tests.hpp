/*******************************************************************************
* djinterp [test]                                               dtuple_tests.hpp
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
*   Each per-section runner is a `bool()` unit test. The spec provider,
* dtuple_spec(), lists them for `run_module` (dtuple_tests_runner.cpp).
* Assertions are recorded on a local verdict through this header's
* `record_assertion(bool&, bool, const char*)`.
*
*   PORTABILITY:
*   C++11 minimum (matching dtuple.hpp itself).  Variable-template `_v`
* checks are gated on `D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES` at the
* point of use inside each .cpp.
*
*
* path:      /tests/djinterp/cpp/meta/dtuple_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.19
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    INCLUDES
      --------

II.   SHARED HELPER TYPES
      -------------------

III.  PER-SECTION WORKER DECLARATIONS
      -------------------------------
*/

#ifndef DJINTERP_DTUPLE_TESTS_
#define DJINTERP_DTUPLE_TESTS_ 1

// std
#include <cstdio>
#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include <djinterp/cpp/meta/dtuple.hpp>
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// record_assertion
//   function: one check of a converted body: on failure it prints what was
// expected and clears the body's verdict.  (The bodies used DTest's retired
// handler API, record_assertion(handler, condition, what).)
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what
)
{
    if (!_condition)
    {
        std::printf("    ! %s\n", _what);
        _ok = false;
    }

    return;
}


// record_assertion (with detail)
//   function: the five-argument form the bodies also used: the detail and
// the extra text are printed with a failure.
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what,
    const char* _detail,
    const char* _extra = nullptr
)
{
    if (!_condition)
    {
        std::printf("    ! %s (%s%s%s)\n",
                    _what,
                    _detail ? _detail : "",
                    _extra ? "; " : "",
                    _extra ? _extra : "");
        _ok = false;
    }

    return;
}

// DUnitTestTally
//   struct: kept for the converted bodies' run_unit_test calls; a body's
// verdict is its bool now, so the tally holds nothing.
struct DUnitTestTally
{
};

// run_unit_test
//   function: runs one named unit inside a converted body, and prints its
// name when it was the unit that first failed.
template<typename Fn>
void run_unit_test(
    bool&           _ok,
    DUnitTestTally& _tally,
    const char*     _name,
    Fn              _unit
)
{
    const bool before = _ok;

    (void)_tally;
    _unit();

    if ( (before) &&
         (!_ok) )
    {
        std::printf("    [FAIL] %s\n", _name);
    }

    return;
}

// D_DTUPLE_CHECK
//   macro: one check of a converted body, its text the message.
#define D_DTUPLE_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)


// pull the two framework types this header's declarations need into
// djinterp::testing.  test_handler lives in djinterp::test (see
// test_handler.hpp); test_option lives in djinterp::test
// (see test_options.hpp).  record_assertion is reached via ADL on
// _test_handler inside each section's .cpp, so it does not need a using.


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
// resulting type when instantiated.  See also `DWrapAll`, which uses
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
// requires a struct-with-::type shape (currently only `DWrapAll`
// tests, which call `_Modifier<X>::type`).
template<typename _Type>
struct identity_st
{
    using type = _Type;
};

} // namespace dtuple_test_types
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                III. PER-SECTION WORKER DECLARATIONS                     ///
///////////////////////////////////////////////////////////////////////////////
//   Each `dtuple_tests_*_all` symbol is the entry point implemented in
// the corresponding .cpp file.  Signature matches the function-pointer
// parameter of `dtuple_tests_runner.cpp::run_module`, which is
// `void(*)(test_handler&)`.  Sections that need test-option awareness
// should consume options through fields on the handler rather than
// widening this signature, so the runner-side contract stays stable.
bool dtuple_tests_pack_all();
bool dtuple_tests_construction_all();
bool dtuple_tests_modifiers_all();
bool dtuple_tests_transformation_all();
bool dtuple_tests_access_all();
bool dtuple_tests_counting_all();
bool dtuple_tests_splitting_all();
bool dtuple_tests_utilities_all();
bool dtuple_tests_selection_all();
bool dtuple_tests_homogeneity_all();
bool dtuple_tests_2d_all();
bool dtuple_tests_relations_all();


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// dtuple_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
dtuple_spec()
{
    return dt::module_spec{
        "dtuple",
        "dtuple.hpp",
        {
            dt::block_spec{
                "pack",
                "Pack",
                {
                    { "dtuple_tests_pack_all",
                      "dtuple pack all",
                      &dtuple_tests_pack_all },
                }
            },
            dt::block_spec{
                "construction",
                "Construction",
                {
                    { "dtuple_tests_construction_all",
                      "dtuple construction all",
                      &dtuple_tests_construction_all },
                }
            },
            dt::block_spec{
                "modifiers",
                "Modifiers",
                {
                    { "dtuple_tests_modifiers_all",
                      "dtuple modifiers all",
                      &dtuple_tests_modifiers_all },
                }
            },
            dt::block_spec{
                "transformation",
                "Transformation",
                {
                    { "dtuple_tests_transformation_all",
                      "dtuple transformation all",
                      &dtuple_tests_transformation_all },
                }
            },
            dt::block_spec{
                "access",
                "Access",
                {
                    { "dtuple_tests_access_all",
                      "dtuple access all",
                      &dtuple_tests_access_all },
                }
            },
            dt::block_spec{
                "counting",
                "Counting",
                {
                    { "dtuple_tests_counting_all",
                      "dtuple counting all",
                      &dtuple_tests_counting_all },
                }
            },
            dt::block_spec{
                "splitting",
                "Splitting",
                {
                    { "dtuple_tests_splitting_all",
                      "dtuple splitting all",
                      &dtuple_tests_splitting_all },
                }
            },
            dt::block_spec{
                "utilities",
                "Utilities",
                {
                    { "dtuple_tests_utilities_all",
                      "dtuple utilities all",
                      &dtuple_tests_utilities_all },
                }
            },
            dt::block_spec{
                "selection",
                "Selection",
                {
                    { "dtuple_tests_selection_all",
                      "dtuple selection all",
                      &dtuple_tests_selection_all },
                }
            },
            dt::block_spec{
                "homogeneity",
                "Homogeneity",
                {
                    { "dtuple_tests_homogeneity_all",
                      "dtuple homogeneity all",
                      &dtuple_tests_homogeneity_all },
                }
            },
            dt::block_spec{
                "2d",
                "2d",
                {
                    { "dtuple_tests_2d_all",
                      "dtuple 2d all",
                      &dtuple_tests_2d_all },
                }
            },
            dt::block_spec{
                "relations",
                "Relations",
                {
                    { "dtuple_tests_relations_all",
                      "dtuple relations all",
                      &dtuple_tests_relations_all },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_DTUPLE_TESTS_
