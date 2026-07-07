/******************************************************************************
* djinterp [functional]                                  accumulator_tests.hpp
*
* Unit-test declarations for accumulator.hpp.
*   This header declares the full battery of accumulator tests and the small
* set of helpers shared between the per-section translation units. Each
* semantic section of accumulator.hpp maps to one .cpp file and one section
* runner declared here:
*
*   accumulator_tests_primitive.cpp    -> run_primitive_tests
*   accumulator_tests_reducers.cpp     -> run_reducers_tests
*   accumulator_tests_statistics.cpp   -> run_statistics_tests
*   accumulator_tests_positional.cpp   -> run_positional_tests
*   accumulator_tests_collectors.cpp   -> run_collectors_tests
*   accumulator_tests_predicates.cpp   -> run_predicates_tests
*   accumulator_tests_combinators.cpp  -> run_combinators_tests
*   accumulator_tests_combine.cpp      -> run_combine_tests
*   accumulator_tests_boxed.cpp        -> run_boxed_tests
*   accumulator_tests_traits.cpp       -> run_traits_tests
*
*   Every test is a parameterless predicate returning true on success and
* false on the first failed check. Tests live in djinterp::testing; the
* accumulator types under test are flat in djinterp (no functional
* namespace).
*
*   SPEC PROVIDER (DTEST_SPEC_MODE):
*   Defining DTEST_SPEC_MODE before including this header additionally pulls in
* the DTest authoring surface (test_defaults.hpp) and exposes
* accumulator_spec() (section IV) - the ten sections as a module_spec: one
* block per section, one test_spec per predicate, each stamped with a name and
* the one-line descriptor lifted from its section .cpp. run_module() lowers
* that spec into the six-kind tree and projects it onto the report / PDF, so
* the &&-folded section runners below (retained for a bare pass/fail answer and
* for the C++98 build) and the enriched spec are two views of one suite. The
* enriched runner in .../functional/accumulator/ consumes the spec; the section
* .cpp files, built without DTEST_SPEC_MODE, supply the predicate definitions
* that the spec references by address.
*
* path:      /tests/djinterp/core/functional/accumulator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      1.  D_INTERNAL_ACC_CHECK            (early-return assertion)
      2.  approx_eq                       (floating-point comparison)
      3.  point / by_x / by_y / is_even   (shared fixtures)
II.   PER-SECTION TEST DECLARATIONS
      1.  primitive       (accumulator class + make_accumulator)
      2.  reducers        (sum/product/count/count_if/min/max/min_by/max_by)
      3.  statistics      (mean/variance/stddev)
      4.  positional      (first/last/nth)
      5.  collectors      (joining/to_vector/to_map_by/group_by/histogram/top_k)
      6.  predicates      (all_match/any_match/none_match)
      7.  combinators     (contramap/map_output/filtered/take)
      8.  combine         (variadic parallel folds)
      9.  boxed           (boxed_accumulator/box_accumulator)
      10. traits          (is_accumulator/is_boxed_accumulator/concepts)
III.  SECTION RUNNERS + TOP-LEVEL AGGREGATE
IV.   SPEC PROVIDER                       (DTEST_SPEC_MODE: accumulator_spec)
*/

#ifndef DJINTERP_TESTING_FUNCTIONAL_ACCUMULATOR_
#define DJINTERP_TESTING_FUNCTIONAL_ACCUMULATOR_ 1

// std
#include <cstddef>
// djinterp
#include "../../inc/functional/accumulator.hpp"

#ifdef DTEST_SPEC_MODE
    // The enriched runner defines DTEST_SPEC_MODE and consumes accumulator_spec()
    // (section IV). Pull in the DTest authoring surface: module_spec /
    // block_spec / test_spec, build_enriched_tree, run_module, and the
    // test_option_set the runner configures. Resolved via the djinterp include
    // root, the same root the runners' <djinterp/core/djinterp.hpp> resolves
    // against. Gated so the C++98-subset section builds - which never define
    // DTEST_SPEC_MODE - stay free of the C++11+ framework.
#   include <djinterp/test/test_defaults.hpp>
#endif


///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                              ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_ACC_CHECK
//   macro: returns false from the enclosing test the moment a condition
// fails. Wrapped in a do/while so it is a single statement usable without
// surrounding braces.
#define D_INTERNAL_ACC_CHECK(_cond)                                           \
    do                                                                        \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


