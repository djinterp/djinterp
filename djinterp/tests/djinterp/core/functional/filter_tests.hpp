/******************************************************************************
* djinterp [functional]                                        filter_tests.hpp
*
* Unit-test declarations for filter.hpp (which now folds in the filterable
* container traits formerly in filterable_traits.hpp).
*   This header declares the full battery of filter tests and the helpers
* shared between the per-section translation units. Each semantic section of
* filter.hpp maps to one .cpp file and one section runner declared here:
*
*   filter_tests_result.cpp       -> run_result_tests        (III)
*   filter_tests_chain.cpp        -> run_chain_tests         (I, II, IV)
*   filter_tests_combinators.cpp  -> run_combinators_tests   (V)
*   filter_tests_iterator.cpp     -> run_iterator_tests      (VI)
*   filter_tests_builder.cpp      -> run_builder_tests       (VII)
*   filter_tests_typed.cpp        -> run_typed_tests         (VIII)
*   filter_tests_filterable.cpp   -> run_filterable_tests    (IX)
*   filter_tests_traits.cpp       -> run_traits_tests        (X)
*
*   Every test is a parameterless predicate returning true on success and
* false on the first failed check. Tests live in djinterp::testing; the
* filter types under test are flat djinterp types (no functional namespace).
*
*   FIXTURES use named functor types rather than lambdas. This is deliberate:
* the traits section takes decltype of types built from these callables, and
* a lambda in an unevaluated context is only legal from C++20. Named functors
* keep the whole suite compiling from C++11 upward.
*
*   SPEC PROVIDER (DTEST_SPEC_MODE):
*   Defining DTEST_SPEC_MODE before including this header additionally pulls in
* the DTest authoring surface (test_defaults.hpp) and exposes filter_spec()
* (section IV) - the eight sections as a module_spec: one block per section, one
* test_spec per predicate, each stamped with a name and the one-line descriptor
* lifted from its section .cpp. run_module() lowers that spec into the six-kind
* tree and projects it onto the report / PDF, so the &&-folded section runners
* below (retained for a bare pass/fail answer) and the enriched spec are two
* views of one suite. The enriched runner in .../functional/filter/ consumes the
* spec; the section .cpp files, built without DTEST_SPEC_MODE, supply the
* predicate definitions the spec references by address.
*
* path:      /test/functional/filter_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.02.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      1.  D_INTERNAL_FLT_CHECK            (early-return assertion)
      2.  predicates                      (is_even / is_positive / gt3 ...)
      3.  equality functors               (int_eq / mod3_eq)
      4.  native_filter_type              (.filter() member, for detection)
      5.  raw_op                           (filter_op_fn-shaped functor)
      6.  not_a_filter                     (negative fixture)
      7.  ints                             (vector<int> builder)
II.   PER-SECTION TEST DECLARATIONS
III.  SECTION RUNNERS + TOP-LEVEL AGGREGATE
IV.   SPEC PROVIDER                       (DTEST_SPEC_MODE: filter_spec)
*/

#ifndef DJINTERP_TEST_FUNCTIONAL_FILTER_
#define DJINTERP_TEST_FUNCTIONAL_FILTER_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
#include "../../inc/functional/filter.hpp"

#ifdef DTEST_SPEC_MODE
    // The enriched runner defines DTEST_SPEC_MODE and consumes filter_spec()
    // (section IV). Pull in the DTest authoring surface: module_spec /
    // block_spec / test_spec, build_enriched_tree, run_module, and the
    // test_option_set the runner configures. Resolved via the djinterp include
    // root, the same root the runners' <djinterp/core/djinterp.hpp> resolves
    // against. Gated so the plain sectional builds - which never define
    // DTEST_SPEC_MODE - do not pull in the framework.
#   include <djinterp/test/test_defaults.hpp>
#endif


///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                              ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_FLT_CHECK
//   macro: returns false from the enclosing test the moment a condition
// fails. Wrapped in a do/while so it is a single statement usable without
// surrounding braces.
#define D_INTERNAL_FLT_CHECK(_cond)                                           \
    do                                                                        \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


NS_DJINTERP
NS_TESTING


// is_even
//   struct: predicate, true for even ints.
struct is_even
{
    bool operator()(
        const int& _v
    ) const
    {
        return (_v % 2 == 0);
    }
};


