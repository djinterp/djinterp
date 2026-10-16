/*******************************************************************************
* djinterp [test]                                       selection_sort_tests.hpp
*
*   unit-test suite for selection_sort.hpp.
* Declares every tests_selection_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_selection_sort_apply.cpp   internal::selection_sort_apply -- the
*                                     minimum scan, the swap bound, and the
*                                     exact comparison count
*     test_selection_sort_entry.cpp   the public selection_sort /
*                                     selection_sort_ordered entry points
*
*
* path:      /tests/djinterp/core/util/sort/selection_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SELECTION_SORT_TESTS_
#define DJINTERP_TESTS_SELECTION_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/sort/selection_sort.hpp"
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

// selection_apply_ints
//   helper: the internal driver over vector<int> iterators, ascending.
inline
void selection_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::selection_sort_apply(_first,
                                               _last,
                                               less<int>());

    return;
}

// selection_sort_ints
//   helper: the two-argument public entry point over vector<int>.
inline
void selection_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    selection_sort(_first, _last);

    return;
}

// selection_sort_ints_descending
//   helper: the three-argument public entry point under greater<int>.
inline
void selection_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    selection_sort(_first, _last, greater<int>());

    return;
}

// selection_sort_ints_raw
//   helper: the public entry point over raw pointers.
inline
void selection_sort_ints_raw(
    int* _first,
    int* _last
)
{
    selection_sort(_first, _last);

    return;
}

// selection_sort_keyed
//   helper: the public entry point under the key-only comparator.  Selection
// sort is NOT stable, so this adapter is used to demonstrate the documented
// instability, not to assert stability.
inline
void selection_sort_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    selection_sort(_first, _last, keyed_value_less());

    return;
}

//   selection_sort_ordered has the same bool / sort_order mismatch as
// bubble_sort_ordered; the probe keeps it out of the immediate compile.
D_TESTS_SORT_ORDERED_PROBE(selection, ::djinterp::selection_sort_ordered)


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// apply -- internal::selection_sort_apply
bool tests_selection_sort_apply_degenerate_ranges();
bool tests_selection_sort_apply_ordering_battery();
bool tests_selection_sort_apply_comparison_count();
bool tests_selection_sort_apply_swap_bound();
bool tests_selection_sort_apply_self_swap_skipped();

// entry -- selection_sort / selection_sort_ordered
bool tests_selection_sort_entry_comparator();
bool tests_selection_sort_entry_default();
bool tests_selection_sort_entry_ordered();
bool tests_selection_sort_entry_instability();
bool tests_selection_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// selection_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// selection_sort_tests_runner.cpp.
inline dt::module_spec
selection_sort_spec()
{
    return dt::module_spec
    {
        "selection_sort.hpp",
        "selection sort: the internal driver's minimum scan and guarded "
        "swap, and the public entry points.",
        {
            dt::block_spec
            {
                "apply",
                "internal::selection_sort_apply",
                {
                    {
                        "selection_sort_apply_degenerate_ranges",
                        "degenerate ranges",
                        &tests_selection_sort_apply_degenerate_ranges
                    },
                    {
                        "selection_sort_apply_ordering_battery",
                        "ordering battery",
                        &tests_selection_sort_apply_ordering_battery
                    },
                    {
                        "selection_sort_apply_comparison_count",
                        "comparison count",
                        &tests_selection_sort_apply_comparison_count
                    },
                    {
                        "selection_sort_apply_swap_bound",
                        "swap bound",
                        &tests_selection_sort_apply_swap_bound
                    },
                    {
                        "selection_sort_apply_self_swap_skipped",
                        "self swap skipped",
                        &tests_selection_sort_apply_self_swap_skipped
                    }
                }
            },
            dt::block_spec
            {
                "entry",
                "selection_sort / selection_sort_ordered",
                {
                    {
                        "selection_sort_entry_comparator",
                        "comparator",
                        &tests_selection_sort_entry_comparator
                    },
                    {
                        "selection_sort_entry_default",
                        "default",
                        &tests_selection_sort_entry_default
                    },
                    {
                        "selection_sort_entry_ordered",
                        "ordered",
                        &tests_selection_sort_entry_ordered
                    },
                    {
                        "selection_sort_entry_instability",
                        "instability",
                        &tests_selection_sort_entry_instability
                    },
                    {
                        "selection_sort_entry_pointer_iterators",
                        "pointer iterators",
                        &tests_selection_sort_entry_pointer_iterators
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SELECTION_SORT_TESTS_
