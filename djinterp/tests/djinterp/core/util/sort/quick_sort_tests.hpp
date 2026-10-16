/*******************************************************************************
* djinterp [test]                                           quick_sort_tests.hpp
*
*   unit-test suite for quick_sort.hpp.
* Declares every tests_quick_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_quick_sort_median.cpp      internal::quick_sort_median_of_three --
*                                     all six orderings of a distinct triple
*     test_quick_sort_partition.cpp   internal::quick_sort_partition -- the
*                                     Hoare convergence and its post-condition
*     test_quick_sort_apply.cpp       internal::quick_sort_apply -- the
*                                     threshold, the insertion-sort tail, and
*                                     the tail-call loop
*     test_quick_sort_cpp98.cpp       internal::quick_sort_apply_98
*     test_quick_sort_entry.cpp       the public quick_sort /
*                                     quick_sort_ordered entry points
*
*   D_QUICK_SORT_INSERTION_THRESHOLD is 16, so any input of sixteen or fewer
* elements never reaches the partitioner at all.  Every test that means to
* exercise median-of-three, partitioning or recursion therefore uses more than
* sixteen elements.
*
*
* path:      /tests/djinterp/core/util/sort/quick_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_QUICK_SORT_TESTS_
#define DJINTERP_TESTS_QUICK_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
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

// make_pair_vector
//   helper: a two-element vector, for the permutation checks that keep the
// median-of-three tests free of aggregate-initialisation noise.
inline
std::vector<int> make_pair_vector(
    int _a,
    int _b
)
{
    std::vector<int> result;

    result.push_back(_a);
    result.push_back(_b);

    return result;
}

// make_triple
//   helper: a three-element vector, used the same way.
inline
std::vector<int> make_triple(
    int _a,
    int _b,
    int _c
)
{
    std::vector<int> result;

    result.push_back(_a);
    result.push_back(_b);
    result.push_back(_c);

    return result;
}

// quick_median_ints
//   helper: median-of-three over vector<int>, ascending.
inline
void quick_median_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _mid,
    std::vector<int>::iterator _end
)
{
    ::djinterp::internal::quick_sort_median_of_three(_first,
                                                     _mid,
                                                     _end,
                                                     less<int>());

    return;
}

// quick_partition_ints
//   helper: the Hoare partitioner over vector<int>, ascending.  The pivot is
// expected to be sitting at *_first already.
inline
std::vector<int>::iterator quick_partition_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    return ::djinterp::internal::quick_sort_partition(_first,
                                                      _last,
                                                      less<int>());
}

// quick_apply_ints
//   helper: the internal driver over vector<int> iterators, ascending.
inline
void quick_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::quick_sort_apply(_first,
                                           _last,
                                           less<int>());

    return;
}

// quick_apply_98_ints
//   helper: the C++98 hint-parameter driver over vector<int> iterators.
inline
void quick_apply_98_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::quick_sort_apply_98(_first,
                                              _last,
                                              less<int>(),
                                              static_cast<int*>(nullptr));

    return;
}

// quick_apply_98_keyed
//   helper: the C++98 driver under the key-only comparator.
inline
void quick_apply_98_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    ::djinterp::internal::quick_sort_apply_98(
        _first,
        _last,
        keyed_value_less(),
        static_cast<keyed_value*>(nullptr));

    return;
}

// quick_sort_ints
//   helper: the two-argument public entry point over vector<int>.
inline
void quick_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    quick_sort(_first, _last);

    return;
}

// quick_sort_ints_descending
//   helper: the three-argument public entry point under greater<int>.
inline
void quick_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    quick_sort(_first, _last, greater<int>());

    return;
}

// quick_sort_ints_raw
//   helper: the public entry point over raw pointers.
inline
void quick_sort_ints_raw(
    int* _first,
    int* _last
)
{
    quick_sort(_first, _last);

    return;
}

// quick_sort_keyed
//   helper: the public entry point under the key-only comparator.  Quicksort
// is NOT stable; this adapter drives the ordering and permutation checks, not
// a stability claim.
inline
void quick_sort_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    quick_sort(_first, _last, keyed_value_less());

    return;
}

// quick_sort_keyed_ordered_ascending
//   helper: quick_sort_ordered with sort_order::ascending.  Quicksort's
// ordered entry point takes a real sort_order, so it needs no probe.
inline
void quick_sort_keyed_ordered_ascending(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    quick_sort_ordered(_first,
                       _last,
                       keyed_value_less(),
                       sort_order::ascending);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// median -- internal::quick_sort_median_of_three
bool tests_quick_sort_median_all_permutations();
bool tests_quick_sort_median_duplicates();
bool tests_quick_sort_median_degenerate_positions();

// partition -- internal::quick_sort_partition
bool tests_quick_sort_partition_postcondition();
bool tests_quick_sort_partition_all_equal();
bool tests_quick_sort_partition_extreme_pivots();
bool tests_quick_sort_partition_minimal_ranges();

// apply -- internal::quick_sort_apply
bool tests_quick_sort_apply_threshold_boundary();
bool tests_quick_sort_apply_ordering_battery();
bool tests_quick_sort_apply_adversarial_shapes();

// cpp98 -- internal::quick_sort_apply_98
bool tests_quick_sort_cpp98_ordering_battery();
bool tests_quick_sort_cpp98_parity_with_apply();

// entry -- quick_sort / quick_sort_ordered
bool tests_quick_sort_entry_comparator();
bool tests_quick_sort_entry_default();
bool tests_quick_sort_entry_ordered();
bool tests_quick_sort_entry_short_range_guard();
bool tests_quick_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// quick_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// quick_sort_tests_runner.cpp.
inline dt::module_spec
quick_sort_spec()
{
    return dt::module_spec
    {
        "quick_sort.hpp",
        "quicksort: median-of-three pivot selection, the Hoare partitioner, "
        "the hybrid driver with its insertion-sort threshold and "
        "smaller-side recursion, the C++98 variant, and the public entry "
        "points.",
        {
            dt::block_spec
            {
                "median",
                "internal::quick_sort_median_of_three",
                {
                    {
                        "quick_sort_median_all_permutations",
                        "all permutations",
                        &tests_quick_sort_median_all_permutations
                    },
                    {
                        "quick_sort_median_duplicates",
                        "duplicates",
                        &tests_quick_sort_median_duplicates
                    },
                    {
                        "quick_sort_median_degenerate_positions",
                        "degenerate positions",
                        &tests_quick_sort_median_degenerate_positions
                    }
                }
            },
            dt::block_spec
            {
                "partition",
                "internal::quick_sort_partition",
                {
                    {
                        "quick_sort_partition_postcondition",
                        "postcondition",
                        &tests_quick_sort_partition_postcondition
                    },
                    {
                        "quick_sort_partition_all_equal",
                        "all equal",
                        &tests_quick_sort_partition_all_equal
                    },
                    {
                        "quick_sort_partition_extreme_pivots",
                        "extreme pivots",
                        &tests_quick_sort_partition_extreme_pivots
                    },
                    {
                        "quick_sort_partition_minimal_ranges",
                        "minimal ranges",
                        &tests_quick_sort_partition_minimal_ranges
                    }
                }
            },
            dt::block_spec
            {
                "apply",
                "internal::quick_sort_apply",
                {
                    {
                        "quick_sort_apply_threshold_boundary",
                        "threshold boundary",
                        &tests_quick_sort_apply_threshold_boundary
                    },
                    {
                        "quick_sort_apply_ordering_battery",
                        "ordering battery",
                        &tests_quick_sort_apply_ordering_battery
                    },
                    {
                        "quick_sort_apply_adversarial_shapes",
                        "adversarial shapes",
                        &tests_quick_sort_apply_adversarial_shapes
                    }
                }
            },
            dt::block_spec
            {
                "cpp98",
                "internal::quick_sort_apply_98",
                {
                    {
                        "quick_sort_cpp98_ordering_battery",
                        "ordering battery",
                        &tests_quick_sort_cpp98_ordering_battery
                    },
                    {
                        "quick_sort_cpp98_parity_with_apply",
                        "parity with apply",
                        &tests_quick_sort_cpp98_parity_with_apply
                    }
                }
            },
            dt::block_spec
            {
                "entry",
                "quick_sort / quick_sort_ordered",
                {
                    {
                        "quick_sort_entry_comparator",
                        "comparator",
                        &tests_quick_sort_entry_comparator
                    },
                    {
                        "quick_sort_entry_default",
                        "default",
                        &tests_quick_sort_entry_default
                    },
                    {
                        "quick_sort_entry_ordered",
                        "ordered",
                        &tests_quick_sort_entry_ordered
                    },
                    {
                        "quick_sort_entry_short_range_guard",
                        "short range guard",
                        &tests_quick_sort_entry_short_range_guard
                    },
                    {
                        "quick_sort_entry_pointer_iterators",
                        "pointer iterators",
                        &tests_quick_sort_entry_pointer_iterators
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_QUICK_SORT_TESTS_
