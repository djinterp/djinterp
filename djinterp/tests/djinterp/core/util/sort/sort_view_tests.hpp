/*******************************************************************************
* djinterp [test]                                            sort_view_tests.hpp
*
*   unit-test suite for sort_view.hpp.
* Declares every tests_sort_view_* predicate and the helpers the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_sort_view_terminals.cpp   internal::sort_default_terminal and
*                                    internal::sort_with_terminal -- the drain,
*                                    the algorithm selection, and the empty
*                                    pipeline
*     test_sort_view_factories.cpp   sorted() / sorted_by() and the operator|
*                                    plumbing they are driven through
*
*   The view protocol (operator|, the terminal contract, views::transform and
* friends) is used only in this suite, so a change in functional/view.hpp is
* contained here.
*
*
* path:      /tests/djinterp/core/util/sort/sort_view_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_VIEW_TESTS_
#define DJINTERP_TESTS_SORT_VIEW_TESTS_ 1

// std
#include <cstddef>
#include <type_traits>
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
///                    I.   HELPERS                                         ///
///////////////////////////////////////////////////////////////////////////////

// negate_int
//   struct: a unary transform used to build an adapted view feeding a sort
// terminal.
struct negate_int
{
    int operator()(
        int _value
    ) const
    {
        return (-_value);
    }
};

// is_odd
//   struct: a unary predicate used to build a filtered view feeding a sort
// terminal.
struct is_odd
{
    bool operator()(
        int _value
    ) const
    {
        return (((_value % 2) != 0));
    }
};

// identity_int
//   struct: a unary transform that returns its argument unchanged.  Used to
// lift a plain container into a view without altering the values, so a
// terminal's apply() can be driven directly.
struct identity_int
{
    int operator()(
        int _value
    ) const
    {
        return _value;
    }
};

// is_negative
//   struct: a unary predicate that rejects every element of the sample, used
// to build a pipeline that yields nothing.
struct is_negative
{
    bool operator()(
        int _value
    ) const
    {
        return (_value < 0);
    }
};

// unrecognised_view_algorithm
//   function: a sort_algorithm value outside the six enumerators, produced
// the only way a caller could produce one -- by casting.  The terminals
// forward it to the run-time dispatcher, whose switch has no default label.
inline
sort_algorithm unrecognised_view_algorithm()
{
    return static_cast<sort_algorithm>(99);
}

// view_sample
//   function: a small unordered int sequence with duplicates, the shared
// input for the pipeline tests.
inline
std::vector<int> view_sample()
{
    std::vector<int> data;

    data.push_back(5);
    data.push_back(1);
    data.push_back(4);
    data.push_back(1);
    data.push_back(9);
    data.push_back(2);
    data.push_back(6);

    return data;
}

// view_sample_storage
//   function: the same sequence, held in storage that outlives the
// expression.  A view refers to its source rather than owning it, so a
// pipeline built over a temporary container would dangle.
inline
const std::vector<int>& view_sample_storage()
{
    static const std::vector<int> data = view_sample();

    return data;
}


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// terminals -- internal::sort_default_terminal / sort_with_terminal
bool tests_sort_view_default_terminal_applies();
bool tests_sort_view_with_terminal_applies();
bool tests_sort_view_terminal_algorithm_selection();
bool tests_sort_view_terminal_empty_pipeline();
bool tests_sort_view_terminal_is_recognised();

// factories -- sorted() / sorted_by()
bool tests_sort_view_sorted_default_algorithm();
bool tests_sort_view_sorted_explicit_algorithm();
bool tests_sort_view_sorted_by_comparator();
bool tests_sort_view_sorted_by_explicit_algorithm();
bool tests_sort_view_pipe_from_container();
bool tests_sort_view_pipe_from_adapted_view();



#ifdef DTEST_SPEC_MODE

// sort_view_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_view_tests_runner.cpp.
inline dt::module_spec
sort_view_spec()
{
    return dt::module_spec
    {
        "sort_view.hpp",
        "the bridge to the functional view subsystem: the two sort "
        "terminals and the sorted() / sorted_by() factories that build "
        "them.",
        {
            dt::block_spec
            {
                "terminals",
                "internal::sort_default_terminal / sort_with_terminal",
                {
                    {
                        "sort_view_default_terminal_applies",
                        "default terminal applies",
                        &tests_sort_view_default_terminal_applies
                    },
                    {
                        "sort_view_with_terminal_applies",
                        "with terminal applies",
                        &tests_sort_view_with_terminal_applies
                    },
                    {
                        "sort_view_terminal_algorithm_selection",
                        "terminal algorithm selection",
                        &tests_sort_view_terminal_algorithm_selection
                    },
                    {
                        "sort_view_terminal_empty_pipeline",
                        "terminal empty pipeline",
                        &tests_sort_view_terminal_empty_pipeline
                    },
                    {
                        "sort_view_terminal_is_recognised",
                        "terminal is recognised",
                        &tests_sort_view_terminal_is_recognised
                    }
                }
            },
            dt::block_spec
            {
                "factories",
                "sorted() / sorted_by()",
                {
                    {
                        "sort_view_sorted_default_algorithm",
                        "sorted default algorithm",
                        &tests_sort_view_sorted_default_algorithm
                    },
                    {
                        "sort_view_sorted_explicit_algorithm",
                        "sorted explicit algorithm",
                        &tests_sort_view_sorted_explicit_algorithm
                    },
                    {
                        "sort_view_sorted_by_comparator",
                        "sorted by comparator",
                        &tests_sort_view_sorted_by_comparator
                    },
                    {
                        "sort_view_sorted_by_explicit_algorithm",
                        "sorted by explicit algorithm",
                        &tests_sort_view_sorted_by_explicit_algorithm
                    },
                    {
                        "sort_view_pipe_from_container",
                        "pipe from container",
                        &tests_sort_view_pipe_from_container
                    },
                    {
                        "sort_view_pipe_from_adapted_view",
                        "pipe from adapted view",
                        &tests_sort_view_pipe_from_adapted_view
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_VIEW_TESTS_
