/*******************************************************************************
* djinterp [test]                                            heap_sort_tests.hpp
*
*   unit-test suite for heap_sort.hpp.
* Declares every tests_heap_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     heap_sort_tests_traits.cpp  heap_sort_traits -- the compile-time
*                                 property block
*     heap_sort_tests_sift.cpp    heap_parent / heap_left_child /
*                                 heap_larger_child / heap_sift_down -- the
*                                 index arithmetic, the tie rule, every exit
*                                 from the sift loop, and agreement with a
*                                 swapping sift
*     heap_sort_tests_apply.cpp   internal::heap_sort_apply -- heapify then
*                                 extract
*     heap_sort_tests_entry.cpp   the public heap_sort / heap_sort_ordered
*                                 entry points
*
*
* path:      /tests/djinterp/cpp/util/sort/heap_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_HEAP_SORT_TESTS_
#define DJINTERP_TESTS_HEAP_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/util/sort/heap_sort.hpp"
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/cpp/modules/test/test_defaults.hpp"  // spec, run_module
#endif  // DTEST_SPEC_MODE
                                                       // the traits cross-check
// djinterp [test]
#include "sort_support.hpp"


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

// heap_sift_ints
//   helper: heap_sift_down over vector<int>, ascending, on the sub-range
// [_first, _first + _size) rooted at _root.
inline
void heap_sift_ints(
    std::vector<int>::iterator _first,
    std::ptrdiff_t             _size,
    std::ptrdiff_t             _root
)
{
    ::djinterp::internal::heap_sift_down(_first,
                                         _size,
                                         _root,
                                         std::less<int>());

    return;
}

// heap_apply_ints
//   helper: the internal driver over vector<int> iterators, ascending.
inline
void heap_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::heap_sort_apply(_first,
                                          _last,
                                          std::less<int>());

    return;
}

// heap_sort_ints
//   helper: the two-argument public entry point over vector<int>.
inline
void heap_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    heap_sort(_first, _last);

    return;
}

// heap_sort_ints_descending
//   helper: the three-argument public entry point under std::greater<int>.
inline
void heap_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    heap_sort(_first, _last, std::greater<int>());

    return;
}

// heap_sort_ints_raw
//   helper: the public entry point over raw pointers.
inline
void heap_sort_ints_raw(
    int* _first,
    int* _last
)
{
    heap_sort(_first, _last);

    return;
}

// heap_sort_keyed
//   helper: the public entry point under the key-only comparator.  Heap sort
// is NOT stable; this adapter drives the ordering and permutation checks.
inline
void heap_sort_keyed(
    std::vector<DKeyedValue>::iterator _first,
    std::vector<DKeyedValue>::iterator _last
)
{
    heap_sort(_first, _last, keyed_value_less());

    return;
}

// heap_sort_keyed_ordered_ascending
//   helper: heap_sort_ordered with sort_order::ascending.  Heap sort's
// ordered entry point takes a real sort_order::value, so it needs no probe.
inline
void heap_sort_keyed_ordered_ascending(
    std::vector<DKeyedValue>::iterator _first,
    std::vector<DKeyedValue>::iterator _last
)
{
    heap_sort_ordered(_first,
                      _last,
                      keyed_value_less(),
                      sort_order::ascending);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// traits -- heap_sort_traits
bool tests_heap_sort_traits_values();

// sift -- heap_parent / heap_left_child / heap_larger_child / heap_sift_down
bool tests_heap_sort_index_arithmetic();
bool tests_heap_sort_sift_down_leaf_exit();
bool tests_heap_sort_sift_down_missing_right_child();
bool tests_heap_sort_sift_down_already_satisfied();
bool tests_heap_sort_sift_down_cascade();
bool tests_heap_sort_larger_child_ties();
bool tests_heap_sort_sift_down_matches_swapping_sift();

// apply -- internal::heap_sort_apply
bool tests_heap_sort_apply_degenerate_ranges();
bool tests_heap_sort_apply_ordering_battery();
bool tests_heap_sort_apply_heap_invariant();

// entry -- heap_sort / heap_sort_ordered
bool tests_heap_sort_entry_comparator();
bool tests_heap_sort_entry_default();
bool tests_heap_sort_entry_ordered();
bool tests_heap_sort_entry_instability();
bool tests_heap_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// heap_sort_traits_block
//   provider: the "traits" block of heap_sort_spec.
inline
dt::block_spec heap_sort_traits_block()
{
    return dt::block_spec
    {
        "traits",
        "heap_sort_traits",
        {
            {
                "heap_sort_traits_values",
                "values",
                &tests_heap_sort_traits_values
            }
        }
    };
}

// heap_sort_sift_block
//   provider: the "sift" block of heap_sort_spec.
inline
dt::block_spec heap_sort_sift_block()
{
    return dt::block_spec
    {
        "sift",
        "heap_parent / heap_left_child / heap_larger_child / "
        "heap_sift_down",
        {
            {
                "heap_sort_index_arithmetic",
                "index arithmetic",
                &tests_heap_sort_index_arithmetic
            },
            {
                "heap_sort_sift_down_leaf_exit",
                "down leaf exit",
                &tests_heap_sort_sift_down_leaf_exit
            },
            {
                "heap_sort_sift_down_missing_right_child",
                "down missing right child",
                &tests_heap_sort_sift_down_missing_right_child
            },
            {
                "heap_sort_sift_down_already_satisfied",
                "down already satisfied",
                &tests_heap_sort_sift_down_already_satisfied
            },
            {
                "heap_sort_sift_down_cascade",
                "down cascade",
                &tests_heap_sort_sift_down_cascade
            },
            {
                "heap_sort_larger_child_ties",
                "larger child: ties",
                &tests_heap_sort_larger_child_ties
            },
            {
                "heap_sort_sift_down_matches_swapping_sift",
                "matches a swapping sift",
                &tests_heap_sort_sift_down_matches_swapping_sift
            }
        }
    };
}

// heap_sort_apply_block
//   provider: the "apply" block of heap_sort_spec.
inline
dt::block_spec heap_sort_apply_block()
{
    return dt::block_spec
    {
        "apply",
        "internal::heap_sort_apply",
        {
            {
                "heap_sort_apply_degenerate_ranges",
                "degenerate ranges",
                &tests_heap_sort_apply_degenerate_ranges
            },
            {
                "heap_sort_apply_ordering_battery",
                "ordering battery",
                &tests_heap_sort_apply_ordering_battery
            },
            {
                "heap_sort_apply_heap_invariant",
                "heap invariant",
                &tests_heap_sort_apply_heap_invariant
            }
        }
    };
}

// heap_sort_entry_block
//   provider: the "entry" block of heap_sort_spec.
inline
dt::block_spec heap_sort_entry_block()
{
    return dt::block_spec
    {
        "entry",
        "heap_sort / heap_sort_ordered",
        {
            {
                "heap_sort_entry_comparator",
                "comparator",
                &tests_heap_sort_entry_comparator
            },
            {
                "heap_sort_entry_default",
                "default",
                &tests_heap_sort_entry_default
            },
            {
                "heap_sort_entry_ordered",
                "ordered",
                &tests_heap_sort_entry_ordered
            },
            {
                "heap_sort_entry_instability",
                "instability",
                &tests_heap_sort_entry_instability
            },
            {
                "heap_sort_entry_pointer_iterators",
                "pointer iterators",
                &tests_heap_sort_entry_pointer_iterators
            }
        }
    };
}

// heap_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// heap_sort_tests_runner.cpp.
inline
dt::module_spec heap_sort_spec()
{
    return dt::module_spec
    {
        "heap_sort.hpp",
        "heap sort: the implicit-heap index arithmetic, the hole-moving "
        "sift-down and its tie rule, the two-phase driver, the published "
        "traits, and the public entry points.",
        {
            heap_sort_traits_block(),
            heap_sort_sift_block(),
            heap_sort_apply_block(),
            heap_sort_entry_block()
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_HEAP_SORT_TESTS_
