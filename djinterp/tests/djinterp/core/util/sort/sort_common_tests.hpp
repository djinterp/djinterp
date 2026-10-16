/*******************************************************************************
* djinterp [test]                                          sort_common_tests.hpp
*
*   unit-test suite for sort_common.hpp.
* Declares every tests_sort_common_* predicate and the helper types the
* suite's translation units share.
*
*   sections (one .cpp each):
*     test_sort_common_order.cpp             sort_order -- the enumerators and
*                                            the pinned underlying type
*     test_sort_common_comparators.cpp       less / greater -- strict weak
*                                            ordering on built-ins and on a
*                                            user type
*     test_sort_common_order_comparator.cpp  internal::order_comparator -- the
*                                            run-time direction flip, both
*                                            branches and the heterogeneous
*                                            call operator
*     test_sort_common_aliases.cpp           default_comparator /
*                                            default_comparator_t and the
*                                            include guard
*     test_sort_common_algebra.cpp           the functional comparator algebra
*                                            this header pulls in, exercised
*                                            through the sort entry points
*
*   The algebra section is deliberately the ONLY place the suites depend on
* functional/comparator.hpp's spelling.  If that header's surface differs from
* what sort_common.hpp documents, exactly one translation unit needs touching.
*
*
* path:      /tests/djinterp/core/util/sort/sort_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_COMMON_TESTS_
#define DJINTERP_TESTS_SORT_COMMON_TESTS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
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
///                    I.   HELPERS                                         ///
///////////////////////////////////////////////////////////////////////////////

// boxed_int
//   struct: a user-defined type with operator< and operator> but no other
// ordering support, used to prove that less / greater reach the operators of
// an arbitrary type rather than only built-ins.
struct boxed_int
{
    int value;
};

// make_boxed
//   function: builds a boxed_int.
inline
boxed_int make_boxed(
    int _value
)
{
    boxed_int result;

    result.value = _value;

    return result;
}

// operator<
//   function: ordering on the boxed value.
inline
bool operator<(
    const boxed_int& _a,
    const boxed_int& _b
)
{
    return (_a.value < _b.value);
}

// operator>
//   function: the reverse ordering on the boxed value.
inline
bool operator>(
    const boxed_int& _a,
    const boxed_int& _b
)
{
    return (_a.value > _b.value);
}

// even_first
//   struct: a comparator that is NOT a total order -- every even value
// precedes every odd one and values of the same parity are equivalent.  Used
// to prove order_comparator flips an arbitrary strict weak ordering, not just
// a natural one.
struct even_first
{
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return ( ((_a % 2) == 0) &&
                 ((_b % 2) != 0) );
    }
};

// keyed_tag_less
//   struct: orders keyed_value by TAG rather than key.  Used as the
// tie-breaker in the composed-comparator test, where it turns the key
// ordering into a total one.
struct keyed_tag_less
{
    bool operator()(
        const keyed_value& _a,
        const keyed_value& _b
    ) const
    {
        return (_a.tag < _b.tag);
    }
};

// cross_type_less
//   struct: a comparator whose call operator is itself a template.
// order_comparator forwards through a heterogeneous operator(), and that
// forwarding is only observable with a comparator that can actually accept
// two different types.
struct cross_type_less
{
    template<typename _A,
             typename _B>
    bool operator()(
        const _A& _a,
        const _B& _b
    ) const
    {
        return (_a < _b);
    }
};

// int_key_of
//   struct: a key extractor for the comparator algebra tests.
struct int_key_of
{
    int operator()(
        const keyed_value& _value
    ) const
    {
        return _value.key;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  UNIT TESTS                                      ///
///////////////////////////////////////////////////////////////////////////////

#endif  // !DTEST_SPEC_MODE  (fixtures)


// order -- sort_order
bool tests_sort_common_order_enumerators();
bool tests_sort_common_order_underlying_type();

// comparators -- less / greater
bool tests_sort_common_less_semantics();
bool tests_sort_common_greater_semantics();
bool tests_sort_common_comparator_irreflexivity();
bool tests_sort_common_comparator_on_user_type();

// order_comparator -- internal::order_comparator
bool tests_sort_common_order_comparator_ascending();
bool tests_sort_common_order_comparator_descending();
bool tests_sort_common_order_comparator_none();
bool tests_sort_common_order_comparator_heterogeneous();
bool tests_sort_common_order_comparator_drives_a_sort();

// aliases -- default_comparator / default_comparator_t
bool tests_sort_common_default_comparator();
bool tests_sort_common_default_comparator_alias();
bool tests_sort_common_include_guard();

// algebra -- the functional comparator vocabulary
bool tests_sort_common_is_comparator_trait();
bool tests_sort_common_natural_matches_less();
bool tests_sort_common_reversed_matches_greater();
bool tests_sort_common_composed_comparator_sorts();



#ifdef DTEST_SPEC_MODE

// sort_common_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_common_tests_runner.cpp.
inline dt::module_spec
sort_common_spec()
{
    return dt::module_spec
    {
        "sort_common.hpp",
        "core vocabulary: the sort_order enum, the less/greater comparator "
        "primitives, the run-time order_comparator wrapper, the "
        "default_comparator aliases and the functional comparator algebra "
        "the header re-exports.",
        {
            dt::block_spec
            {
                "order",
                "sort_order",
                {
                    {
                        "sort_common_order_enumerators",
                        "enumerators",
                        &tests_sort_common_order_enumerators
                    },
                    {
                        "sort_common_order_underlying_type",
                        "underlying type",
                        &tests_sort_common_order_underlying_type
                    }
                }
            },
            dt::block_spec
            {
                "comparators",
                "less / greater",
                {
                    {
                        "sort_common_less_semantics",
                        "less semantics",
                        &tests_sort_common_less_semantics
                    },
                    {
                        "sort_common_greater_semantics",
                        "greater semantics",
                        &tests_sort_common_greater_semantics
                    },
                    {
                        "sort_common_comparator_irreflexivity",
                        "comparator irreflexivity",
                        &tests_sort_common_comparator_irreflexivity
                    },
                    {
                        "sort_common_comparator_on_user_type",
                        "comparator on user type",
                        &tests_sort_common_comparator_on_user_type
                    }
                }
            },
            dt::block_spec
            {
                "order_comparator",
                "internal::order_comparator",
                {
                    {
                        "sort_common_order_comparator_ascending",
                        "ascending",
                        &tests_sort_common_order_comparator_ascending
                    },
                    {
                        "sort_common_order_comparator_descending",
                        "descending",
                        &tests_sort_common_order_comparator_descending
                    },
                    {
                        "sort_common_order_comparator_none",
                        "none",
                        &tests_sort_common_order_comparator_none
                    },
                    {
                        "sort_common_order_comparator_heterogeneous",
                        "heterogeneous",
                        &tests_sort_common_order_comparator_heterogeneous
                    },
                    {
                        "sort_common_order_comparator_drives_a_sort",
                        "drives a sort",
                        &tests_sort_common_order_comparator_drives_a_sort
                    }
                }
            },
            dt::block_spec
            {
                "aliases",
                "default_comparator / default_comparator_t",
                {
                    {
                        "sort_common_default_comparator",
                        "default comparator",
                        &tests_sort_common_default_comparator
                    },
                    {
                        "sort_common_default_comparator_alias",
                        "default comparator alias",
                        &tests_sort_common_default_comparator_alias
                    },
                    {
                        "sort_common_include_guard",
                        "include guard",
                        &tests_sort_common_include_guard
                    }
                }
            },
            dt::block_spec
            {
                "algebra",
                "the functional comparator vocabulary",
                {
                    {
                        "sort_common_is_comparator_trait",
                        "is comparator trait",
                        &tests_sort_common_is_comparator_trait
                    },
                    {
                        "sort_common_natural_matches_less",
                        "natural matches less",
                        &tests_sort_common_natural_matches_less
                    },
                    {
                        "sort_common_reversed_matches_greater",
                        "reversed matches greater",
                        &tests_sort_common_reversed_matches_greater
                    },
                    {
                        "sort_common_composed_comparator_sorts",
                        "composed comparator sorts",
                        &tests_sort_common_composed_comparator_sorts
                    }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_COMMON_TESTS_
