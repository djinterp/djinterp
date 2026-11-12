/*******************************************************************************
* djinterp [test]                                           merge_sort_tests.hpp
*
*   unit-test suite for merge_sort.hpp.
* Declares every tests_merge_sort_* predicate and the adapters the suite's
* translation units share.
*
*   sections (one .cpp each):
*     merge_sort_tests_traits.cpp    merge_sort_traits -- the property block,
*                                    and the adaptivity it claims
*     merge_sort_tests_merge.cpp     internal::merge_runs -- the merge
*                                    primitive in isolation: both exhaustion
*                                    tails, the left-favouring tie and the
*                                    ordered-pair copy
*     merge_sort_tests_pass.cpp      internal::merge_pass and the ping-pong of
*                                    internal::merge_sort_apply -- short and
*                                    lone runs, and a result that ends in the
*                                    scratch
*     merge_sort_tests_buffered.cpp  merge_sort_buffered -- the caller's
*                                    scratch, held to parity with merge_sort
*     merge_sort_tests_entry.cpp     the public merge_sort /
*                                    merge_sort_ordered entry points
*
*   The merge sort here is bottom-up: passes of doubling width, each reading
* one buffer and writing the other.  merge_runs and merge_pass take the source
* and the destination separately and index them by position, so the adapters
* below hand them two vectors and positions rather than iterators into one.
*
*
* path:      /tests/djinterp/cpp/util/sort/merge_sort_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_MERGE_SORT_TESTS_
#define DJINTERP_TESTS_MERGE_SORT_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/util/sort/merge_sort.hpp"
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

// merge_runs_ints
//   helper: merge_runs over vector<int>, ascending: merges [_begin, _mid) and
// [_mid, _end) of _src into the same span of _dst.
inline
void merge_runs_ints(
    const std::vector<int>& _src,
    std::vector<int>&       _dst,
    std::ptrdiff_t          _begin,
    std::ptrdiff_t          _mid,
    std::ptrdiff_t          _end
)
{
    ::djinterp::internal::merge_runs(_src.begin(),
                                     _dst.begin(),
                                     _begin,
                                     _mid,
                                     _end,
                                     std::less<int>());

    return;
}

// merge_runs_keyed
//   helper: merge_runs under the key-only comparator, so the merge's
// left-favouring tie -- the source of the algorithm's stability -- is
// directly observable.
inline
void merge_runs_keyed(
    const std::vector<DKeyedValue>& _src,
    std::vector<DKeyedValue>&       _dst,
    std::ptrdiff_t                  _begin,
    std::ptrdiff_t                  _mid,
    std::ptrdiff_t                  _end
)
{
    ::djinterp::internal::merge_runs(_src.begin(),
                                     _dst.begin(),
                                     _begin,
                                     _mid,
                                     _end,
                                     keyed_value_less());

    return;
}

// merge_pass_ints
//   helper: one merge_pass of width _width over all of _src into _dst,
// ascending.
inline
void merge_pass_ints(
    const std::vector<int>& _src,
    std::vector<int>&       _dst,
    std::ptrdiff_t          _width
)
{
    ::djinterp::internal::merge_pass(_src.begin(),
                                     _dst.begin(),
                                     static_cast<std::ptrdiff_t>(_src.size()),
                                     _width,
                                     std::less<int>());

    return;
}

// merge_apply_ints
//   helper: the internal driver over vector<int> iterators, with a scratch
// buffer it owns for the call -- what the three-argument entry point does.
inline
void merge_apply_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    std::vector<int> scratch(_first, _last);

    ::djinterp::internal::merge_sort_apply(_first,
                                           _last,
                                           scratch.begin(),
                                           std::less<int>());

    return;
}

// merge_buffered_ints
//   helper: merge_sort_buffered over vector<int>, with a fresh caller-owned
// scratch of exactly the range's length.
inline
void merge_buffered_ints(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    std::vector<int> scratch(_first, _last);

    merge_sort_buffered(_first,
                        _last,
                        std::less<int>(),
                        scratch.begin());

    return;
}

// merge_buffered_keyed
//   helper: merge_sort_buffered under the key-only comparator.
inline
void merge_buffered_keyed(
    std::vector<DKeyedValue>::iterator _first,
    std::vector<DKeyedValue>::iterator _last
)
{
    std::vector<DKeyedValue> scratch(_first, _last);

    merge_sort_buffered(_first,
                        _last,
                        keyed_value_less(),
                        scratch.begin());

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
//   helper: the three-argument public entry point under std::greater<int>.
inline
void merge_sort_ints_descending(
    std::vector<int>::iterator _first,
    std::vector<int>::iterator _last
)
{
    merge_sort(_first, _last, std::greater<int>());

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
    std::vector<DKeyedValue>::iterator _first,
    std::vector<DKeyedValue>::iterator _last
)
{
    merge_sort(_first, _last, keyed_value_less());

    return;
}

// merge_sort_keyed_ordered_ascending
//   helper: merge_sort_ordered with sort_order::ascending.
inline
void merge_sort_keyed_ordered_ascending(
    std::vector<DKeyedValue>::iterator _first,
    std::vector<DKeyedValue>::iterator _last
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
bool tests_merge_sort_traits_adaptivity();

// merge -- internal::merge_runs
bool tests_merge_sort_merge_runs_basic();
bool tests_merge_sort_merge_runs_empty_runs();
bool tests_merge_sort_merge_runs_exhaustion_tails();
bool tests_merge_sort_merge_runs_stability();
bool tests_merge_sort_merge_runs_ordered_pair_copied();

// pass -- internal::merge_pass / the ping-pong
bool tests_merge_sort_pass_short_and_lone_runs();
bool tests_merge_sort_pass_degenerate_widths();
bool tests_merge_sort_apply_ordering_battery();
bool tests_merge_sort_apply_pass_parities();

// buffered -- merge_sort_buffered
bool tests_merge_sort_buffered_degenerate_ranges();
bool tests_merge_sort_buffered_ordering_battery();
bool tests_merge_sort_buffered_parity_with_merge_sort();
bool tests_merge_sort_buffered_stability();
bool tests_merge_sort_buffered_reuses_scratch();

// entry -- merge_sort / merge_sort_ordered
bool tests_merge_sort_entry_comparator();
bool tests_merge_sort_entry_default();
bool tests_merge_sort_entry_ordered();
bool tests_merge_sort_entry_stability();
bool tests_merge_sort_entry_pointer_iterators();



#ifdef DTEST_SPEC_MODE

// merge_sort_traits_block
//   provider: the "traits" block of merge_sort_spec.
inline
dt::block_spec merge_sort_traits_block()
{
    return dt::block_spec
    {
        "traits",
        "merge_sort_traits",
        {
            {
                "merge_sort_traits_values",
                "values",
                &tests_merge_sort_traits_values
            },
            {
                "merge_sort_traits_adaptivity",
                "adaptivity",
                &tests_merge_sort_traits_adaptivity
            }
        }
    };
}

// merge_sort_merge_block
//   provider: the "merge" block of merge_sort_spec.
inline
dt::block_spec merge_sort_merge_block()
{
    return dt::block_spec
    {
        "merge",
        "internal::merge_runs",
        {
            {
                "merge_sort_merge_runs_basic",
                "basic",
                &tests_merge_sort_merge_runs_basic
            },
            {
                "merge_sort_merge_runs_empty_runs",
                "empty runs",
                &tests_merge_sort_merge_runs_empty_runs
            },
            {
                "merge_sort_merge_runs_exhaustion_tails",
                "exhaustion tails",
                &tests_merge_sort_merge_runs_exhaustion_tails
            },
            {
                "merge_sort_merge_runs_stability",
                "stability",
                &tests_merge_sort_merge_runs_stability
            },
            {
                "merge_sort_merge_runs_ordered_pair_copied",
                "an ordered pair is copied",
                &tests_merge_sort_merge_runs_ordered_pair_copied
            }
        }
    };
}

// merge_sort_pass_block
//   provider: the "pass" block of merge_sort_spec.
inline
dt::block_spec merge_sort_pass_block()
{
    return dt::block_spec
    {
        "pass",
        "internal::merge_pass / the ping-pong",
        {
            {
                "merge_sort_pass_short_and_lone_runs",
                "short and lone runs",
                &tests_merge_sort_pass_short_and_lone_runs
            },
            {
                "merge_sort_pass_degenerate_widths",
                "degenerate widths",
                &tests_merge_sort_pass_degenerate_widths
            },
            {
                "merge_sort_apply_ordering_battery",
                "ordering battery",
                &tests_merge_sort_apply_ordering_battery
            },
            {
                "merge_sort_apply_pass_parities",
                "pass parities",
                &tests_merge_sort_apply_pass_parities
            }
        }
    };
}

// merge_sort_buffered_block
//   provider: the "buffered" block of merge_sort_spec.
inline
dt::block_spec merge_sort_buffered_block()
{
    return dt::block_spec
    {
        "buffered",
        "merge_sort_buffered",
        {
            {
                "merge_sort_buffered_degenerate_ranges",
                "degenerate ranges",
                &tests_merge_sort_buffered_degenerate_ranges
            },
            {
                "merge_sort_buffered_ordering_battery",
                "ordering battery",
                &tests_merge_sort_buffered_ordering_battery
            },
            {
                "merge_sort_buffered_parity_with_merge_sort",
                "parity with merge_sort",
                &tests_merge_sort_buffered_parity_with_merge_sort
            },
            {
                "merge_sort_buffered_stability",
                "stability",
                &tests_merge_sort_buffered_stability
            },
            {
                "merge_sort_buffered_reuses_scratch",
                "reuses its scratch",
                &tests_merge_sort_buffered_reuses_scratch
            }
        }
    };
}

// merge_sort_entry_block
//   provider: the "entry" block of merge_sort_spec.
inline
dt::block_spec merge_sort_entry_block()
{
    return dt::block_spec
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
    };
}

// merge_sort_spec
//   provider: the module spec for this suite, handed to run_module by
// merge_sort_tests_runner.cpp.
inline
dt::module_spec merge_sort_spec()
{
    return dt::module_spec
    {
        "merge_sort.hpp",
        "merge sort: the published traits, the merge_runs primitive, the "
        "bottom-up passes and their ping-pong, the caller-scratch entry "
        "point, and the public entry points.",
        {
            merge_sort_traits_block(),
            merge_sort_merge_block(),
            merge_sort_pass_block(),
            merge_sort_buffered_block(),
            merge_sort_entry_block()
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_MERGE_SORT_TESTS_
