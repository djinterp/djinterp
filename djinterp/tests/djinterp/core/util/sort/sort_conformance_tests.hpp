/*******************************************************************************
* djinterp [test]                                     sort_conformance_tests.hpp
*
*   unit-test suite for the sort subsystem's CROSS-CUTTING contracts -- the
* claims that belong to no single header because they are claims about how the
* headers fit together, or about a property the banners advertise for the
* subsystem as a whole.  Declares every tests_sort_conformance_* predicate.
*
*   sections (one .cpp each):
*     test_sort_conformance_adl.cpp        the free sort() entry points must
*                                          be callable unqualified
*     test_sort_conformance_surface.cpp    the six *_sort_ordered signatures
*                                          agree; the dialect the banners
*                                          promise; reachability of the
*                                          shrink guard
*
*   Everything here was written against a coverage run of the suite.  Where a
* check is expected to FAIL it says so, in the same style the *_entry sections
* use: the assertion states the CORRECT behaviour and stays red until the
* library is corrected, rather than being weakened to match the defect.
*
*
* path:      /tests/djinterp/core/util/sort/sort_conformance_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_CONFORMANCE_TESTS_
#define DJINTERP_TESTS_SORT_CONFORMANCE_TESTS_ 1

// std
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/re_std/algorithm/sort.hpp"
#include "sort_support.hpp"
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
///                    I.   UNQUALIFIED-CALL PROBE                          ///
///////////////////////////////////////////////////////////////////////////////

//   djinterp::sort has the same name and the same (Iterator, Iterator[,
// Comparator]) shape as std::sort.  Whenever the iterator type is a standard
// one, ADL adds std::sort to the overload set of an UNQUALIFIED sort(...)
// call, and the two candidates match equally well.  The result is an
// ambiguity at the call site, in the caller's code, from a header the caller
// merely included.
//
//   Ambiguous overload resolution inside decltype is ill-formed in the
// immediate context, so it is SFINAE-detectable and the defect can be probed
// instead of crashing the build.
//
//   The remedy is NOT a SFINAE constraint on the entry point: when the
// argument is a real comparator both candidates stay viable, and constraining
// can only drop djinterp::sort so std::sort silently wins.  The library fix is
// to rename the free entry points or to require the qualified djinterp::sort
// spelling; the probe below tracks the defect until one of those lands.

// unqualified_sort3_probe
//   trait: true when sort(first, last, comparator) resolves unambiguously.
template<typename _Void,
         typename _Iterator,
         typename _Compare>
struct unqualified_sort3_probe_impl
    : std::false_type
{
};

template<typename _Iterator,
         typename _Compare>
struct unqualified_sort3_probe_impl<
    decltype(void(sort(std::declval<_Iterator>(),
                       std::declval<_Iterator>(),
                       std::declval<_Compare>()))),
    _Iterator,
    _Compare>
    : std::true_type
{
};

template<typename _Iterator,
         typename _Compare>
struct unqualified_sort3_probe
    : unqualified_sort3_probe_impl<void, _Iterator, _Compare>
{
};

// unqualified_sort2_probe
//   trait: true when sort(first, last) resolves unambiguously.
template<typename _Void,
         typename _Iterator>
struct unqualified_sort2_probe_impl
    : std::false_type
{
};

template<typename _Iterator>
struct unqualified_sort2_probe_impl<
    decltype(void(sort(std::declval<_Iterator>(),
                       std::declval<_Iterator>()))),
    _Iterator>
    : std::true_type
{
};

template<typename _Iterator>
struct unqualified_sort2_probe
    : unqualified_sort2_probe_impl<void, _Iterator>
{
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  ORDERED-ENTRY UNIFORMITY PROBES                 ///
///////////////////////////////////////////////////////////////////////////////
//   The three algorithms whose *_sort_ordered fourth parameter is `bool`
// already have per-module probes.  These re-declare the same probe for all
// six so one test can assert the SURFACE is uniform, which is the property a
// caller actually depends on when swapping one algorithm for another.

D_TESTS_SORT_ORDERED_PROBE(conf_bubble,    ::djinterp::bubble_sort_ordered)
D_TESTS_SORT_ORDERED_PROBE(conf_selection, ::djinterp::selection_sort_ordered)
D_TESTS_SORT_ORDERED_PROBE(conf_insertion, ::djinterp::insertion_sort_ordered)
D_TESTS_SORT_ORDERED_PROBE(conf_merge,     ::djinterp::merge_sort_ordered)
D_TESTS_SORT_ORDERED_PROBE(conf_quick,     ::djinterp::quick_sort_ordered)
D_TESTS_SORT_ORDERED_PROBE(conf_heap,      ::djinterp::heap_sort_ordered)


///////////////////////////////////////////////////////////////////////////////
///                    III. UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// adl -- the free entry points must be usable unqualified
bool tests_sort_conformance_unqualified_sort_unambiguous();
bool tests_sort_conformance_qualified_sort_dispatches();

// surface -- subsystem-wide claims
bool tests_sort_conformance_ordered_surface_is_uniform();
bool tests_sort_conformance_dialect_gating_is_consistent();
bool tests_sort_conformance_bubble_shrink_boundary_invariant();



#ifdef DTEST_SPEC_MODE

// sort_conformance_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_conformance_tests_runner.cpp.
inline dt::module_spec
sort_conformance_spec()
{
    return dt::module_spec
    {
        "sort (conformance)",
        "cross-cutting contracts spanning the whole subsystem: unqualified "
        "call resolution against std::sort, the uniformity of the ordered "
        "entry points, the C++98-through-C++23 dialect claim, and the "
        "reachability of the bubble-sort shrink guard.",
        {
            dt::block_spec
            {
                "adl",
                "unqualified free sort() entry points",
                {
                    {
                        "sort_conformance_unqualified_sort_unambiguous",
                        "unqualified sort is unambiguous",
                        &tests_sort_conformance_unqualified_sort_unambiguous
                    },
                    {
                        "sort_conformance_qualified_sort_dispatches",
                        "qualified sort dispatches",
                        &tests_sort_conformance_qualified_sort_dispatches
                    }
                }
            },
            dt::block_spec
            {
                "surface",
                "subsystem-wide surface claims",
                {
                    {
                        "sort_conformance_ordered_surface_is_uniform",
                        "ordered surface is uniform",
                        &tests_sort_conformance_ordered_surface_is_uniform
                    },
                    {
                        "sort_conformance_dialect_gating_is_consistent",
                        "dialect gating is consistent",
                        &tests_sort_conformance_dialect_gating_is_consistent
                    },
                    {
                        "sort_conformance_bubble_shrink_boundary_invariant",
                        "bubble shrink boundary invariant",
                        &tests_sort_conformance_bubble_shrink_boundary_invariant
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_CONFORMANCE_TESTS_
