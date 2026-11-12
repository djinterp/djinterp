/*******************************************************************************
* djinterp [test]                                       insertion_sort_tests.hpp
*
*   unit-test suite for insertion_sort.hpp.
* Declares every tests_insertion_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     insertion_sort_tests_primitives.cpp  insertion_place / insertion_pass
*                                          -- where a candidate lands, where
*                                          it stops, and the pass over a
*                                          sub-run
*     insertion_sort_tests_apply.cpp       internal::insertion_sort_apply --
*                                          the shift loop, the already-in-
*                                          place fast path, and the exact
*                                          comparison counts
*     insertion_sort_tests_entry.cpp       the public insertion_sort /
*                                          insertion_sort_ordered entry points
*
*
* path:      /tests/djinterp/cpp/util/sort/insertion_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_INSERTION_SORT_TESTS_
#define DJINTERP_TESTS_INSERTION_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/util/sort/insertion_sort.hpp"
// djinterp [test]
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
                                               std::less<int>());

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
//   helper: the three-argument public entry point under std::greater<int>.
inline
void insertion_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    insertion_sort(_first, _last, std::greater<int>());

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
    std::vector<DKeyedValue>::iterator _first,
    std::vector<DKeyedValue>::iterator _last
)
{
    insertion_sort(_first, _last, keyed_value_less());

    return;
}

//   the probe asks whether insertion_sort_ordered takes the shared
// (It, It, Cmp, sort_order::value) shape, without naming the call.
D_TESTS_SORT_ORDERED_PROBE(insertion, ::djinterp::insertion_sort_ordered)


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// primitives -- insertion_place / insertion_pass
bool tests_insertion_sort_place_landing_index();
bool tests_insertion_sort_place_stops_at_equivalent();
bool tests_insertion_sort_pass_sub_run();

// apply -- internal::insertion_sort_apply
bool tests_insertion_sort_apply_degenerate_ranges();
bool tests_insertion_sort_apply_ordering_battery();
bool tests_insertion_sort_apply_fast_path();
bool tests_insertion_sort_apply_full_shift();
bool tests_insertion_sort_apply_comparison_counts();

// entry -- insertion_sort / insertion_sort_ordered
bool tests_insertion_sort_entry_comparator();
bool tests_insertion_sort_entry_default();
bool tests_insertion_sort_entry_ordered();
bool tests_insertion_sort_entry_stability();
bool tests_insertion_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// insertion_sort_primitives_block
//   provider: the "primitives" block of insertion_sort_spec.
inline
dt::block_spec insertion_sort_primitives_block()
{
    return dt::block_spec
    {
        "primitives",
        "insertion_place / insertion_pass",
        {
            {
                "insertion_sort_place_landing_index",
                "place: the landing index",
                &tests_insertion_sort_place_landing_index
            },
            {
                "insertion_sort_place_stops_at_equivalent",
                "place: stops at an equivalent",
                &tests_insertion_sort_place_stops_at_equivalent
            },
            {
                "insertion_sort_pass_sub_run",
                "pass over a sub-run",
                &tests_insertion_sort_pass_sub_run
            }
        }
    };
}

// insertion_sort_apply_block
//   provider: the "apply" block of insertion_sort_spec.
inline
dt::block_spec insertion_sort_apply_block()
{
    return dt::block_spec
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
    };
}

// insertion_sort_entry_block
//   provider: the "entry" block of insertion_sort_spec.
inline
dt::block_spec insertion_sort_entry_block()
{
    return dt::block_spec
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
    };
}

// insertion_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// insertion_sort_tests_runner.cpp.
inline
dt::module_spec insertion_sort_spec()
{
    return dt::module_spec
    {
        "insertion_sort.hpp",
        "insertion sort: the placement and pass primitives, the shift-based "
        "internal driver, and the public entry points.",
        {
            insertion_sort_primitives_block(),
            insertion_sort_apply_block(),
            insertion_sort_entry_block()
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_INSERTION_SORT_TESTS_
