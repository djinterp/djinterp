/******************************************************************************
* djinterp [functional]                                   comparator_tests.hpp
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
*   SPEC PROVIDER (DTEST_SPEC_MODE):
*   Defining DTEST_SPEC_MODE before including this header additionally pulls in
* the DTest authoring surface (test_defaults.hpp) and exposes comparator_spec()
* (section IV) - the five sections as a module_spec: one block per section, one
* test_spec per predicate, each stamped with a name and the one-line descriptor
* lifted from its section .cpp. run_module() lowers that spec into the six-kind
* tree and projects it onto the report / PDF. That surface, and the enriched
* runner in .../functional/comparator/ that consumes it, are C++11+ (the DTest
* floor), so BOTH are gated behind DTEST_SPEC_MODE and leave the C++98-subset
* section builds - which never define it - untouched: the C++98 fallback path
* is still exercised by compiling the section tests under C++98 and driving
* them through run_all_comparator_tests(). The section .cpp files, built
* without DTEST_SPEC_MODE, supply the predicate definitions the spec references
* by address.
*
* path:      /tests/djinterp/core/functional/comparator_tests.hpp
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
IV.   SPEC PROVIDER                       (DTEST_SPEC_MODE: comparator_spec)
*/

#ifndef DJINTERP_TESTING_FUNCTIONAL_COMPARATOR_
#define DJINTERP_TESTING_FUNCTIONAL_COMPARATOR_ 1

// djinterp
#include "../../inc/functional/comparator.hpp"

#ifdef DTEST_SPEC_MODE
    // The enriched runner defines DTEST_SPEC_MODE and consumes comparator_spec()
    // (section IV). Pull in the DTest authoring surface: module_spec /
    // block_spec / test_spec, build_enriched_tree, run_module, and the
    // test_option_set the runner configures. Resolved via the djinterp include
    // root, the same root the runners' <djinterp/core/djinterp.hpp> resolves
    // against. This is the one C++11+ dependency in the file; gating it here
    // keeps the C++98-subset section builds - which never define
    // DTEST_SPEC_MODE - free of the framework, preserving the dual-standard
    // contract above.
#   include <djinterp/test/test_defaults.hpp>
#endif


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


///////////////////////////////////////////////////////////////////////////////
///                IV.  SPEC PROVIDER  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// comparator_spec
//   function: the suite as plain data for the enriched runner. Each of the
// five sections becomes one block_spec; each predicate declared in section II
// becomes one test_spec carrying a name (the predicate's identifier minus the
// test_ prefix) and the one-line descriptor lifted verbatim from its section
// .cpp. run_module() lowers this into the six-kind tree and drives the report
// / PDF from the same data, running each predicate exactly once for its leaf
// verdict.  C++11+ only, in keeping with the DTest floor - hence the
// DTEST_SPEC_MODE gate, which keeps the module's C++98 fallback build clean.
namespace dt = ::djinterp::test;

