/******************************************************************************
* djinterp [functional]                                     extractor_tests.hpp
*
* Unit-test declarations for extractor.hpp.
*   This header declares the full battery of extractor tests and the helpers
* shared between the per-section translation units. Each semantic section of
* extractor.hpp maps to one .cpp file and one section runner declared here:
*
*   extractor_tests_primitive.cpp     -> run_primitive_tests
*   extractor_tests_combinators.cpp   -> run_combinators_tests
*   extractor_tests_pipeline.cpp      -> run_pipeline_tests
*   extractor_tests_drivers.cpp       -> run_drivers_tests
*   extractor_tests_traits.cpp        -> run_traits_tests
*
*   Every test is a parameterless predicate returning true on success and
* false on the first failed check. Tests live in djinterp::testing; the
* extractor factories under test are reached through djinterp::extractors,
* and maybe / just / nothing are flat djinterp types (no functional
* namespace).
*
*   FIXTURES use named functor types rather than lambdas throughout. This is
* deliberate: the traits section takes decltype of factory calls, and a
* lambda in an unevaluated context is only legal from C++20. Named functors
* keep the entire suite compiling from C++11 upward.
*
*   SPEC PROVIDER (DTEST_SPEC_MODE):
*   Defining DTEST_SPEC_MODE before including this header additionally pulls in
* the DTest authoring surface (test_defaults.hpp) and exposes extractor_spec()
* (section IV) - the five sections as a module_spec: one block per section, one
* test_spec per predicate, each stamped with a name and the one-line descriptor
* lifted from its section .cpp. run_module() lowers that spec into the six-kind
* tree and projects it onto the report / PDF, so the &&-folded section runners
* below (retained for a bare pass/fail answer) and the enriched spec are two
* views of one suite. The enriched runner in .../functional/extractor/ consumes
* the spec; the section .cpp files, built without DTEST_SPEC_MODE, supply the
* predicate definitions the spec references by address.
*
* path:      /test/functional/extractor_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      1.  D_INTERNAL_EXT_CHECK             (early-return assertion)
      2.  person / triple                  (source fixtures)
      3.  key functors                     (get_id / get_age / get_dept ...)
      4.  predicates / transforms          (is_adult / times_two / ...)
      5.  age_of                            (free key function)
      6.  checked_age                       (throwing extractor, for try)
      7.  make_people                       (sample container builder)
II.   PER-SECTION TEST DECLARATIONS
      1.  primitive   (identity/constant/from_function/from_member/from_index)
      2.  combinators (then_extract/fanout/mapped/filtered/guarded/defaulted/try)
      3.  pipeline    (operator| with the three adapters)
      4.  drivers     (extract_all/first/unique/into_map/group_by)
      5.  traits      (is_extractor/result_t/is_maybe/is_maybe_extractor/concepts)
III.  SECTION RUNNERS + TOP-LEVEL AGGREGATE
IV.   SPEC PROVIDER                        (DTEST_SPEC_MODE: extractor_spec)
*/

#ifndef DJINTERP_TEST_FUNCTIONAL_EXTRACTOR_
#define DJINTERP_TEST_FUNCTIONAL_EXTRACTOR_ 1

// std
#include <string>
#include <vector>
// djinterp
#include "../../inc/functional/extractor.hpp"

#ifdef DTEST_SPEC_MODE
    // The enriched runner defines DTEST_SPEC_MODE and consumes extractor_spec()
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

// D_INTERNAL_EXT_CHECK
//   macro: returns false from the enclosing test the moment a condition
// fails. Wrapped in a do/while so it is a single statement usable without
// surrounding braces.
#define D_INTERNAL_EXT_CHECK(_cond)                                           \
    do                                                                        \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


NS_DJINTERP
NS_TESTING


// person
//   struct: primary source fixture. Three int features so extractors,
// fan-out tuples, maps, and grouping can all be exercised without dragging
// in heavier value types.
struct person
{
    int id;
    int age;
    int dept;
};


// get_id / get_age / get_dept
//   structs: key functors extracting each person feature. Named (not
// lambdas) so decltype of a factory built from them is well-formed pre-C++20.
struct get_id
{
    int operator()(
        const person& _p
    ) const
    {
        return _p.id;
    }
};

struct get_age
{
    int operator()(
        const person& _p
    ) const
    {
        return _p.age;
    }
};

struct get_dept
{
    int operator()(
        const person& _p
    ) const
    {
        return _p.dept;
    }
};


// is_adult
//   struct: predicate on an extracted age (value-side gate for filtered).
struct is_adult
{
    bool operator()(
        int _age
    ) const
    {
        return (_age >= 18);
    }
};


// dept_nonzero
//   struct: predicate on a source person (source-side gate for guarded).
struct dept_nonzero
{
    bool operator()(
        const person& _p
    ) const
    {
        return (_p.dept != 0);
    }
};


// times_two
//   struct: post-transform on an extracted int (for mapped / then_extract).
struct times_two
{
    int operator()(
        int _v
    ) const
    {
        return (_v * 2);
    }
};


