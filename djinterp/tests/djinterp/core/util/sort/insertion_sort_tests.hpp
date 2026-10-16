/*******************************************************************************
* djinterp [test]                                       insertion_sort_tests.hpp
*
*   unit-test suite for insertion_sort.hpp.
* Declares every tests_insertion_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_insertion_sort_apply.cpp   internal::insertion_sort_apply -- the
*                                     shift loop, the already-in-place fast
*                                     path, and the exact comparison counts
*     test_insertion_sort_cpp98.cpp   internal::insertion_sort_apply_98 -- the
*                                     hint-parameter variant, held to parity
*                                     with the traits-deducing one
*     test_insertion_sort_entry.cpp   the public insertion_sort /
*                                     insertion_sort_ordered entry points
*
*
* path:      /tests/djinterp/core/util/sort/insertion_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_INSERTION_SORT_TESTS_
#define DJINTERP_TESTS_INSERTION_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/sort/insertion_sort.hpp"
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

// insertion_apply_ints
//   helper: the internal driver over vector<int> iterators, ascending.
inline
void insertion_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::insertion_sort_apply(_first,
                                               _last,
                                               less<int>());

    return;
}

// insertion_apply_98_ints
//   helper: the C++98 hint-parameter driver over vector<int> iterators.  The
// trailing null pointer names the element type in place of
// std::iterator_traits.
inline
void insertion_apply_98_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::insertion_sort_apply_98(_first,
                                                  _last,
                                                  less<int>(),
                                                  static_cast<int*>(nullptr));

    return;
}

// insertion_apply_98_keyed
//   helper: the C++98 driver under the key-only comparator.
inline
void insertion_apply_98_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    ::djinterp::internal::insertion_sort_apply_98(
        _first,
        _last,
        keyed_value_less(),
        static_cast<keyed_value*>(nullptr));

    return;
}

// insertion_sort_ints
//   helper: the two-argument public entry point over vector<int>.
inline
void insertion_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    insertion_sort(_first, _last);

    return;
}

// insertion_sort_ints_descending
//   helper: the three-argument public entry point under greater<int>.
inline
void insertion_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    insertion_sort(_first, _last, greater<int>());

    return;
}

// insertion_sort_ints_raw
//   helper: the public entry point over raw pointers.
inline
void insertion_sort_ints_raw(
    int* _first,
    int* _last
)
{
    insertion_sort(_first, _last);

    return;
}

// insertion_sort_keyed
//   helper: the public entry point under the key-only comparator, so that
// stability is observable.
inline
void insertion_sort_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    insertion_sort(_first, _last, keyed_value_less());

    return;
}

//   insertion_sort_ordered has the same bool / sort_order mismatch as
// bubble_sort_ordered; the probe keeps it out of the immediate compile.
D_TESTS_SORT_ORDERED_PROBE(insertion, ::djinterp::insertion_sort_ordered)


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// apply -- internal::insertion_sort_apply
bool tests_insertion_sort_apply_degenerate_ranges();
bool tests_insertion_sort_apply_ordering_battery();
bool tests_insertion_sort_apply_fast_path();
bool tests_insertion_sort_apply_full_shift();
bool tests_insertion_sort_apply_comparison_counts();

// cpp98 -- internal::insertion_sort_apply_98
bool tests_insertion_sort_cpp98_degenerate_ranges();
bool tests_insertion_sort_cpp98_ordering_battery();
bool tests_insertion_sort_cpp98_parity_with_apply();
bool tests_insertion_sort_cpp98_stability();

// entry -- insertion_sort / insertion_sort_ordered
bool tests_insertion_sort_entry_comparator();
bool tests_insertion_sort_entry_default();
bool tests_insertion_sort_entry_ordered();
bool tests_insertion_sort_entry_stability();
bool tests_insertion_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// insertion_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// insertion_sort_tests_runner.cpp.
inline dt::module_spec
insertion_sort_spec()
{
    return dt::module_spec
    {
        "insertion_sort.hpp",
        "insertion sort: the shift-based internal driver, the C++98 "
        "hint-parameter variant, and the public entry points.",
        {
            dt::block_spec
            {
                "apply",
                "internal::insertion_sort_apply",
                {
                    {
                        "insertion_sort_apply_degenerate_ranges",
                        "degenerate ranges",
                        &tests_insertion_sort_apply_degenerate_ranges
                    },
                    {
                        "insertion_sort_apply_ordering_battery",
                        "ordering battery",
                        &tests_insertion_sort_apply_ordering_battery
                    },
                    {
                        "insertion_sort_apply_fast_path",
                        "fast path",
                        &tests_insertion_sort_apply_fast_path
                    },
                    {
                        "insertion_sort_apply_full_shift",
                        "full shift",
                        &tests_insertion_sort_apply_full_shift
                    },
                    {
                        "insertion_sort_apply_comparison_counts",
                        "comparison counts",
                        &tests_insertion_sort_apply_comparison_counts
                    }
                }
            },
            dt::block_spec
            {
                "cpp98",
                "internal::insertion_sort_apply_98",
                {
                    {
                        "insertion_sort_cpp98_degenerate_ranges",
                        "degenerate ranges",
                        &tests_insertion_sort_cpp98_degenerate_ranges
                    },
                    {
                        "insertion_sort_cpp98_ordering_battery",
                        "ordering battery",
                        &tests_insertion_sort_cpp98_ordering_battery
                    },
                    {
                        "insertion_sort_cpp98_parity_with_apply",
                        "parity with apply",
                        &tests_insertion_sort_cpp98_parity_with_apply
                    },
                    {
                        "insertion_sort_cpp98_stability",
                        "stability",
                        &tests_insertion_sort_cpp98_stability
                    }
                }
            },
            dt::block_spec
            {
                "entry",
                "insertion_sort / insertion_sort_ordered",
                {
                    {
                        "insertion_sort_entry_comparator",
                        "comparator",
                        &tests_insertion_sort_entry_comparator
                    },
                    {
                        "insertion_sort_entry_default",
                        "default",
                        &tests_insertion_sort_entry_default
                    },
                    {
                        "insertion_sort_entry_ordered",
                        "ordered",
                        &tests_insertion_sort_entry_ordered
                    },
                    {
                        "insertion_sort_entry_stability",
                        "stability",
                        &tests_insertion_sort_entry_stability
                    },
                    {
                        "insertion_sort_entry_pointer_iterators",
                        "pointer iterators",
                        &tests_insertion_sort_entry_pointer_iterators
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_INSERTION_SORT_TESTS_
