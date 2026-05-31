/******************************************************************************
* djinterp [functional]                                   accumulator_tests.hpp
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
* path:      /test/functional/accumulator_tests.hpp
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
*/

#ifndef DJINTERP_TEST_FUNCTIONAL_ACCUMULATOR_
#define DJINTERP_TEST_FUNCTIONAL_ACCUMULATOR_ 1

// std
#include <cstddef>
// djinterp
#include "../../inc/functional/accumulator.hpp"


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


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTIONAL_ACCUMULATOR_