// age_of
//   function: free-function key extractor, used to confirm from_function
// accepts a plain function (pointer) as readily as a functor.
inline int
age_of(
    const person& _p
)
{
    return _p.age;
}


// checked_age
//   struct: an extractor that throws on a sentinel source. Used to exercise
// try_extract: a negative age raises, everything else returns the age.
struct checked_age
{
    int operator()(
        const person& _p
    ) const
    {
        if (_p.age < 0)
        {
            throw std::runtime_error("checked_age: negative");
        }

        return _p.age;
    }
};


// make_people
//   function: builds a small, deterministic sample container shared by the
// driver tests. Layout (id, age, dept):
//   {1, 30, 10}, {2, 15, 20}, {3, 30, 10}, {4, 40, 20}
// Notes: ages include a duplicate (30) for extract_unique; depts form two
// groups (10, 20) for group_by; ids are unique map keys.
inline std::vector<person>
make_people()
{
    std::vector<person> people;
    person a = { 1, 30, 10 };
    person b = { 2, 15, 20 };
    person c = { 3, 30, 10 };
    person d = { 4, 40, 20 };
    people.push_back(a);
    people.push_back(b);
    people.push_back(c);
    people.push_back(d);
    return people;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

// 1.  primitive
//////////////////////////////////////////
bool test_primitive_identity();
bool test_primitive_constant();
bool test_primitive_from_function_functor();
bool test_primitive_from_function_pointer();
bool test_primitive_from_member();
bool test_primitive_from_index();
bool test_primitive_constexpr();

// 2.  combinators
//////////////////////////////////////////
bool test_combinators_then_extract();
bool test_combinators_fanout2();
bool test_combinators_fanout3();
bool test_combinators_mapped();
bool test_combinators_filtered_pass();
bool test_combinators_filtered_fail();
bool test_combinators_guarded();
bool test_combinators_defaulted_present();
bool test_combinators_defaulted_absent();
bool test_combinators_try_extract_success();
bool test_combinators_try_extract_throws();

// 3.  pipeline
//////////////////////////////////////////
bool test_pipeline_then_extract_adapter();
bool test_pipeline_mapped_adapter();
bool test_pipeline_filtered_adapter();
bool test_pipeline_chained();
bool test_pipeline_equivalence();

// 4.  drivers
//////////////////////////////////////////
bool test_drivers_extract_all();
bool test_drivers_extract_all_empty();
bool test_drivers_extract_first();
bool test_drivers_extract_first_empty();
bool test_drivers_extract_unique();
bool test_drivers_extract_into_map();
bool test_drivers_extract_into_map_overwrite();
bool test_drivers_group_by_extractor();

// 5.  traits
//////////////////////////////////////////
bool test_traits_is_extractor();
bool test_traits_result_type();
bool test_traits_is_maybe();
bool test_traits_is_maybe_extractor();
bool test_traits_value_aliases();
bool test_traits_concepts();


///////////////////////////////////////////////////////////////////////////////
///                III. SECTION RUNNERS + TOP-LEVEL AGGREGATE                ///
///////////////////////////////////////////////////////////////////////////////

bool run_primitive_tests();
bool run_combinators_tests();
bool run_pipeline_tests();
bool run_drivers_tests();
bool run_traits_tests();


// run_all_extractor_tests
//   function: drives every section runner. Returns true only when all
// sections pass. Inlined here so the entry point is header-resident and the
// .cpp files stay purely sectional.
inline bool
run_all_extractor_tests()
{
    return ( run_primitive_tests()   &&
             run_combinators_tests() &&
             run_pipeline_tests()    &&
             run_drivers_tests()     &&
             run_traits_tests() );
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  SPEC PROVIDER  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#ifdef DTEST_SPEC_MODE

// extractor_spec
//   function: the suite as plain data for the enriched runner. Each of the
// five sections becomes one block_spec; each predicate declared in section II
// becomes one test_spec carrying a name (the predicate's identifier minus the
// test_ prefix) and the one-line descriptor lifted verbatim from its section
// .cpp. run_module() lowers this into the six-kind tree and drives the report
// / PDF from the same data, running each predicate exactly once for its leaf
// verdict.
namespace dt = ::djinterp::test;

inline dt::module_spec
extractor_spec()
{
    return dt::module_spec{
        "extractor.hpp",
        "Primitive extractors, the combinators, the operator| pipeline "
        "adapters, the container drivers, and structural traits for "
        "extractor.hpp",
        {
            dt::block_spec{ "primitive",
                "identity / constant / from_function / from_member / from_index",
                {
                    { "primitive_identity",              "Verifies identity returns its source unchanged.",                          &test_primitive_identity },
                    { "primitive_constant",              "Verifies constant ignores its source and always yields the stored value.", &test_primitive_constant },
                    { "primitive_from_function_functor", "Verifies from_function lifts a unary functor into an extractor.",          &test_primitive_from_function_functor },
                    { "primitive_from_function_pointer", "Confirms from_function accepts a plain free function (pointer).",           &test_primitive_from_function_pointer },
                    { "primitive_from_member",           "Verifies from_member reads a pointer-to-data-member.",                     &test_primitive_from_member },
                    { "primitive_from_index",            "Verifies from_index reads the N-th element of a tuple / pair via std::get.", &test_primitive_from_index },
                    { "primitive_constexpr",             "Confirms the primitive extractors are usable in constant expressions.",    &test_primitive_constexpr }
                }
            },
            dt::block_spec{ "combinators",
                "then_extract / fanout / mapped / filtered / guarded / defaulted / try_extract",
                {
                    { "combinators_then_extract",        "Verifies then_extract composes two extractors (outer applied to inner).",  &test_combinators_then_extract },
                    { "combinators_fanout2",             "Verifies binary fanout applies two extractors to one source and tuples them.", &test_combinators_fanout2 },
                    { "combinators_fanout3",             "Verifies ternary fanout tuples three extracted features.",                 &test_combinators_fanout3 },
                    { "combinators_mapped",              "Verifies mapped post-transforms an extractor's output.",                   &test_combinators_mapped },
                    { "combinators_filtered_pass",       "Verifies filtered yields just(value) when the value-side predicate passes.", &test_combinators_filtered_pass },
                    { "combinators_filtered_fail",       "Verifies filtered yields nothing when the value-side predicate fails.",    &test_combinators_filtered_fail },
                    { "combinators_guarded",             "Verifies guarded gates on the source before extraction (just / nothing).", &test_combinators_guarded },
                    { "combinators_defaulted_present",   "Verifies defaulted passes the inner maybe's value through when present.",  &test_combinators_defaulted_present },
                    { "combinators_defaulted_absent",    "Verifies defaulted substitutes the stored default when the inner is nothing.", &test_combinators_defaulted_absent },
                    { "combinators_try_extract_success", "Verifies try_extract wraps a non-throwing extraction as just(value).",     &test_combinators_try_extract_success },
                    { "combinators_try_extract_throws",  "Verifies try_extract captures an exception as nothing.",                   &test_combinators_try_extract_throws }
                }
            },
            dt::block_spec{ "pipeline",
                "operator| with the then_extract / mapped / filtered adapters",
                {
                    { "pipeline_then_extract_adapter", "Verifies `inner | then_extract(outer)` composes via the pipeline operator.", &test_pipeline_then_extract_adapter },
                    { "pipeline_mapped_adapter",       "Verifies `e | mapped(fn)` post-transforms via the pipeline operator.",       &test_pipeline_mapped_adapter },
                    { "pipeline_filtered_adapter",     "Verifies `e | filtered(p)` gates via the pipeline operator and yields maybe<T>.", &test_pipeline_filtered_adapter },
                    { "pipeline_chained",              "Verifies a multi-stage pipeline (mapped then filtered) respects stage order.", &test_pipeline_chained },
                    { "pipeline_equivalence",          "Confirms the pipeline forms are equivalent to the direct factory calls.",    &test_pipeline_equivalence }
                }
            },
            dt::block_spec{ "drivers",
                "extract_all / extract_first / extract_unique / extract_into_map / group_by_extractor",
                {
                    { "drivers_extract_all",              "Verifies extract_all applies an extractor to every element in order.",     &test_drivers_extract_all },
                    { "drivers_extract_all_empty",        "Verifies extract_all on an empty container yields an empty vector.",       &test_drivers_extract_all_empty },
                    { "drivers_extract_first",            "Verifies extract_first returns just(extract(front)) for a non-empty container.", &test_drivers_extract_first },
                    { "drivers_extract_first_empty",      "Verifies extract_first yields nothing for an empty container.",            &test_drivers_extract_first_empty },
                    { "drivers_extract_unique",           "Verifies extract_unique returns distinct extracted values in first-seen order.", &test_drivers_extract_unique },
                    { "drivers_extract_into_map",         "Verifies extract_into_map builds a map from a key and a value extractor.", &test_drivers_extract_into_map },
                    { "drivers_extract_into_map_overwrite", "Verifies later duplicates overwrite earlier map entries (last-write-wins).", &test_drivers_extract_into_map_overwrite },
                    { "drivers_group_by_extractor",       "Verifies group_by_extractor buckets sources by an extracted key, in order.", &test_drivers_group_by_extractor }
                }
            },
            dt::block_spec{ "traits",
                "is_extractor / extractor_result_t / is_maybe / is_maybe_extractor / concepts",
                {
                    { "traits_is_extractor",       "Exercises the is_extractor trait at run time.",          &test_traits_is_extractor },
                    { "traits_result_type",        "Exercises the extractor_result_t alias at run time.",     &test_traits_result_type },
                    { "traits_is_maybe",           "Exercises the is_maybe trait at run time.",              &test_traits_is_maybe },
                    { "traits_is_maybe_extractor", "Exercises the is_maybe_extractor trait at run time.",     &test_traits_is_maybe_extractor },
                    { "traits_value_aliases",      "Exercises the C++14 *_v variable-template shorthands.",   &test_traits_value_aliases },
                    { "traits_concepts",           "Exercises the C++20 extractor concepts.",                &test_traits_concepts }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTIONAL_EXTRACTOR_
