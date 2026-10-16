/*******************************************************************************
* djinterp [test]                                          bubble_sort_tests.hpp
*
*   unit-test suite for bubble_sort.hpp.
* Declares every tests_bubble_sort_* predicate and the small adapters the
* suite's translation units share.
*
*   sections (one .cpp each):
*     test_bubble_sort_apply.cpp   internal::bubble_sort_apply -- the driver,
*                                  its early exit, and its shrinking boundary
*     test_bubble_sort_entry.cpp   the public bubble_sort / bubble_sort_ordered
*                                  entry points
*
*
* path:      /tests/djinterp/core/util/sort/bubble_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_BUBBLE_SORT_TESTS_
#define DJINTERP_TESTS_BUBBLE_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/sort/bubble_sort.hpp"
// djinterp [test]
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
///                    I.   ADAPTERS                                        ///
///////////////////////////////////////////////////////////////////////////////
//   The battery drivers call their sorter as sorter(begin, end), so each
// entry point under test is presented through a two-argument adapter.  These
// are ordinary free functions rather than lambdas so a plain function pointer
// can be handed to the driver templates.

// bubble_apply_ints
//   helper: the internal driver over vector<int> iterators, ascending.
inline
void bubble_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::bubble_sort_apply(_first,
                                            _last,
                                            less<int>());

    return;
}

// bubble_sort_ints
//   helper: the two-argument public entry point over vector<int>.
inline
void bubble_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    bubble_sort(_first, _last);

    return;
}

// bubble_sort_ints_descending
//   helper: the three-argument public entry point under greater<int>.
inline
void bubble_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    bubble_sort(_first, _last, greater<int>());

    return;
}

// bubble_sort_ints_raw
//   helper: the public entry point over raw pointers.
inline
void bubble_sort_ints_raw(
    int* _first,
    int* _last
)
{
    bubble_sort(_first, _last);

    return;
}

// bubble_sort_keyed
//   helper: the public entry point under the key-only comparator, so that
// stability is observable.
inline
void bubble_sort_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    bubble_sort(_first, _last, keyed_value_less());

    return;
}

//   bubble_sort_ordered declares its fourth parameter as `bool` but passes it
// to order_comparator, which takes a sort_order: the template cannot be
// instantiated.  The probe detects that without naming the call directly.
D_TESTS_SORT_ORDERED_PROBE(bubble, ::djinterp::bubble_sort_ordered)


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// apply -- internal::bubble_sort_apply
bool tests_bubble_sort_apply_degenerate_ranges();
bool tests_bubble_sort_apply_ordering_battery();
bool tests_bubble_sort_apply_early_exit();
bool tests_bubble_sort_apply_shrinking_boundary();
bool tests_bubble_sort_apply_swap_traffic();

// entry -- bubble_sort / bubble_sort_ordered
bool tests_bubble_sort_entry_comparator();
bool tests_bubble_sort_entry_default();
bool tests_bubble_sort_entry_ordered();
bool tests_bubble_sort_entry_stability();
bool tests_bubble_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// bubble_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// bubble_sort_tests_runner.cpp.
inline dt::module_spec
bubble_sort_spec()
{
    return dt::module_spec
    {
        "bubble_sort.hpp",
        "bubble sort: the internal driver with its early-exit and shrinking "
        "boundary optimisations, and the public entry points.",
        {
            dt::block_spec
            {
                "apply",
                "internal::bubble_sort_apply",
                {
                    {
                        "bubble_sort_apply_degenerate_ranges",
                        "degenerate ranges",
                        &tests_bubble_sort_apply_degenerate_ranges
                    },
                    {
                        "bubble_sort_apply_ordering_battery",
                        "ordering battery",
                        &tests_bubble_sort_apply_ordering_battery
                    },
                    {
                        "bubble_sort_apply_early_exit",
                        "early exit",
                        &tests_bubble_sort_apply_early_exit
                    },
                    {
                        "bubble_sort_apply_shrinking_boundary",
                        "shrinking boundary",
                        &tests_bubble_sort_apply_shrinking_boundary
                    },
                    {
                        "bubble_sort_apply_swap_traffic",
                        "swap traffic",
                        &tests_bubble_sort_apply_swap_traffic
                    }
                }
            },
            dt::block_spec
            {
                "entry",
                "bubble_sort / bubble_sort_ordered",
                {
                    {
                        "bubble_sort_entry_comparator",
                        "comparator",
                        &tests_bubble_sort_entry_comparator
                    },
                    {
                        "bubble_sort_entry_default",
                        "default",
                        &tests_bubble_sort_entry_default
                    },
                    {
                        "bubble_sort_entry_ordered",
                        "ordered",
                        &tests_bubble_sort_entry_ordered
                    },
                    {
                        "bubble_sort_entry_stability",
                        "stability",
                        &tests_bubble_sort_entry_stability
                    },
                    {
                        "bubble_sort_entry_pointer_iterators",
                        "pointer iterators",
                        &tests_bubble_sort_entry_pointer_iterators
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_BUBBLE_SORT_TESTS_
