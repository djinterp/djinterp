/*******************************************************************************
* djinterp [test]                                     sort_conformance_tests.hpp
*
*   unit-test suite for the sort subsystem's CROSS-CUTTING contracts -- the
* claims that belong to no single header because they are claims about how the
* headers fit together, or about a property the banners advertise for the
* subsystem as a whole.  Declares every tests_sort_conformance_* predicate.
*
*   sections (one .cpp each):
*     sort_conformance_tests_adl.cpp      djinterp::sort must be callable
*                                         unqualified, next to std::sort
*     sort_conformance_tests_surface.cpp  the six *_sort_ordered entry points
*                                         share one shape and one meaning
*
*   Both claims were once red: djinterp::sort collided with std::sort under
* argument-dependent lookup, and three of the six *_sort_ordered entry points
* took a bool.  The assertions state the correct behaviour, as they did while
* it was missing; they are green now because the library changed, not
* because they did.
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_conformance_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.21
*                                                            revised: 2026.11.12
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
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/util/sort/sort.hpp"
#include "sort_support.hpp"
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"  // spec, run_module
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

//   djinterp::sort has the name and the (Iterator, Iterator[, Comparator])
// shape of std::sort.  Were it a function template, an UNQUALIFIED
// sort(...) on standard iterators would find it by ordinary lookup and
// std::sort by argument-dependent lookup, the two would match equally well,
// and the call would be ambiguous in the caller's code.  sort_dispatch.hpp
// makes djinterp::sort a function object instead: lookup that finds an
// object performs no argument-dependent lookup, so the call resolves.
//
//   Ambiguous overload resolution inside decltype is ill-formed in the
// immediate context, so it is SFINAE-detectable: the probes below report an
// ambiguity as false rather than breaking the build, which is how the
// collision was tracked while it existed.

// DUnqualifiedSort3Probe
//   trait: true when sort(first, last, comparator) resolves unambiguously.
template<typename Void,
         typename Iterator,
         typename Compare>
struct unqualified_sort3_probe_impl
    : std::false_type
{
};

template<typename Iterator,
         typename Compare>
struct unqualified_sort3_probe_impl<
    decltype(void(sort(std::declval<Iterator>(),
                       std::declval<Iterator>(),
                       std::declval<Compare>()))),
    Iterator,
    Compare>
    : std::true_type
{
};

template<typename Iterator,
         typename Compare>
struct DUnqualifiedSort3Probe
    : unqualified_sort3_probe_impl<void, Iterator, Compare>
{
};

// DUnqualifiedSort2Probe
//   trait: true when sort(first, last) resolves unambiguously.
template<typename Void,
         typename Iterator>
struct unqualified_sort2_probe_impl
    : std::false_type
{
};

template<typename Iterator>
struct unqualified_sort2_probe_impl<
    decltype(void(sort(std::declval<Iterator>(),
                       std::declval<Iterator>()))),
    Iterator>
    : std::true_type
{
};

template<typename Iterator>
struct DUnqualifiedSort2Probe
    : unqualified_sort2_probe_impl<void, Iterator>
{
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  ORDERED-ENTRY UNIFORMITY PROBES                 ///
///////////////////////////////////////////////////////////////////////////////
//   The per-module suites probe their own *_sort_ordered.  These declare the
// same probe for all six so one test can assert the SURFACE is uniform, which
// is the property a caller depends on when swapping one algorithm for
// another.

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


// adl -- djinterp::sort must be usable unqualified
bool tests_sort_conformance_unqualified_sort_unambiguous();
bool tests_sort_conformance_qualified_sort_dispatches();

// surface -- subsystem-wide claims
bool tests_sort_conformance_ordered_surface_is_uniform();
bool tests_sort_conformance_ordered_means_the_same();



#ifdef DTEST_SPEC_MODE

// sort_conformance_adl_block
//   provider: the "adl" block of sort_conformance_spec.
inline
dt::block_spec sort_conformance_adl_block()
{
    return dt::block_spec
    {
        "adl",
        "djinterp::sort, unqualified",
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
    };
}

// sort_conformance_surface_block
//   provider: the "surface" block of sort_conformance_spec.
inline
dt::block_spec sort_conformance_surface_block()
{
    return dt::block_spec
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
                "sort_conformance_ordered_means_the_same",
                "ordered means the same everywhere",
                &tests_sort_conformance_ordered_means_the_same
            }
        }
    };
}

// sort_conformance_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_conformance_tests_runner.cpp.
inline
dt::module_spec sort_conformance_spec()
{
    return dt::module_spec
    {
        "sort (conformance)",
        "cross-cutting contracts spanning the whole subsystem: unqualified "
        "call resolution against std::sort, and the shape and meaning the "
        "six ordered entry points share.",
        {
            sort_conformance_adl_block(),
            sort_conformance_surface_block()
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_CONFORMANCE_TESTS_