inline dt::module_spec
comparator_spec()
{
    return dt::module_spec{
        "comparator.hpp",
        "Primitive factories, combinators, the pipeline operator| adapters, "
        "comparator-derived predicates, and traits for comparator.hpp",
        {
            dt::block_spec{ "primitive",
                "natural / by_key / by_member / by_function",
                {
                    { "primitive_natural_orders",        "Verifies the natural (operator<) comparator over a built-in type.",       &test_primitive_natural_orders },
                    { "primitive_natural_strictness",    "Confirms the natural comparator is irreflexive.",                         &test_primitive_natural_strictness },
                    { "primitive_by_key_functor",        "Verifies by_key builds a comparator from a key-extracting functor.",      &test_primitive_by_key_functor },
                    { "primitive_by_key_function_pointer", "Confirms by_key accepts a plain function (pointer) as its key.",         &test_primitive_by_key_function_pointer },
                    { "primitive_by_member_primary",     "Verifies by_member builds a comparator from a pointer-to-data-member.",   &test_primitive_by_member_primary },
                    { "primitive_by_member_secondary",   "Confirms by_member targets the named member specifically (orders by id).", &test_primitive_by_member_secondary },
                    { "primitive_by_function",           "Verifies by_function wraps an arbitrary binary callable as a comparator.", &test_primitive_by_function },
                    { "primitive_by_function_equiv",     "Confirms by_function is a transparent pass-through.",                      &test_primitive_by_function_equiv }
                }
            },
            dt::block_spec{ "combinators",
                "reversed / then / lifted",
                {
                    { "combinators_reversed",            "Verifies reversed produces the inverse ordering of its inner comparator.", &test_combinators_reversed },
                    { "combinators_reversed_strictness", "Confirms reversed preserves strict-weak-ordering irreflexivity.",          &test_combinators_reversed_strictness },
                    { "combinators_then_tiebreak",       "Verifies then falls back to the secondary comparator on primary equivalence.", &test_combinators_then_tiebreak },
                    { "combinators_then_primary_wins",   "Confirms then never consults the secondary when the primary already decides.", &test_combinators_then_primary_wins },
                    { "combinators_then_nested",         "Verifies a three-level tie-breaker chain by nesting then.",                &test_combinators_then_nested },
                    { "combinators_lifted",              "Verifies lifted composes a comparator with a key function.",              &test_combinators_lifted },
                    { "combinators_lifted_reversed",     "Confirms lifted carries an arbitrary inner comparator, not just natural.", &test_combinators_lifted_reversed }
                }
            },
            dt::block_spec{ "pipeline",
                "operator| with then_adapter / reversed_adapter",
                {
                    { "pipeline_then_adapter",           "Verifies `primary | then(secondary)` builds the same tie-breaker chain.", &test_pipeline_then_adapter },
                    { "pipeline_reversed_adapter",       "Verifies `comparator | reversed()` wraps its left-hand side in reversed.", &test_pipeline_reversed_adapter },
                    { "pipeline_reversed_adapter_equiv", "Confirms `cmp | reversed()` is equivalent to reversed(cmp).",             &test_pipeline_reversed_adapter_equiv },
                    { "pipeline_combined_adapters",      "Verifies a pipeline mixing both adapters in one expression.",             &test_pipeline_combined_adapters },
                    { "pipeline_raw_callable_lhs",       "Confirms the pipeline operators accept a raw functor on the left.",       &test_pipeline_raw_callable_lhs }
                }
            },
            dt::block_spec{ "predicates",
                "equal_under / less_than / greater_than",
                {
                    { "predicates_equal_under",        "Verifies equal_under derives an equivalence predicate from a comparator.", &test_predicates_equal_under },
                    { "predicates_equal_under_by_key", "Confirms equal_under composes over a key-based comparator.",               &test_predicates_equal_under_by_key },
                    { "predicates_less_than",          "Verifies less_than binds the second operand of a comparator.",            &test_predicates_less_than },
                    { "predicates_greater_than",       "Verifies greater_than binds the first operand of a comparator.",          &test_predicates_greater_than },
                    { "predicates_boundary",           "Exercises the exact boundary of the one-sided binders.",                  &test_predicates_boundary },
                    { "predicates_with_reversed",      "Confirms the binders carry whatever ordering the comparator defines.",    &test_predicates_with_reversed }
                }
            },
            dt::block_spec{ "traits",
                "is_comparator / is_*_predicate / has_result_type / concepts",
                {
                    { "traits_is_comparator",       "Exercises the is_comparator trait at run time.",         &test_traits_is_comparator },
                    { "traits_is_binary_predicate", "Exercises the is_binary_predicate trait at run time.",    &test_traits_is_binary_predicate },
                    { "traits_is_unary_predicate",  "Exercises the is_unary_predicate trait at run time.",     &test_traits_is_unary_predicate },
                    { "traits_has_result_type",     "Exercises the has_result_type structural hint.",         &test_traits_has_result_type },
                    { "traits_value_aliases",       "Exercises the C++14 *_v variable-template shorthands.",   &test_traits_value_aliases },
                    { "traits_concepts",            "Exercises the C++20 comparator concepts.",               &test_traits_concepts }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_COMPARATOR_
