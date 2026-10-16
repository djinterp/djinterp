/*******************************************************************************
* djinterp [test]                                          sort_monoid_tests.hpp
*
*   unit-test suite for sort_monoid.hpp.
* Declares every tests_sort_monoid_* predicate and the helpers the suite's
* translation units share.
*
*   sections (one .cpp each):
*     test_sort_monoid_run.cpp         sorted_run -- construction, accessors,
*                                      and the adopting constructor
*     test_sort_monoid_merge.cpp       internal::merge_sorted_helper -- the
*                                      two-pointer merge and both tails
*     test_sort_monoid_algebra.cpp     the semigroup / monoid registration and
*                                      the laws: identity, associativity, and
*                                      mconcat as a bottom-up merge sort
*     test_sort_monoid_factories.cpp   make_sorted_run, adopt_sorted_run,
*                                      singleton_run, to_singleton_runs
*
*   The typeclass protocol (semigroup_traits / monoid_traits / mappend /
* mempty / mconcat) is used only in this suite, so a change in the functional
* layer's spelling is contained here.
*
*
* path:      /tests/djinterp/core/util/sort/sort_monoid_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_MONOID_TESTS_
#define DJINTERP_TESTS_SORT_MONOID_TESTS_ 1

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

// int_run
//   type: the run under test in most cases -- ints in the subsystem's default
// ascending order.
typedef sorted_run<int, default_comparator_t<int> > int_run;

// keyed_run
//   type: a run of keyed_value ordered by key alone, so that the merge's
// left-favouring tie-break is observable as stability.
typedef sorted_run<keyed_value, keyed_value_less> keyed_run;

// ints_of
//   function: builds a vector<int> from a C array, so the test bodies stay
// free of aggregate-initialisation noise.
inline
std::vector<int> ints_of(
    const int*  _values,
    std::size_t _count
)
{
    std::vector<int> result;
    std::size_t      i;

    result.reserve(_count);

    for (i = 0; i < _count; ++i)
    {
        result.push_back(_values[i]);
    }

    return result;
}

// keyed_of
//   function: builds a vector<keyed_value> from parallel key and tag arrays.
inline
std::vector<keyed_value> keyed_of(
    const int*  _keys,
    const int*  _tags,
    std::size_t _count
)
{
    std::vector<keyed_value> result;
    std::size_t              i;

    result.reserve(_count);

    for (i = 0; i < _count; ++i)
    {
        result.push_back(make_keyed(_keys[i], _tags[i]));
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// run -- sorted_run
bool tests_sort_monoid_run_default_construction();
bool tests_sort_monoid_run_comparator_construction();
bool tests_sort_monoid_run_adopting_construction();
bool tests_sort_monoid_run_accessors();
bool tests_sort_monoid_run_value_semantics();

// merge -- internal::merge_sorted_helper
bool tests_sort_monoid_merge_interleaves();
bool tests_sort_monoid_merge_empty_operands();
bool tests_sort_monoid_merge_exhaustion_tails();
bool tests_sort_monoid_merge_is_stable();

// algebra -- semigroup / monoid registration and laws
bool tests_sort_monoid_semigroup_registration();
bool tests_sort_monoid_monoid_registration();
bool tests_sort_monoid_mappend_merges();
bool tests_sort_monoid_mempty_is_identity();
bool tests_sort_monoid_identity_laws();
bool tests_sort_monoid_associativity();
bool tests_sort_monoid_mconcat_is_merge_sort();

// factories -- make / adopt / singleton / to_singleton_runs
bool tests_sort_monoid_make_sorted_run();
bool tests_sort_monoid_make_sorted_run_default();
bool tests_sort_monoid_adopt_sorted_run();
bool tests_sort_monoid_singleton_run();
bool tests_sort_monoid_to_singleton_runs();



#ifdef DTEST_SPEC_MODE

// sort_monoid_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_monoid_tests_runner.cpp.
inline dt::module_spec
sort_monoid_spec()
{
    return dt::module_spec
    {
        "sort_monoid.hpp",
        "the sorted_run merge monoid: the newtype and its constructors, the "
        "stable merge primitive, the semigroup and monoid registrations "
        "with their laws, and the run factories.",
        {
            dt::block_spec
            {
                "run",
                "sorted_run",
                {
                    {
                        "sort_monoid_run_default_construction",
                        "default construction",
                        &tests_sort_monoid_run_default_construction
                    },
                    {
                        "sort_monoid_run_comparator_construction",
                        "comparator construction",
                        &tests_sort_monoid_run_comparator_construction
                    },
                    {
                        "sort_monoid_run_adopting_construction",
                        "adopting construction",
                        &tests_sort_monoid_run_adopting_construction
                    },
                    {
                        "sort_monoid_run_accessors",
                        "accessors",
                        &tests_sort_monoid_run_accessors
                    },
                    {
                        "sort_monoid_run_value_semantics",
                        "value semantics",
                        &tests_sort_monoid_run_value_semantics
                    }
                }
            },
            dt::block_spec
            {
                "merge",
                "internal::merge_sorted_helper",
                {
                    {
                        "sort_monoid_merge_interleaves",
                        "interleaves",
                        &tests_sort_monoid_merge_interleaves
                    },
                    {
                        "sort_monoid_merge_empty_operands",
                        "empty operands",
                        &tests_sort_monoid_merge_empty_operands
                    },
                    {
                        "sort_monoid_merge_exhaustion_tails",
                        "exhaustion tails",
                        &tests_sort_monoid_merge_exhaustion_tails
                    },
                    {
                        "sort_monoid_merge_is_stable",
                        "is stable",
                        &tests_sort_monoid_merge_is_stable
                    }
                }
            },
            dt::block_spec
            {
                "algebra",
                "semigroup / monoid registration and laws",
                {
                    {
                        "sort_monoid_semigroup_registration",
                        "semigroup registration",
                        &tests_sort_monoid_semigroup_registration
                    },
                    {
                        "sort_monoid_monoid_registration",
                        "monoid registration",
                        &tests_sort_monoid_monoid_registration
                    },
                    {
                        "sort_monoid_mappend_merges",
                        "mappend merges",
                        &tests_sort_monoid_mappend_merges
                    },
                    {
                        "sort_monoid_mempty_is_identity",
                        "mempty is identity",
                        &tests_sort_monoid_mempty_is_identity
                    },
                    {
                        "sort_monoid_identity_laws",
                        "identity laws",
                        &tests_sort_monoid_identity_laws
                    },
                    {
                        "sort_monoid_associativity",
                        "associativity",
                        &tests_sort_monoid_associativity
                    },
                    {
                        "sort_monoid_mconcat_is_merge_sort",
                        "mconcat is merge sort",
                        &tests_sort_monoid_mconcat_is_merge_sort
                    }
                }
            },
            dt::block_spec
            {
                "factories",
                "make / adopt / singleton / to_singleton_runs",
                {
                    {
                        "sort_monoid_make_sorted_run",
                        "make sorted run",
                        &tests_sort_monoid_make_sorted_run
                    },
                    {
                        "sort_monoid_make_sorted_run_default",
                        "make sorted run default",
                        &tests_sort_monoid_make_sorted_run_default
                    },
                    {
                        "sort_monoid_adopt_sorted_run",
                        "adopt sorted run",
                        &tests_sort_monoid_adopt_sorted_run
                    },
                    {
                        "sort_monoid_singleton_run",
                        "singleton run",
                        &tests_sort_monoid_singleton_run
                    },
                    {
                        "sort_monoid_to_singleton_runs",
                        "to singleton runs",
                        &tests_sort_monoid_to_singleton_runs
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_MONOID_TESTS_