// is_positive
//   struct: predicate, true for ints greater than zero.
struct is_positive
{
    bool operator()(
        const int& _v
    ) const
    {
        return (_v > 0);
    }
};


// gt3
//   struct: predicate, true for ints greater than three.
struct gt3
{
    bool operator()(
        const int& _v
    ) const
    {
        return (_v > 3);
    }
};


// int_eq
//   struct: binary equality on ints (for distinct custom-equality).
struct int_eq
{
    bool operator()(
        const int& _a,
        const int& _b
    ) const
    {
        return (_a == _b);
    }
};


// mod3_eq
//   struct: binary equivalence modulo three (for distinct custom-equality:
// collapses values sharing a residue class).
struct mod3_eq
{
    bool operator()(
        const int& _a,
        const int& _b
    ) const
    {
        return ((_a % 3) == (_b % 3));
    }
};


// native_filter_type
//   struct: a container-like type exposing a .filter(predicate) member and a
// value_type, used as the positive fixture for has_filter_method. Minimal by
// design -- it need not be fully filterable to exercise native detection.
struct native_filter_type
{
    typedef int value_type;

    native_filter_type filter(
        bool (*)(const int&)
    ) const
    {
        return *this;
    }
};


// raw_op
//   struct: a functor matching the filter_op_fn protocol (maps a
// vector<int> to a vector of surviving indices). Here it keeps even indices,
// but its behaviour is irrelevant to the structural traits -- only its shape
// matters. Named (not a lambda) so decltype is well-formed pre-C++20.
struct raw_op
{
    std::vector<std::size_t> operator()(
        const std::vector<int>& _in
    ) const
    {
        std::vector<std::size_t> out;
        for (std::size_t i = 0; i < _in.size(); i += 2)
        {
            out.push_back(i);
        }
        return out;
    }
};


// not_a_filter
//   struct: negative fixture. No call operator, no apply, no result surface.
struct not_a_filter
{
    int x;
};


// ints
//   function: small helper building a std::vector<int> from a count and a
// pointer, so the per-section files can spin up sample data without C++11
// initializer-list-to-vector friction in awkward spots. Most tests simply
// use brace-init directly; this is provided for the few that build from an
// array.
inline std::vector<int>
ints(
    const int*  _data,
    std::size_t _n
)
{
    return std::vector<int>(_data, _data + _n);
}


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

// III. result
//////////////////////////////////////////
bool test_result_success();
bool test_result_empty_status();
bool test_result_error_ctor();
bool test_result_take_elements();
bool test_result_iteration();

// I/II/IV. chain + op factories
//////////////////////////////////////////
bool test_chain_empty_passthrough();
bool test_chain_single_op();
bool test_chain_sequential_ops();
bool test_chain_length_and_clear();
bool test_chain_op_factories_positional();
bool test_chain_op_factories_edges();
bool test_chain_op_where_and_indices();
bool test_chain_op_distinct_reverse();

// V. combinators
//////////////////////////////////////////
bool test_combinators_union();
bool test_combinators_union_empty_list();
bool test_combinators_intersection();
bool test_combinators_intersection_zero_chains();
bool test_combinators_difference();

// VI. iterator
//////////////////////////////////////////
bool test_iterator_traversal();
bool test_iterator_reset();
bool test_iterator_empty();

// VII. builder
//////////////////////////////////////////
bool test_builder_take_skip();
bool test_builder_aliases();
bool test_builder_nth_range_slice();
bool test_builder_where_variants();
bool test_builder_at_variants();
bool test_builder_distinct_reverse();
bool test_builder_chained();
bool test_builder_apply_container();
bool test_builder_match_queries();
bool test_builder_build_chain();
bool test_builder_empty_input();

// VIII. typed fast-path
//////////////////////////////////////////
bool test_typed_identity();
bool test_typed_where();
bool test_typed_take_skip();
bool test_typed_composed();
bool test_typed_to_chain();

// IX. filterable container traits
//////////////////////////////////////////
bool test_filterable_member_detection();
bool test_filterable_iterable();
bool test_filterable_output_capable();
bool test_filterable_composite();
bool test_filterable_native_method();
bool test_filterable_value_type();

// X. filter structural traits & concepts
//////////////////////////////////////////
bool test_traits_is_filter_operation();
bool test_traits_is_filter_applicable();
bool test_traits_is_filter_result();
bool test_traits_value_aliases();
bool test_traits_concepts();


