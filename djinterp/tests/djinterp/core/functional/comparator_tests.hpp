/******************************************************************************
* djinterp [functional]                                    comparator_tests.hpp
*
* Unit-test declarations for comparator.hpp.
*   This header declares the full battery of comparator tests and the small
* set of helpers shared between the per-section translation units. Each
* semantic section of comparator.hpp maps to one .cpp file and one section
* runner declared here:
*
*   comparator_tests_primitive.cpp    -> run_primitive_tests
*   comparator_tests_combinators.cpp  -> run_combinators_tests
*   comparator_tests_pipeline.cpp     -> run_pipeline_tests
*   comparator_tests_predicates.cpp   -> run_predicates_tests
*   comparator_tests_traits.cpp       -> run_traits_tests
*
*   Every test is a parameterless predicate returning true on success and
* false on the first failed check. Tests live in djinterp::testing; the
* comparator factories under test are reached unqualified through the
* djinterp::comparators namespace (no functional namespace).
*
*   PORTABILITY: the fixtures and tests are deliberately written to the
* C++98 subset (named functors instead of lambdas, no auto/decltype outside
* the guarded traits section) so the same suite compiles and runs against
* both implementation paths of comparator.hpp: the C++11+ primary path and
* the C++98 fallback. The traits/concepts tests are inherently C++11+/C++20
* and guard their bodies accordingly.
*
* path:      /test/functional/comparator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      1.  D_INTERNAL_CMP_CHECK            (early-return assertion)
      2.  record / get_rank / get_id      (key-based fixtures)
      3.  rank_of                          (free key function)
      4.  asc_int / desc_int               (raw binary comparators)
II.   PER-SECTION TEST DECLARATIONS
      1.  primitive       (natural / by_key / by_member / by_function)
      2.  combinators     (reversed / then / lifted)
      3.  pipeline        (operator| with then_adapter / reversed_adapter)
      4.  predicates      (equal_under / less_than / greater_than)
      5.  traits          (is_comparator / is_*_predicate / concepts)
III.  SECTION RUNNERS + TOP-LEVEL AGGREGATE
*/

#ifndef DJINTERP_TEST_FUNCTIONAL_COMPARATOR_
#define DJINTERP_TEST_FUNCTIONAL_COMPARATOR_ 1

// djinterp
#include "../../inc/functional/comparator.hpp"


///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                              ///
///////////////////////////////////////////////////////////////////////////////

// D_INTERNAL_CMP_CHECK
//   macro: returns false from the enclosing test the moment a condition
// fails. Wrapped in a do/while so it is a single statement usable without
// surrounding braces.
#define D_INTERNAL_CMP_CHECK(_cond)                                           \
    do                                                                        \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


NS_DJINTERP
NS_TESTING


// record
//   struct: minimal fixture carrying two orderable fields. Used to exercise
// key-based comparators, tie-breaker chains, and lifts. `rank` is the
// primary ordering field; `id` is the secondary (tie-break) field.
struct record
{
    int rank;
    int id;
};


// get_rank
//   struct: key functor extracting record::rank.
struct get_rank
{
    int operator()(
        const record& _r
    ) const
    {
        return _r.rank;
    }
};


// get_id
//   struct: key functor extracting record::id.
struct get_id
{
    int operator()(
        const record& _r
    ) const
    {
        return _r.id;
    }
};


// rank_of
//   function: free-function key extractor, used to confirm by_key accepts a
// plain function (pointer) as readily as a functor.
inline int
rank_of(
    const record& _r
)
{
    return _r.rank;
}


// asc_int
//   struct: raw ascending binary comparator (a < b). Stands in for a
// user-supplied comparator-shaped callable handed to by_function / the
// pipeline operators / the traits.
struct asc_int
{
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return (_a < _b);
    }
};


// desc_int
//   struct: raw descending binary comparator (b < a). Used to verify
// by_function preserves an arbitrary ordering and to contrast with the
// natural ordering.
struct desc_int
{
    bool operator()(
        int _a,
        int _b
    ) const
    {
        return (_b < _a);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

// 1.  primitive
//////////////////////////////////////////
bool test_primitive_natural_orders();
bool test_primitive_natural_strictness();
bool test_primitive_by_key_functor();
bool test_primitive_by_key_function_pointer();
bool test_primitive_by_member_primary();
bool test_primitive_by_member_secondary();
bool test_primitive_by_function();
bool test_primitive_by_function_equiv();

// 2.  combinators
//////////////////////////////////////////
bool test_combinators_reversed();
bool test_combinators_reversed_strictness();
bool test_combinators_then_tiebreak();
bool test_combinators_then_primary_wins();
bool test_combinators_then_nested();
bool test_combinators_lifted();
bool test_combinators_lifted_reversed();

// 3.  pipeline
//////////////////////////////////////////
bool test_pipeline_then_adapter();
bool test_pipeline_reversed_adapter();
bool test_pipeline_reversed_adapter_equiv();
bool test_pipeline_combined_adapters();
bool test_pipeline_raw_callable_lhs();

// 4.  predicates
//////////////////////////////////////////
bool test_predicates_equal_under();
bool test_predicates_equal_under_by_key();
bool test_predicates_less_than();
bool test_predicates_greater_than();
bool test_predicates_boundary();
bool test_predicates_with_reversed();

// 5.  traits
//////////////////////////////////////////
bool test_traits_is_comparator();
bool test_traits_is_binary_predicate();
bool test_traits_is_unary_predicate();
bool test_traits_has_result_type();
bool test_traits_value_aliases();
bool test_traits_concepts();


///////////////////////////////////////////////////////////////////////////////
///                III. SECTION RUNNERS + TOP-LEVEL AGGREGATE                ///
///////////////////////////////////////////////////////////////////////////////

bool run_primitive_tests();
bool run_combinators_tests();
bool run_pipeline_tests();
bool run_predicates_tests();
bool run_traits_tests();


// run_all_comparator_tests
//   function: drives every section runner. Returns true only when all
// sections pass. Inlined here so the entry point is header-resident and the
// .cpp files stay purely sectional.
inline bool
run_all_comparator_tests()
{
    return ( run_primitive_tests()   &&
             run_combinators_tests() &&
             run_pipeline_tests()    &&
             run_predicates_tests()  &&
             run_traits_tests() );
}


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTIONAL_COMPARATOR_
