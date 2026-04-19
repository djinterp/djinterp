/******************************************************************************
* djinterp [test]                                            test_defaults.hpp
*
*   Default test framework configuration: the option enumeration, the
* five built-in test kinds, and their default metadata.
*
*   This header is the single source of truth for the DTest framework's
* default vocabulary of test types.  All built-in kind constants, rank
* assignments, leaf/interior flags, and default option content are
* defined here and nowhere else.  The generic infrastructure in
* test_type.hpp and test_object.hpp is intentionally kind-agnostic.
*
*   OPTION ENUMERATION:
*   DTestOption enumerates the configurable parameters available
* in a dtest_option_set.  Currently defines test_metadata; additional
* option keys will be added as the framework grows.
*
*   TEST METADATA:
*   Every built-in kind carries a default test_metadata option whose
* value is a sorted std::vector<std::string>.  The metadata vector
* stores tags, labels, or descriptive strings associated with that
* kind.  Default metadata content is empty — the user populates it
* at registration or runtime.  The sorted invariant is maintained
* by insertion helpers.
*
*   BUILT-IN KINDS (ranked lowest to highest):
*     0  assert       — single boolean assertion         (leaf)
*     1  test_fn      — test function pointer wrapper    (leaf)
*     2  test         — individual test case             (interior)
*     3  test_block   — group of tests                   (interior)
*     4  module       — group of blocks                  (interior)
*
*   OPTION LIFETIME:
*   make_default_test_type() creates kinds with nullptr default
* options.  To wire up per-kind defaults, the caller creates
* dtest_option_set instances via make_kind_options(), owns their
* storage, and passes pointers into the test_kind entries.  This
* keeps lifetime management explicit and avoids hidden statics.
*
*   PORTABILITY:
*   C++11 minimum.  Uses env.h for version detection and djinterp.hpp
* for namespace macros and constexpr support.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST METADATA TYPE
* II.   TEST METADATA HELPERS
* III.  BUILT-IN KIND CONSTANTS
* IV.   DEFAULT TYPE FACTORY
* V.    DEFAULT OPTION FACTORIES
* VI.   CONVENIENCE OBJECT FACTORIES
*
*
* path:      /inc/djinterp/test/test_defaults.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_TEST_DEFAULTS_
#define DJINTERP_TEST_DEFAULTS_ 1

// std
#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../restd/any/any.hpp"
#include "../core/options/option_pair.hpp"
#include "../core/options/option_set.hpp"
#include "./test_common.hpp"
#include "./test_options.hpp"
#include "./test_type.hpp"
#include "./test_object.hpp"


NS_DJINTERP
NS_TEST

using djinterp::restd::any;

///////////////////////////////////////////////////////////////////////////////
///                I.   TEST METADATA TYPE                                   ///
///////////////////////////////////////////////////////////////////////////////

// test_metadata_type
//   type: the concrete value type for the test_metadata
// option key.  A sorted vector of strings — the sorted
// invariant is maintained by the insertion helpers below.
using test_metadata_type = std::vector<std::string>;


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST METADATA HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

// metadata_insert
//   function: inserts a tag into a sorted metadata vector
// at its sorted position.  Duplicates are silently ignored.
D_INLINE void
metadata_insert(
    test_metadata_type& _meta,
    const std::string&  _tag
)
{
    auto pos = std::lower_bound(_meta.begin(),
                                _meta.end(),
                                _tag);

    // skip duplicate
    if ( (pos != _meta.end()) &&
         (*pos == _tag) )
    {
        return;
    }

    _meta.insert(pos, _tag);

    return;
}

// metadata_contains
//   function: returns true if _tag is present in the sorted
// metadata vector.  Uses binary search.
D_INLINE bool
metadata_contains(
    const test_metadata_type& _meta,
    const std::string&        _tag
)
{
    return std::binary_search(_meta.begin(),
                              _meta.end(),
                              _tag);
}

// metadata_remove
//   function: removes _tag from the sorted metadata vector.
// Returns true if the tag was found and removed.
D_INLINE bool
metadata_remove(
    test_metadata_type& _meta,
    const std::string&  _tag
)
{
    auto pos = std::lower_bound(_meta.begin(),
                                _meta.end(),
                                _tag);

    if ( (pos == _meta.end()) ||
         (*pos != _tag) )
    {
        return false;
    }

    _meta.erase(pos);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
///                III. BUILT-IN KIND CONSTANTS                              ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_KIND_ASSERT
//   constant: test_type_id for assertion-level test objects.
// Rank 0, leaf.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_ASSERT     = 0;

// D_TEST_KIND_TEST_FN
//   constant: test_type_id for test function pointer wrappers.
// Rank 1, leaf.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_FN    = 1;

// D_TEST_KIND_TEST
//   constant: test_type_id for individual test cases.
// Rank 2, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST       = 2;

// D_TEST_KIND_TEST_BLOCK
//   constant: test_type_id for test blocks (groups of tests).
// Rank 3, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_TEST_BLOCK = 3;

// D_TEST_KIND_MODULE
//   constant: test_type_id for test modules (top-level grouping).
// Rank 4, interior.
D_STATIC_CONSTEXPR test_type_id D_TEST_KIND_MODULE     = 4;

// D_TEST_KIND_COUNT
//   constant: number of built-in kind constants.
D_STATIC_CONSTEXPR std::size_t  D_TEST_KIND_COUNT      = 5;


///////////////////////////////////////////////////////////////////////////////
///                IV.  DEFAULT TYPE FACTORY                                 ///
///////////////////////////////////////////////////////////////////////////////

// make_default_test_type
//   function: constructs a test_type pre-populated with the
// five built-in kinds using their default ranks and
// leaf/interior flags.  All default_options pointers are
// nullptr — the caller is responsible for creating option
// sets and wiring them in.
template<typename _Container = std::vector<test_kind>>
D_INLINE test_type<_Container>
make_default_test_type()
{
    test_type<_Container> tt;

    tt.register_kind({ D_TEST_KIND_ASSERT,
                       "assert",
                       0,
                       true,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_TEST_FN,
                       "test_fn",
                       1,
                       true,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_TEST,
                       "test",
                       2,
                       false,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_TEST_BLOCK,
                       "test_block",
                       3,
                       false,
                       nullptr });

    tt.register_kind({ D_TEST_KIND_MODULE,
                       "module",
                       4,
                       false,
                       nullptr });

    return tt;
}


///////////////////////////////////////////////////////////////////////////////
///                V.   DEFAULT OPTION FACTORIES                             ///
///////////////////////////////////////////////////////////////////////////////

// make_kind_options
//   function: constructs a dtest_option_set containing a
// single test_metadata entry initialized to an empty sorted
// vector.  The caller may populate the vector via
// metadata_insert() after construction.
D_INLINE dtest_option_set
make_kind_options()
{
    dtest_option_set opts;

    opts.insert(DTestOption::metadata, any(test_metadata_type{}));

    return opts;
}

// make_kind_options (with initial tags)
//   function: constructs a dtest_option_set whose
// test_metadata vector is pre-populated with the given
// tags.  Tags are sorted on construction.
D_INLINE dtest_option_set
make_kind_options(
    test_metadata_type _tags
)
{
    // ensure sorted invariant
    std::sort(_tags.begin(), _tags.end());

    // remove duplicates
    _tags.erase(std::unique(_tags.begin(),
                            _tags.end()),
                _tags.end());

    dtest_option_set opts;

    opts.insert(DTestOption::metadata,
                any(static_cast<test_metadata_type&&>(_tags)));

    return opts;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  CONVENIENCE OBJECT FACTORIES                         ///
///////////////////////////////////////////////////////////////////////////////

// make_assert
//   function: creates an assertion-level leaf test.
D_CONSTEXPR_INLINE basic_test
make_assert(
    bool        _result,
    const char* _name         = nullptr,
    const char* _message_pass = nullptr,
    const char* _message_fail = nullptr
) D_NOEXCEPT
{
    return basic_test(
        D_TEST_KIND_ASSERT,
        _result,
        _name,
        _message_pass,
        _message_fail);
}

// make_test_fn
//   function: creates a test_fn-level leaf test.
D_CONSTEXPR_INLINE basic_test
make_test_fn(
    bool        _result,
    const char* _name = nullptr
) D_NOEXCEPT
{
    return basic_test(D_TEST_KIND_TEST_FN,
                      _result,
                      _name);
}

// make_test_case
//   function: creates a test-level interior node.
D_CONSTEXPR_INLINE basic_test
make_test_case(
    const char* _name
) D_NOEXCEPT
{
    return make_interior(D_TEST_KIND_TEST, _name);
}

// make_test_block
//   function: creates a test_block-level interior node.
D_CONSTEXPR_INLINE basic_test
make_test_block(
    const char* _name
) D_NOEXCEPT
{
    return make_interior(D_TEST_KIND_TEST_BLOCK, _name);
}

// make_module
//   function: creates a module-level interior node.
D_CONSTEXPR_INLINE basic_test
make_module(
    const char* _name
) D_NOEXCEPT
{
    return make_interior(D_TEST_KIND_MODULE, _name);
}

// ---- the template string ----
D_STATIC const char* const D_TEST_TPL_MASTER_SUITE =
    "============================================"
    "====================================\n"
    "  TESTING:     %module_name%\n"
    "============================================"
    "====================================\n"
    "  description: %module_description%\n"
    "  path:        %module_path%\n"
    "  date/time:   %timestamp_start%\n"
    "============================================"
    "====================================\n"

    "\n"

    "--------------------------------------------"
    "------------------------------------\n"
    "  MODULE: %description_short%\n"
    "  %description_long%\n"
    "--------------------------------------------"
    "------------------------------------\n"

    "\n"

    "%test_modules%"

    "\n"

    "--------------------------------------------"
    "------------------------------------\n"
    "  COMPREHENSIVE TEST RESULTS\n"
    "--------------------------------------------"
    "------------------------------------\n"

    "  MODULE SUMMARY:\n"
    "    Modules Tested:       %modules_tested%\n"
    "    Modules Passed:       %modules_passed%\n"
    "    Module Success Rate:  %modules_percent%\n"

    "\n"

    "  ASSERTION SUMMARY:\n"
    "    Total Assertions:     %asserts_total%\n"
    "    Assertions Passed:    %asserts_passed%\n"
    "    Assertions Failed:    %asserts_failed%\n"
    "    Assertion Pass Rate:  %asserts_percent%\n"

    "\n"

    "  UNIT TEST SUMMARY:\n"
    "    Total Unit Tests:     %tests_total%\n"
    "    Unit Tests Passed:    %tests_passed%\n"
    "    Unit Tests Failed:    %tests_failed%\n"
    "    Unit Test Pass Rate:  %tests_percent%\n"

    "\n"

    "  EXECUTION TIME:\n"
    "    Total Time:           %time_total%\n"

    "--------------------------------------------"
    "------------------------------------\n"

    "\n"

    "============================================"
    "====================================\n"
    "  MODULE RESULTS: %module_name%\n"
    "============================================"
    "====================================\n"
    "  Assertions: %asserts_passed% / %asserts_total%"
    " (%asserts_percent%)\n"
    "  Unit Tests: %tests_passed% / %tests_total%"
    " (%tests_percent%)\n"
    "  Status:     %has_passed%\n"
    "============================================"
    "====================================\n";


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_DEFAULTS_