///////////////////////////////////////////////////////////////////////////////
///                III. SECTION RUNNERS + TOP-LEVEL AGGREGATE                ///
///////////////////////////////////////////////////////////////////////////////

bool run_result_tests();
bool run_chain_tests();
bool run_combinators_tests();
bool run_iterator_tests();
bool run_builder_tests();
bool run_typed_tests();
bool run_filterable_tests();
bool run_traits_tests();


// run_all_filter_tests
//   function: drives every section runner. Returns true only when all
// sections pass. Inlined here so the entry point is header-resident and the
// .cpp files stay purely sectional.
inline bool
run_all_filter_tests()
{
    return ( run_result_tests()       &&
             run_chain_tests()         &&
             run_combinators_tests()   &&
             run_iterator_tests()      &&
             run_builder_tests()       &&
             run_typed_tests()         &&
             run_filterable_tests()    &&
             run_traits_tests() );
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  SPEC PROVIDER  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// filter_spec
//   function: the suite as plain data for the enriched runner. Each of the
// eight sections becomes one block_spec; each predicate declared in section II
// becomes one test_spec carrying a name (the predicate's identifier minus the
// test_ prefix) and the one-line descriptor lifted verbatim from its section
// .cpp. Block order matches the section runners' document order. run_module()
// lowers this into the six-kind tree and drives the report / PDF from the same
// data, running each predicate exactly once for its leaf verdict.
namespace dt = ::djinterp::test;

inline dt::module_spec
filter_spec()
{
    return dt::module_spec{
        "filter.hpp",
        "filter_result, the op chain and factories, set combinators, the "
        "iterator, the builder facade, the typed fast-path, filterable "
        "container traits, and structural traits for filter.hpp",
        {
            dt::block_spec{ "result",
                "filter_result: status, inspection, take_elements, iteration",
                {
                    { "result_success",      "Verifies the success constructor of filter_result and its inspection surface.", &test_result_success },
                    { "result_empty_status", "Verifies the success constructor downgrades status to empty for an empty vector.", &test_result_empty_status },
                    { "result_error_ctor",   "Verifies the error constructor records a status and message and reports not-ok.", &test_result_error_ctor },
                    { "result_take_elements", "Verifies take_elements moves the stored elements out intact.",             &test_result_take_elements },
                    { "result_iteration",    "Verifies filter_result exposes begin/end so it is range-iterable.",         &test_result_iteration }
                }
            },
            dt::block_spec{ "chain",
                "filter_chain + the make_*_op operation factories",
                {
                    { "chain_empty_passthrough",       "Verifies an empty chain passes every element through unchanged.",          &test_chain_empty_passthrough },
                    { "chain_single_op",               "Verifies a one-op chain applies it and maps indices back to the input.",   &test_chain_single_op },
                    { "chain_sequential_ops",          "Verifies operations compose left-to-right with correct index mapping.",    &test_chain_sequential_ops },
                    { "chain_length_and_clear",        "Verifies the chain bookkeeping methods (length / is_empty / clear).",      &test_chain_length_and_clear },
                    { "chain_op_factories_positional", "Verifies the positional op factories (take/skip first/last, nth).",        &test_chain_op_factories_positional },
                    { "chain_op_factories_edges",      "Exercises the boundary behaviour of the positional / range / slice ops.",  &test_chain_op_factories_edges },
                    { "chain_op_where_and_indices",    "Verifies the predicate and index op factories (where / where_not / indices).", &test_chain_op_where_and_indices },
                    { "chain_op_distinct_reverse",     "Verifies the distinct and reverse op factories.",                          &test_chain_op_distinct_reverse }
                }
            },
            dt::block_spec{ "combinators",
                "filter_union / filter_intersection / filter_difference",
                {
                    { "combinators_union",                 "Verifies filter_union includes an element passing ANY chain, in input order.", &test_combinators_union },
                    { "combinators_union_empty_list",      "Verifies a union over no chains includes nothing.",                    &test_combinators_union_empty_list },
                    { "combinators_intersection",          "Verifies filter_intersection includes an element only if it passes EVERY chain.", &test_combinators_intersection },
                    { "combinators_intersection_zero_chains", "Pins the zero-chains edge: with no chains, all elements are included.", &test_combinators_intersection_zero_chains },
                    { "combinators_difference",            "Verifies filter_difference includes elements passing include but not exclude.", &test_combinators_difference }
                }
            },
            dt::block_spec{ "iterator",
                "filter_iterator: lazy traversal / reset",
                {
                    { "iterator_traversal", "Verifies the lazy iterator yields filtered elements in order and tracks remaining.", &test_iterator_traversal },
                    { "iterator_reset",     "Verifies reset returns the iterator to the start so it can be replayed.",   &test_iterator_reset },
                    { "iterator_empty",     "Verifies the iterator over a chain that selects nothing reports no next.",  &test_iterator_empty }
                }
            },
            dt::block_spec{ "builder",
                "filter_builder fluent surface",
                {
                    { "builder_take_skip",       "Verifies the take/skip family on the fluent builder.",                &test_builder_take_skip },
                    { "builder_aliases",         "Verifies the head/tail/init/rest convenience aliases.",               &test_builder_aliases },
                    { "builder_nth_range_slice", "Verifies take_nth, range, and slice on the builder.",                 &test_builder_nth_range_slice },
                    { "builder_where_variants",  "Verifies where and where_not on the builder.",                        &test_builder_where_variants },
                    { "builder_at_variants",     "Verifies at and at_indices, including out-of-range handling.",        &test_builder_at_variants },
                    { "builder_distinct_reverse", "Verifies distinct (default and custom equality) and reverse.",       &test_builder_distinct_reverse },
                    { "builder_chained",         "Verifies multi-stage fluent chaining preserves left-to-right order.", &test_builder_chained },
                    { "builder_apply_container", "Verifies the container-accepting apply overload converts a non-vector range.", &test_builder_apply_container },
                    { "builder_match_queries",   "Verifies the any/all/none/count match query methods.",                &test_builder_match_queries },
                    { "builder_build_chain",     "Verifies both build_chain overloads (lvalue const& and rvalue &&).",  &test_builder_build_chain },
                    { "builder_empty_input",     "Verifies builder operations are well-behaved on an empty input.",     &test_builder_empty_input }
                }
            },
            dt::block_spec{ "typed",
                "typed fast-path (de-erased)",
                {
                    { "typed_identity",  "Verifies the seed typed filter selects every element.",                    &test_typed_identity },
                    { "typed_where",     "Verifies a typed where stage keeps satisfying elements.",                  &test_typed_where },
                    { "typed_take_skip", "Verifies the typed take_first and skip_first stages.",                     &test_typed_take_skip },
                    { "typed_composed",  "Verifies multi-stage typed composition preserves left-to-right semantics.", &test_typed_composed },
                    { "typed_to_chain",  "Verifies to_chain lowers a typed filter into an erased filter_chain.",     &test_typed_to_chain }
                }
            },
            dt::block_spec{ "filterable",
                "filterable container traits (folded from filterable_traits.hpp)",
                {
                    { "filterable_member_detection", "Exercises the low-level member detectors at run time.",     &test_filterable_member_detection },
                    { "filterable_iterable",         "Exercises the is_iterable composite.",                      &test_filterable_iterable },
                    { "filterable_output_capable",   "Exercises the is_output_capable composite.",                &test_filterable_output_capable },
                    { "filterable_composite",        "Exercises the top-level is_filterable contract, incl. the array edge.", &test_filterable_composite },
                    { "filterable_native_method",    "Exercises has_filter_method.",                              &test_filterable_native_method },
                    { "filterable_value_type",       "Exercises filterable_value_t.",                             &test_filterable_value_type }
                }
            },
            dt::block_spec{ "traits",
                "filter structural traits & concepts",
                {
                    { "traits_is_filter_operation",  "Exercises is_filter_operation at run time.",                 &test_traits_is_filter_operation },
                    { "traits_is_filter_applicable", "Exercises is_filter_applicable at run time.",                &test_traits_is_filter_applicable },
                    { "traits_is_filter_result",     "Exercises is_filter_result at run time.",                    &test_traits_is_filter_result },
                    { "traits_value_aliases",        "Exercises the C++14 *_v variable-template shorthands (new and folded).", &test_traits_value_aliases },
                    { "traits_concepts",             "Exercises the C++20 concepts (new and folded).",            &test_traits_concepts }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTIONAL_FILTER_
