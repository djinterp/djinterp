/*******************************************************************************
* djinterp [test]                                           merge_sort_tests.hpp
*
*   unit-test suite for merge_sort.hpp.
* Declares every tests_merge_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_merge_sort_traits.cpp    merge_sort_traits -- the compile-time
*                                   property block
*     test_merge_sort_merge.cpp     internal::merge_halves -- the merge
*                                   primitive in isolation, including both
*                                   exhaustion tails
*     test_merge_sort_recurse.cpp   internal::merge_sort_recurse -- the split
*                                   and the shared buffer
*     test_merge_sort_cpp98.cpp     internal::merge_sort_apply_98 -- the
*                                   hint-parameter variant
*     test_merge_sort_entry.cpp     the public merge_sort / merge_sort_ordered
*                                   entry points
*
*   merge_halves and merge_sort_recurse take a trailing, non-deducible
* _DiffType template parameter, so every call to them must name all four
* template arguments explicitly.  The adapters below do that once.
*
*
* path:      /tests/djinterp/core/util/sort/merge_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_MERGE_SORT_TESTS_
#define DJINTERP_TESTS_MERGE_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/core/util/sort/merge_sort.hpp"
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"   // module_spec + run_module
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

// merge_halves_ints
//   helper: merge_halves over vector<int>, ascending, with every template
// argument named because _DiffType is not deducible from the call.
inline
void merge_halves_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _mid,
    std::vector<int>::iterator _last,
    int*                       _buffer
)
{
    ::djinterp::internal::merge_halves<std::vector<int>::iterator,
                                       int,
                                       less<int>,
                                       std::ptrdiff_t>(_first,
                                                       _mid,
                                                       _last,
                                                       _buffer,
                                                       less<int>());

    return;
}

// merge_halves_keyed
//   helper: merge_halves under the key-only comparator, so the merge step's
// left-favouring tie-break -- the source of the algorithm's stability -- is
// directly observable.
inline
void merge_halves_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _mid,
    std::vector<keyed_value>::iterator _last,
    keyed_value*                       _buffer
)
{
    ::djinterp::internal::merge_halves<std::vector<keyed_value>::iterator,
                                       keyed_value,
                                       keyed_value_less,
                                       std::ptrdiff_t>(_first,
                                                       _mid,
                                                       _last,
                                                       _buffer,
                                                       keyed_value_less());

    return;
}

// merge_recurse_ints
//   helper: merge_sort_recurse over vector<int> with a caller-owned buffer.
inline
void merge_recurse_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last,
    int*                       _buffer
)
{
    ::djinterp::internal::merge_sort_recurse<std::vector<int>::iterator,
                                             int,
                                             less<int>,
                                             std::ptrdiff_t>(_first,
                                                             _last,
                                                             _buffer,
                                                             less<int>());

    return;
}

// merge_apply_ints
//   helper: the internal top-level driver, which owns its own buffer.
inline
void merge_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::merge_sort_apply(_first,
                                           _last,
                                           less<int>());

    return;
}

// merge_apply_98_ints
//   helper: the C++98 hint-parameter driver over vector<int> iterators.
inline
void merge_apply_98_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    ::djinterp::internal::merge_sort_apply_98(
        _first,
        _last,
        less<int>(),
        static_cast<int*>(nullptr),
        static_cast<std::ptrdiff_t*>(nullptr));

    return;
}

// merge_apply_98_keyed
//   helper: the C++98 driver under the key-only comparator.
inline
void merge_apply_98_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    ::djinterp::internal::merge_sort_apply_98(
        _first,
        _last,
        keyed_value_less(),
        static_cast<keyed_value*>(nullptr),
        static_cast<std::ptrdiff_t*>(nullptr));

    return;
}

// merge_sort_ints
//   helper: the two-argument public entry point over vector<int>.
inline
void merge_sort_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    merge_sort(_first, _last);

    return;
}

// merge_sort_ints_descending
//   helper: the three-argument public entry point under greater<int>.
inline
void merge_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    merge_sort(_first, _last, greater<int>());

    return;
}

// merge_sort_ints_raw
//   helper: the public entry point over raw pointers.
inline
void merge_sort_ints_raw(
    int* _first,
    int* _last
)
{
    merge_sort(_first, _last);

    return;
}

// merge_sort_keyed
//   helper: the public entry point under the key-only comparator.
inline
void merge_sort_keyed(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    merge_sort(_first, _last, keyed_value_less());

    return;
}

// merge_sort_keyed_ordered_ascending
//   helper: merge_sort_ordered with sort_order::ascending.  merge_sort's
// ordered entry point takes a real sort_order, so it needs no probe.
inline
void merge_sort_keyed_ordered_ascending(
    std::vector<keyed_value>::iterator _first,
    std::vector<keyed_value>::iterator _last
)
{
    merge_sort_ordered(_first,
                       _last,
                       keyed_value_less(),
                       sort_order::ascending);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// traits -- merge_sort_traits
bool tests_merge_sort_traits_values();

// merge -- internal::merge_halves
bool tests_merge_sort_merge_halves_basic();
bool tests_merge_sort_merge_halves_empty_halves();
bool tests_merge_sort_merge_halves_exhaustion_tails();
bool tests_merge_sort_merge_halves_stability();

// recurse -- internal::merge_sort_recurse
bool tests_merge_sort_recurse_base_cases();
bool tests_merge_sort_recurse_ordering_battery();
bool tests_merge_sort_recurse_split_parities();

// cpp98 -- internal::merge_sort_apply_98
bool tests_merge_sort_cpp98_degenerate_ranges();
bool tests_merge_sort_cpp98_ordering_battery();
bool tests_merge_sort_cpp98_parity_with_apply();
bool tests_merge_sort_cpp98_stability();

// entry -- merge_sort / merge_sort_ordered
bool tests_merge_sort_entry_comparator();
bool tests_merge_sort_entry_default();
bool tests_merge_sort_entry_ordered();
bool tests_merge_sort_entry_stability();
bool tests_merge_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// merge_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// merge_sort_tests_runner.cpp.
inline dt::module_spec
merge_sort_spec()
{
    return dt::module_spec
    {
        "merge_sort.hpp",
        "merge sort: the merge primitive, the recursive driver, the C++98 "
        "variant, the published traits, and the public entry points.",
        {
            dt::block_spec
            {
                "traits",
                "merge_sort_traits",
                {
                    {
                        "merge_sort_traits_values",
                        "values",
                        &tests_merge_sort_traits_values
                    }
                }
            },
            dt::block_spec
            {
                "merge",
                "internal::merge_halves",
                {
                    {
                        "merge_sort_merge_halves_basic",
                        "halves basic",
                        &tests_merge_sort_merge_halves_basic
                    },
                    {
                        "merge_sort_merge_halves_empty_halves",
                        "halves empty halves",
                        &tests_merge_sort_merge_halves_empty_halves
                    },
                    {
                        "merge_sort_merge_halves_exhaustion_tails",
                        "halves exhaustion tails",
                        &tests_merge_sort_merge_halves_exhaustion_tails
                    },
                    {
                        "merge_sort_merge_halves_stability",
                        "halves stability",
                        &tests_merge_sort_merge_halves_stability
                    }
                }
            },
            dt::block_spec
            {
                "recurse",
                "internal::merge_sort_recurse",
                {
                    {
                        "merge_sort_recurse_base_cases",
                        "base cases",
                        &tests_merge_sort_recurse_base_cases
                    },
                    {
                        "merge_sort_recurse_ordering_battery",
                        "ordering battery",
                        &tests_merge_sort_recurse_ordering_battery
                    },
                    {
                        "merge_sort_recurse_split_parities",
                        "split parities",
                        &tests_merge_sort_recurse_split_parities
                    }
                }
            },
            dt::block_spec
            {
                "cpp98",
                "internal::merge_sort_apply_98",
                {
                    {
                        "merge_sort_cpp98_degenerate_ranges",
                        "degenerate ranges",
                        &tests_merge_sort_cpp98_degenerate_ranges
                    },
                    {
                        "merge_sort_cpp98_ordering_battery",
                        "ordering battery",
                        &tests_merge_sort_cpp98_ordering_battery
                    },
                    {
                        "merge_sort_cpp98_parity_with_apply",
                        "parity with apply",
                        &tests_merge_sort_cpp98_parity_with_apply
                    },
                    {
                        "merge_sort_cpp98_stability",
                        "stability",
                        &tests_merge_sort_cpp98_stability
                    }
                }
            },
            dt::block_spec
            {
                "entry",
                "merge_sort / merge_sort_ordered",
                {
                    {
                        "merge_sort_entry_comparator",
                        "comparator",
                        &tests_merge_sort_entry_comparator
                    },
                    {
                        "merge_sort_entry_default",
                        "default",
                        &tests_merge_sort_entry_default
                    },
                    {
                        "merge_sort_entry_ordered",
                        "ordered",
                        &tests_merge_sort_entry_ordered
                    },
                    {
                        "merge_sort_entry_stability",
                        "stability",
                        &tests_merge_sort_entry_stability
                    },
                    {
                        "merge_sort_entry_pointer_iterators",
                        "pointer iterators",
                        &tests_merge_sort_entry_pointer_iterators
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_MERGE_SORT_TESTS_