NS_DJINTERP
NS_TESTING


// approx_eq
//   function: tolerant equality for floating-point results. Avoids <cmath>
// to keep the test layer dependency-light and constexpr-friendly.
inline bool
approx_eq(
    double _a,
    double _b,
    double _epsilon = 1e-9
)
{
    double diff = _a - _b;

    if (diff < 0.0)
    {
        diff = -diff;
    }

    return (diff <= _epsilon);
}


// point
//   struct: minimal 2-D fixture used by key-based and projection tests
// (min_by / max_by / contramap / to_map_by / group_by).
struct point
{
    int x;
    int y;
};


// by_x
//   struct: projection functor extracting point::x.
struct by_x
{
    int operator()(const point& _p) const
    {
        return _p.x;
    }
};


// by_y
//   struct: projection functor extracting point::y.
struct by_y
{
    int operator()(const point& _p) const
    {
        return _p.y;
    }
};


// is_even
//   struct: unary predicate true for even integers.
struct is_even
{
    bool operator()(int _v) const
    {
        return ((_v % 2) == 0);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

// 1.  primitive
//////////////////////////////////////////
bool test_primitive_construct_and_state();
bool test_primitive_step_chaining();
bool test_primitive_finalize_is_pure();
bool test_primitive_run_container();
bool test_primitive_run_iterator_range();
bool test_primitive_run_raw_array();
bool test_primitive_run_empty_inputs();
bool test_primitive_functor_accessors();
bool test_primitive_make_accumulator();
bool test_primitive_copy_independence();

// 2.  reducers
//////////////////////////////////////////
bool test_reducers_sum();
bool test_reducers_product();
bool test_reducers_count();
bool test_reducers_count_if();
bool test_reducers_min();
bool test_reducers_max();
bool test_reducers_min_by();
bool test_reducers_max_by();
bool test_reducers_empty_edges();

// 3.  statistics
//////////////////////////////////////////
bool test_statistics_mean();
bool test_statistics_variance();
bool test_statistics_stddev();
bool test_statistics_degenerate();

// 4.  positional
//////////////////////////////////////////
bool test_positional_first();
bool test_positional_last();
bool test_positional_nth();
bool test_positional_empty_edges();

// 5.  collectors
//////////////////////////////////////////
bool test_collectors_joining();
bool test_collectors_to_vector();
bool test_collectors_histogram();
bool test_collectors_to_map_by();
bool test_collectors_group_by();
bool test_collectors_top_k();

// 6.  predicates
//////////////////////////////////////////
bool test_predicates_all_match();
bool test_predicates_any_match();
bool test_predicates_none_match();
bool test_predicates_empty_edges();

// 7.  combinators
//////////////////////////////////////////
bool test_combinators_contramap();
bool test_combinators_map_output();
bool test_combinators_filtered();
bool test_combinators_take();
bool test_combinators_composition();

// 8.  combine
//////////////////////////////////////////
bool test_combine_single();
bool test_combine_pair();
bool test_combine_heterogeneous();
bool test_combine_run_iterator_range();
bool test_combine_empty();

// 9.  boxed
//////////////////////////////////////////
bool test_boxed_run_container();
bool test_boxed_step_finalize();
bool test_boxed_run_non_vector_container();
bool test_boxed_factory_deduction();

// 10. traits
//////////////////////////////////////////
bool test_traits_is_accumulator();
bool test_traits_is_boxed_accumulator();
bool test_traits_member_detection();
bool test_traits_type_extraction();
bool test_traits_value_aliases();
bool test_traits_concepts();


///////////////////////////////////////////////////////////////////////////////
///                III. SECTION RUNNERS + TOP-LEVEL AGGREGATE                ///
///////////////////////////////////////////////////////////////////////////////

bool run_primitive_tests();
bool run_reducers_tests();
bool run_statistics_tests();
bool run_positional_tests();
bool run_collectors_tests();
bool run_predicates_tests();
bool run_combinators_tests();
bool run_combine_tests();
bool run_boxed_tests();
bool run_traits_tests();


// run_all_accumulator_tests
//   function: drives every section runner. Returns true only when all
// sections pass. Inlined here so the entry point is header-resident and the
// .cpp files stay purely sectional.
inline bool
run_all_accumulator_tests()
{
    return ( run_primitive_tests()   &&
             run_reducers_tests()    &&
             run_statistics_tests()  &&
             run_positional_tests()  &&
             run_collectors_tests()  &&
             run_predicates_tests()  &&
             run_combinators_tests() &&
             run_combine_tests()     &&
             run_boxed_tests()       &&
             run_traits_tests() );
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  SPEC PROVIDER  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// accumulator_spec
//   function: the suite as plain data for the enriched runner. Each of the
// ten sections becomes one block_spec; each predicate declared in section II
// becomes one test_spec carrying a name (the predicate's identifier minus the
// test_ prefix) and the one-line descriptor lifted verbatim from its section
// .cpp. run_module() lowers this into the six-kind tree and drives the report
// / PDF from the same data, running each predicate exactly once for its leaf
// verdict.
namespace dt = ::djinterp::test;

inline dt::module_spec
accumulator_spec()
{
    return dt::module_spec{
        "accumulator.hpp",
        "Reducers, statistics, positional folds, collectors, predicate folds, "
        "combinators, parallel combine, boxing, and traits for accumulator.hpp",
        {
            dt::block_spec{ "primitive",
                "accumulator class + make_accumulator",
                {
                    { "primitive_construct_and_state", "Constructs a pre-built accumulator and inspects its initial state.", &test_primitive_construct_and_state },
                    { "primitive_step_chaining",       "Drives a sum one value at a time.",                                  &test_primitive_step_chaining },
                    { "primitive_finalize_is_pure",    "Verifies finalize() does not disturb the accumulator state.",       &test_primitive_finalize_is_pure },
                    { "primitive_run_container",       "Folds an entire container in one call.",                            &test_primitive_run_container },
                    { "primitive_run_iterator_range",  "Folds a half-open iterator range.",                                 &test_primitive_run_iterator_range },
                    { "primitive_run_raw_array",       "Folds a raw pointer + count buffer.",                               &test_primitive_run_raw_array },
                    { "primitive_run_empty_inputs",    "Exercises every run() overload with no elements.",                  &test_primitive_run_empty_inputs },
                    { "primitive_functor_accessors",   "Uses the stored functors directly via the const accessors.",        &test_primitive_functor_accessors },
                    { "primitive_make_accumulator",    "Builds a bespoke accumulator from raw functors.",                   &test_primitive_make_accumulator },
                    { "primitive_copy_independence",   "Confirms copies progress independently after construction.",        &test_primitive_copy_independence }
                }
            },
            dt::block_spec{ "reducers",
                "sum / product / count / count_if / min / max / min_by / max_by",
                {
                    { "reducers_sum",         "Folds with additive identity.",                        &test_reducers_sum },
                    { "reducers_product",     "Folds with multiplicative identity.",                  &test_reducers_product },
                    { "reducers_count",       "Counts elements regardless of value.",                 &test_reducers_count },
                    { "reducers_count_if",    "Counts only elements satisfying a predicate.",         &test_reducers_count_if },
                    { "reducers_min",         "Tracks the running minimum.",                          &test_reducers_min },
                    { "reducers_max",         "Tracks the running maximum.",                          &test_reducers_max },
                    { "reducers_min_by",      "Minimum under a projection.",                          &test_reducers_min_by },
                    { "reducers_max_by",      "Maximum under a projection.",                          &test_reducers_max_by },
                    { "reducers_empty_edges", "Documents the empty-input behaviour of the reducers.", &test_reducers_empty_edges }
                }
            },
            dt::block_spec{ "statistics",
                "mean / variance / stddev (Welford, population)",
                {
                    { "statistics_mean",       "Computes the arithmetic mean.",                                       &test_statistics_mean },
                    { "statistics_variance",   "Computes the population variance via Welford's algorithm.",           &test_statistics_variance },
                    { "statistics_stddev",     "Computes the population standard deviation (Newton sqrt of variance).", &test_statistics_stddev },
                    { "statistics_degenerate", "Exercises the guarded edge cases of the statistics finalizers.",      &test_statistics_degenerate }
                }
            },
            dt::block_spec{ "positional",
                "first / last / nth",
                {
                    { "positional_first",       "Captures the first element seen.",                    &test_positional_first },
                    { "positional_last",        "Captures the last element seen.",                     &test_positional_last },
                    { "positional_nth",         "Selects the element at a given zero-based index.",    &test_positional_nth },
                    { "positional_empty_edges", "Documents the sentinel behaviour at the boundaries.", &test_positional_empty_edges }
                }
            },
            dt::block_spec{ "collectors",
                "joining / to_vector / histogram / to_map_by / group_by / top_k",
                {
                    { "collectors_joining",   "Concatenates stringified elements with a separator.",             &test_collectors_joining },
                    { "collectors_to_vector", "Materialises the stream into a vector.",                          &test_collectors_to_vector },
                    { "collectors_histogram", "Counts occurrences per distinct key.",                            &test_collectors_histogram },
                    { "collectors_to_map_by", "Indexes elements by a projected key, last-write-wins.",           &test_collectors_to_map_by },
                    { "collectors_group_by",  "Buckets elements by a projected key, preserving arrival order.",  &test_collectors_group_by },
                    { "collectors_top_k",     "Keeps the k largest elements in descending order.",               &test_collectors_top_k }
                }
            },
            dt::block_spec{ "predicates",
                "all_match / any_match / none_match",
                {
                    { "predicates_all_match",   "Folds a conjunction over a predicate.",                 &test_predicates_all_match },
                    { "predicates_any_match",   "Folds a disjunction over a predicate.",                 &test_predicates_any_match },
                    { "predicates_none_match",  "Folds a negated disjunction over a predicate.",         &test_predicates_none_match },
                    { "predicates_empty_edges", "Confirms the vacuous-truth conventions on empty input.", &test_predicates_empty_edges }
                }
            },
            dt::block_spec{ "combinators",
                "contramap / map_output / filtered / take",
                {
                    { "combinators_contramap",   "Pre-applies a projection to each input before the inner step.", &test_combinators_contramap },
                    { "combinators_map_output",  "Post-applies a transform to the inner accumulator's output.",    &test_combinators_map_output },
                    { "combinators_filtered",    "Gates the inner accumulator's input with a predicate.",          &test_combinators_filtered },
                    { "combinators_take",        "Caps how many inputs the inner accumulator observes.",           &test_combinators_take },
                    { "combinators_composition", "Stacks combinators to confirm they nest cleanly.",               &test_combinators_composition }
                }
            },
            dt::block_spec{ "combine",
                "variadic parallel folds (applicative)",
                {
                    { "combine_single",             "Combines a single accumulator.",                       &test_combine_single },
                    { "combine_pair",               "Runs two accumulators in lock-step over one pass.",    &test_combine_pair },
                    { "combine_heterogeneous",      "Combines accumulators with differing output types.",   &test_combine_heterogeneous },
                    { "combine_run_iterator_range", "Drives a combine over a half-open iterator range.",    &test_combine_run_iterator_range },
                    { "combine_empty",              "Combines over an empty input.",                        &test_combine_empty }
                }
            },
            dt::block_spec{ "boxed",
                "boxed_accumulator / box_accumulator (type erasure)",
                {
                    { "boxed_run_container",            "Folds a container through a type-erased accumulator.",         &test_boxed_run_container },
                    { "boxed_step_finalize",            "Drives a boxed accumulator one value at a time.",             &test_boxed_step_finalize },
                    { "boxed_run_non_vector_container", "Folds a non-vector container through the boxed run path.",     &test_boxed_run_non_vector_container },
                    { "boxed_factory_deduction",        "Confirms box_accumulator carries the inner accumulator's types.", &test_boxed_factory_deduction }
                }
            },
            dt::block_spec{ "traits",
                "is_accumulator / is_boxed_accumulator / concepts",
                {
                    { "traits_is_accumulator",       "Exercises the is_accumulator composite trait at run time.",           &test_traits_is_accumulator },
                    { "traits_is_boxed_accumulator", "Exercises the is_boxed_accumulator composite trait at run time.",      &test_traits_is_boxed_accumulator },
                    { "traits_member_detection",     "Exercises the individual member/typedef detectors and their composites.", &test_traits_member_detection },
                    { "traits_type_extraction",      "Exercises the accumulator_state_t / input_t / output_t aliases.",     &test_traits_type_extraction },
                    { "traits_value_aliases",        "Exercises the C++14 *_v variable-template shorthands.",               &test_traits_value_aliases },
                    { "traits_concepts",             "Exercises the C++20 accumulator concepts.",                           &test_traits_concepts }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_ACCUMULATOR_
