/*******************************************************************************
* djinterp [test]                                                 sort_tests.hpp
*
*   unit-test suite for sort.hpp -- the integrated sort surface.
* Declares every tests_sort_facade_* predicate.
*
*   sections (one .cpp each):
*     test_sort_facade_surface.cpp       what including sort.hpp does and does
*                                        NOT bring in
*     test_sort_facade_integration.cpp   the three included headers used
*                                        together, end to end
*
*   sort.hpp deliberately omits sort_view.hpp.  That omission is a documented
* property of the facade, so it is asserted rather than assumed -- which is
* why test_sort_facade_surface.cpp includes THIS header and nothing else: any
* other include would pull sort_view.hpp into the translation unit and make
* the guard check meaningless.  For the same reason this header does not
* include test_sort_support.hpp.
*
*
* path:      /tests/djinterp/core/util/sort/sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_TESTS_
#define DJINTERP_TESTS_SORT_TESTS_ 1

// std
#include <algorithm>
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/re_std/algorithm/sort.hpp"
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // module_spec + run_module
#endif  // DTEST_SPEC_MODE


NS_DJINTERP

//   djinterp::test has to be NAMEABLE in both faces: normal mode does not pull
// in test_defaults.hpp, so the namespace is declared here before it is aliased.
NS_TEST
NS_END  // test

NS_TESTING

//   dt names the entities under test (djinterp::test).  Declared
// UNCONDITIONALLY: the spec provider below needs dt::module_spec too.
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE   // fixtures -- normal mode only


///////////////////////////////////////////////////////////////////////////////
///                    I.   UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// surface -- what sort.hpp includes
bool tests_sort_facade_include_guards();
bool tests_sort_facade_view_is_not_included();
bool tests_sort_facade_core_symbols_visible();

// integration -- the three headers used together
bool tests_sort_facade_every_algorithm_agrees();
bool tests_sort_facade_dispatch_and_monoid_agree();
bool tests_sort_facade_comparator_algebra_end_to_end();



#ifdef DTEST_SPEC_MODE

// sort_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_tests_runner.cpp.
inline dt::module_spec
sort_spec()
{
    return dt::module_spec
    {
        "sort.hpp",
        "the facade: what a bare include of sort.hpp exports, what it "
        "deliberately does not, and the three aggregated headers used "
        "together.",
        {
            dt::block_spec
            {
                "surface",
                "what sort.hpp includes",
                {
                    {
                        "sort_facade_include_guards",
                        "include guards",
                        &tests_sort_facade_include_guards
                    },
                    {
                        "sort_facade_view_is_not_included",
                        "view is not included",
                        &tests_sort_facade_view_is_not_included
                    },
                    {
                        "sort_facade_core_symbols_visible",
                        "core symbols visible",
                        &tests_sort_facade_core_symbols_visible
                    }
                }
            },
            dt::block_spec
            {
                "integration",
                "the three headers used together",
                {
                    {
                        "sort_facade_every_algorithm_agrees",
                        "every algorithm agrees",
                        &tests_sort_facade_every_algorithm_agrees
                    },
                    {
                        "sort_facade_dispatch_and_monoid_agree",
                        "dispatch and monoid agree",
                        &tests_sort_facade_dispatch_and_monoid_agree
                    },
                    {
                        "sort_facade_comparator_algebra_end_to_end",
                        "comparator algebra end to end",
                        &tests_sort_facade_comparator_algebra_end_to_end
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_TESTS_
