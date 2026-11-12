/*******************************************************************************
* djinterp [test]                                          sort_common_tests.hpp
*
*   unit-test suite for sort_common.hpp.
* Declares every tests_sort_common_* predicate and the helper types the
* suite's translation units share.
*
*   sections (one .cpp each):
*     sort_common_tests_order.cpp             sort_order -- the enumerators,
*                                             their parity with the C
*                                             module's dj_sort_order, and the
*                                             C++98 shape
*     sort_common_tests_order_comparator.cpp  internal::DOrderComparator --
*                                             the run-time direction flip,
*                                             both branches, mixed operand
*                                             types, a non-const comparator
*                                             and a greedy one
*     sort_common_tests_algebra.cpp           the functional comparator
*                                             algebra, driven through a sort
*                                             entry point
*     sort_common_tests_header.cpp            the include guard
*
*   The algebra section is the ONLY place the suite depends on
* functional/comparator.hpp's spelling.  sort_common.hpp does not include that
* header -- a caller who sorts with the algebra includes it -- so this suite
* does, for that one section.
*
*
* path:      /tests/djinterp/cpp/util/sort/sort_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.19
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_SORT_COMMON_TESTS_
#define DJINTERP_TESTS_SORT_COMMON_TESTS_ 1

// std
#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>
// djinterp
//   core is ALWAYS first and unconditional -- NS_* / D_* qualifiers and the
// language gates are needed by both faces of this header.
#include <djinterp/cpp/djinterp.hpp>
#ifndef DTEST_SPEC_MODE
#include "../../../../../inc/djinterp/cpp/util/sort/sort_common.hpp"
#include "../../../../../inc/djinterp/cpp/util/sort/insertion_sort.hpp"
#include "../../../../../inc/djinterp/cpp/functional/comparator.hpp"
#include "../../../../../inc/djinterp/c/util/sort/sort_common.h"
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
///                    I.   HELPERS                                         ///
///////////////////////////////////////////////////////////////////////////////

// DBoxedInt
//   struct: a user-defined type with operator< and no other ordering support,
// used to prove that the comparator trait reaches the operator of an arbitrary
// type rather than only built-ins.
struct DBoxedInt
{
    int value;
};

// make_boxed
//   function: builds a DBoxedInt.
inline
DBoxedInt make_boxed(
    int _value
)
{
    DBoxedInt result;

    result.value = _value;

    return result;
}

// operator<
//   function: ordering on the boxed value.
inline
bool operator<(
    const DBoxedInt& _left,
    const DBoxedInt& _right
)
{
    return (_left.value < _right.value);
}

// DEvenFirst
//   struct: a comparator that is NOT a total order -- every even value
// precedes every odd one and values of the same parity are equivalent.  Used
// to prove DOrderComparator flips an arbitrary strict weak ordering, not just
// a natural one.
struct DEvenFirst
{
    bool operator()(
        int _left,
        int _right
    ) const
    {
        return ( ((_left % 2) == 0) &&
                 ((_right % 2) != 0) );
    }
};

// DKeyedTagLess
//   struct: orders DKeyedValue by TAG rather than key.  Used as the
// tie-breaker in the composed-comparator test, where it turns the key
// ordering into a total one.
struct DKeyedTagLess
{
    bool operator()(
        const DKeyedValue& _left,
        const DKeyedValue& _right
    ) const
    {
        return (_left.tag < _right.tag);
    }
};

// DCrossTypeLess
//   struct: a comparator whose call operator is itself a template.
// DOrderComparator forwards through a call operator templated on both
// operands, and that forwarding is only observable with a comparator that can
// accept two different types.
struct DCrossTypeLess
{
    template<typename Left,
             typename Right>
    bool operator()(
        const Left&  _left,
        const Right& _right
    ) const
    {
        return (_left < _right);
    }
};

// DTallyingLess
//   struct: an ascending comparator whose call operator is NOT const -- it
// counts its own calls in a member.  DOrderComparator calls the comparator it
// holds as a non-const lvalue, so this compiles through it and the count lands
// on the wrapper's own copy.
struct DTallyingLess
{
    std::size_t calls;

    DTallyingLess()
        : calls(0)
    {}

    bool operator()(
        int _left,
        int _right
    )
    {
        ++calls;

        return (_left < _right);
    }
};

// DGreedyLess
//   struct: an ascending comparator with an UNCONSTRAINED forwarding
// constructor, as several of the comparator algebra's helpers have.  Handed a
// non-const DGreedyLess, that constructor outbids the copy constructor and
// tries to build the held std::less<int> from a DGreedyLess, which does not
// compile -- so a wrapper that ever copies one from a non-const lvalue fails
// to build rather than failing a check.
struct DGreedyLess
{
    DGreedyLess()
        : inner()
    {}

    template<typename Any>
    explicit DGreedyLess(
        Any&& _any
    )
        : inner(static_cast<Any&&>(_any))
    {}

    bool operator()(
        int _left,
        int _right
    ) const
    {
        return inner(_left, _right);
    }

    std::less<int> inner;
};

// DIntKeyOf
//   struct: a key extractor for the comparator algebra tests.
struct DIntKeyOf
{
    int operator()(
        const DKeyedValue& _value
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
bool tests_sort_common_order_matches_c_module();
bool tests_sort_common_order_shape();

// order_comparator -- internal::DOrderComparator
bool tests_sort_common_order_comparator_ascending();
bool tests_sort_common_order_comparator_descending();
bool tests_sort_common_order_comparator_none();
bool tests_sort_common_order_comparator_heterogeneous();
bool tests_sort_common_order_comparator_non_const();
bool tests_sort_common_order_comparator_copies_from_const();
bool tests_sort_common_order_comparator_drives_a_sort();

// algebra -- the functional comparator vocabulary
bool tests_sort_common_is_comparator_trait();
bool tests_sort_common_natural_matches_less();
bool tests_sort_common_reversed_matches_greater();
bool tests_sort_common_composed_comparator_sorts();

// header -- the include guard
bool tests_sort_common_include_guard();



#ifdef DTEST_SPEC_MODE

// sort_common_order_block
//   provider: the "order" block of sort_common_spec.
inline
dt::block_spec sort_common_order_block()
{
    return dt::block_spec
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
                "sort_common_order_matches_c_module",
                "matches the C module",
                &tests_sort_common_order_matches_c_module
            },
            {
                "sort_common_order_shape",
                "C++98 shape",
                &tests_sort_common_order_shape
            }
        }
    };
}

// sort_common_order_comparator_block
//   provider: the "order_comparator" block of sort_common_spec.
inline
dt::block_spec sort_common_order_comparator_block()
{
    return dt::block_spec
    {
        "order_comparator",
        "internal::DOrderComparator",
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
                "sort_common_order_comparator_non_const",
                "non-const comparator",
                &tests_sort_common_order_comparator_non_const
            },
            {
                "sort_common_order_comparator_copies_from_const",
                "copies from a const lvalue",
                &tests_sort_common_order_comparator_copies_from_const
            },
            {
                "sort_common_order_comparator_drives_a_sort",
                "drives a sort",
                &tests_sort_common_order_comparator_drives_a_sort
            }
        }
    };
}

// sort_common_algebra_block
//   provider: the "algebra" block of sort_common_spec.
inline
dt::block_spec sort_common_algebra_block()
{
    return dt::block_spec
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
                "natural matches std::less",
                &tests_sort_common_natural_matches_less
            },
            {
                "sort_common_reversed_matches_greater",
                "reversed matches std::greater",
                &tests_sort_common_reversed_matches_greater
            },
            {
                "sort_common_composed_comparator_sorts",
                "composed comparator sorts",
                &tests_sort_common_composed_comparator_sorts
            }
        }
    };
}

// sort_common_header_block
//   provider: the "header" block of sort_common_spec.
inline
dt::block_spec sort_common_header_block()
{
    return dt::block_spec
    {
        "header",
        "the include guard",
        {
            {
                "sort_common_include_guard",
                "include guard",
                &tests_sort_common_include_guard
            }
        }
    };
}

// sort_common_spec
//   provider: the module spec for this suite, handed to run_module by
// sort_common_tests_runner.cpp.
inline
dt::module_spec sort_common_spec()
{
    return dt::module_spec
    {
        "sort_common.hpp",
        "the shared vocabulary: the sort_order direction and its parity with "
        "the C module, the run-time DOrderComparator wrapper, the functional "
        "comparator algebra driven through a sort, and the include guard.",
        {
            sort_common_order_block(),
            sort_common_order_comparator_block(),
            sort_common_algebra_block(),
            sort_common_header_block()
        }
    };
}

#endif  // DTEST_SPEC_MODE

NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_SORT_COMMON_TESTS_
