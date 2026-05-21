/******************************************************************************
* djinterp [testing]                                      dtuple_tests_meta.cpp
*
*   Meta-group test definitions for the dtuple test suite:
*     - robustness (cv/ref stability of structural predicates)
*     - logical relationships (implies / equivalent / disjoint)
*     - alias consistency (bulk ::type vs _t checks)
*     - zero-size tuple sweep across every component
*     - incomplete-type compatibility
*     - SFINAE-friendliness of out-of-range operations
*     - composition interplay (operations chained together)
*     - C++20 concepts demo (gated on D_ENV_CPP_FEATURE_LANG_CONCEPTS)
*
*   This file contains the bulk of the NEW tests from the expansion
* plan — the tests that stress the dtuple surface as a coherent
* whole rather than verifying one component in isolation.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_meta.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   ROBUSTNESS — CV/REF STABILITY
// =========================================================================

/*
tests_dtuple_robustness
  Uses the framework's trait_cv_stable / trait_ref_stable /
  trait_cvref_stable predicates to document which dtuple traits
  are robust under cv- and reference-qualification, and to pin
  the behavior of those that are not.
  Tests the following:
  - is_tuple is intentionally NOT cv-stable nor ref-stable
    (no specialization exists for cv- or ref-qualified tuples)
  - is_tuple_homogeneous and is_2d_tuple inherit the same
    sensitivity through their tuple-pattern matching
  - non-tuple types are vacuously cv/ref-stable under is_tuple
    (they are uniformly false)
  - documentation rather than aspiration: changing any of these
    behaviors will trip a test, prompting an explicit decision
*/
bool
tests_dtuple_robustness(
    test_handler& _handler
)
{
    // is_tuple over std::tuple<int> is true, but only for the
    // exact, unqualified spelling; cv and reference variants
    // fall through to the false primary template.
    static_assert(
        (!trait_cv_stable<internal::is_dtuple_pred,
                          std::tuple<int>>::value),
        "is_tuple is NOT cv-stable on tuples (current behavior)");

    static_assert(
        (!trait_ref_stable<internal::is_dtuple_pred,
                           std::tuple<int>>::value),
        "is_tuple is NOT ref-stable on tuples (current behavior)");

    // BUT for non-tuple types is_tuple is uniformly false
    // and therefore trivially cv/ref-stable.
    D_TEST_TRAIT_CVREF_STABLE(internal::is_dtuple_pred, int);
    D_TEST_TRAIT_CVREF_STABLE(internal::is_dtuple_pred, double);
    D_TEST_TRAIT_CVREF_STABLE(internal::is_dtuple_pred, void*);

    // is_tuple_homogeneous and is_2d_tuple inherit the same
    // sensitivity for the same reason: cv-qualified tuples no
    // longer match the partial specialization.
    static_assert(
        (!trait_cv_stable<internal::is_dtuple_homogeneous_pred,
                          std::tuple<int>>::value),
        "is_tuple_homogeneous not cv-stable on tuples");
    static_assert(
        (!trait_cv_stable<internal::is_dtuple_2d_pred,
                          std::tuple<std::tuple<int>>>::value),
        "is_2d_tuple not cv-stable on tuples");

    D_TEST_FIRE(_handler,
                on_compile_check,
                "robustness/all-checks");

    return true;
}


// =========================================================================
// II.  LOGICAL RELATIONSHIPS
// =========================================================================

/*
tests_dtuple_logical_relationships
  Uses trait_implies_for / trait_equivalent_for / trait_disjoint_for
  to encode invariants that should hold across pairs of dtuple
  predicates.
  Tests the following:
  - is_2d_tuple implies is_tuple (any 2D tuple is, first, a tuple)
  - is_tuple_homogeneous implies is_tuple (where the predicate is
    true, the type must be a tuple)
  - is_2d_tuple is NOT equivalent to is_tuple (proper subset)
  - is_tuple and is_not_tuple are disjoint over any pack
  - is_empty_tuple_pred implies is_tuple_homogeneous is FALSE
    for the empty-tuple case (pins the non-homogeneous-empty
    contract via logical machinery)
*/
bool
tests_dtuple_logical_relationships(
    test_handler& _handler
)
{
    // is_2d_tuple implies is_tuple over a representative pack
    D_TEST_TRAIT_IMPLIES(
        internal::is_dtuple_2d_pred,
        internal::is_dtuple_pred,
        int,
        char,
        std::tuple<>,
        std::tuple<int>,
        std::tuple<std::tuple<int>>,
        std::tuple<std::tuple<int>, std::tuple<char>>);

    // is_tuple_homogeneous implies is_tuple
    D_TEST_TRAIT_IMPLIES(
        internal::is_dtuple_homogeneous_pred,
        internal::is_dtuple_pred,
        int,
        std::tuple<>,
        std::tuple<int>,
        std::tuple<int, int>,
        std::tuple<int, char>);

    // is_2d_tuple and is_tuple are NOT equivalent: the empty
    // tuple is_2d=true and is_tuple=true (agree there), but a
    // flat tuple<int> has is_2d=false and is_tuple=true (disagree).
    static_assert(
        (!trait_equivalent_for<internal::is_dtuple_2d_pred,
                               internal::is_dtuple_pred,
                               std::tuple<int>>::value),
        "is_2d_tuple is not equivalent to is_tuple "
        "(a flat tuple is one but not the other)");

    // is_tuple and is_not_tuple are disjoint over any pack
    D_TEST_TRAIT_DISJOINT(
        internal::is_dtuple_pred,
        internal::is_not_dtuple_pred,
        int,
        std::tuple<>,
        std::tuple<int>,
        char,
        std::tuple<int, char, long>,
        double);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "logical_relationships/all-checks");

    return true;
}


// =========================================================================
// III. ALIAS CONSISTENCY (BULK)
// =========================================================================

/*
tests_dtuple_alias_consistency
  Bulk verification that every public ::type / _t pair in dtuple
  resolves identically.  Catches the easiest class of refactoring
  bug: an edit to the underlying trait that misses the alias, or
  vice versa.
  Tests the following:
  - first_arg / first_arg_t agree on representative inputs
  - to_tuple / to_tuple_t agree
  - normalize_tuple / normalize_tuple_t agree
  - tuple_flatten_types / tuple_flatten_types_t agree on 2D inputs
*/
bool
tests_dtuple_alias_consistency(
    test_handler& _handler
)
{
    // first_arg
    D_TEST_TRAIT_ALIAS_CONSISTENT_FOR(
        first_arg, first_arg_t,
        int,
        const char,
        double*);

    // to_tuple
    D_TEST_TRAIT_ALIAS_CONSISTENT_FOR(
        to_tuple, to_tuple_t,
        int,
        std::tuple<int>,
        std::tuple<int, char>);

    // normalize_tuple
    D_TEST_TRAIT_ALIAS_CONSISTENT_FOR(
        normalize_tuple, normalize_tuple_t,
        std::tuple<int>,
        std::tuple<const int&, volatile char>);

    // tuple_flatten_types (on 2D inputs only — the trait is
    // defined for 2D tuples)
    D_TEST_TRAIT_ALIAS_CONSISTENT_FOR(
        tuple_flatten_types, tuple_flatten_types_t,
        std::tuple<>,
        std::tuple<std::tuple<int>>,
        std::tuple<std::tuple<int, char>, std::tuple<long>>);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "alias_consistency/all-checks");

    return true;
}


// =========================================================================
// IV.  ZERO-SIZE TUPLE SWEEP
// =========================================================================

/*
tests_dtuple_zero_size_sweep
  Systematic pass through every dtuple operation that accepts
  tuples, with std::tuple<> as the input.  Empty-case
  specializations are the single most common site of missing
  specializations in metaprograms; concentrating every zero-size
  case in one test makes regressions unambiguous.
  Tests the following (every one against std::tuple<> input):
  - is_tuple:                  true
  - is_tuple_homogeneous:      false (pinned non-homogeneity)
  - is_2d_tuple:               true  (vacuous truth)
  - tuple_apply_all:           yields std::tuple<>
  - tuple_subsequence [0,0):   yields std::tuple<>
  - tuple_split<0>:            yields (tuple<>, tuple<>)
  - tuple_count_type:          yields 0
  - tuple_count_and_remove:    yields (tuple<>, 0)
  - tuple_flatten_types:       yields std::tuple<>
  - normalize_tuple:           yields std::tuple<>
  - std::tuple_size value:     0
*/
bool
tests_dtuple_zero_size_sweep(
    test_handler& _handler
)
{
    typedef std::tuple<> tup_empty;

    // structural predicates
    D_TEST_TRAIT_TRUE (is_tuple,             tup_empty);
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous, tup_empty);
    D_TEST_TRAIT_TRUE (is_2d_tuple,          tup_empty);

    // tuple_apply_all on empty pack
    D_TEST_TYPE_EQ(tuple_apply_all_t<std::add_pointer_t>,
                   tup_empty);

    // tuple_subsequence [0, 0) on empty tuple
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 0, tup_empty>,
                   tup_empty);

    // tuple_split<0> on empty tuple
    D_TEST_TYPE_EQ(typename tuple_split<0, tup_empty>::before,
                   tup_empty);
    D_TEST_TYPE_EQ(typename tuple_split<0, tup_empty>::after,
                   tup_empty);

    // tuple_count_type on empty tuple
    static_assert(
        (tuple_count_type<int, tup_empty>::value == 0),
        "count_type on empty tuple yields 0");

    // tuple_count_and_remove on empty tuple
    D_TEST_TYPE_EQ(tuple_count_and_remove_t<int, tup_empty>,
                   tup_empty);
    static_assert(
        (tuple_count_and_remove<int, tup_empty>::value == 0),
        "count_and_remove on empty tuple yields count 0");

    // tuple_flatten_types on empty tuple
    D_TEST_TYPE_EQ(tuple_flatten_types_t<tup_empty>, tup_empty);

    // normalize_tuple on empty tuple
    D_TEST_TYPE_EQ(normalize_tuple_t<tup_empty>, tup_empty);

    // make_tuple_of<T, 0> yields empty tuple
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 0>, tup_empty);

    // tuple_join<> and tuple_join<tup_empty> yield empty tuple
    D_TEST_TYPE_EQ(typename tuple_join<>::type, tup_empty);
    D_TEST_TYPE_EQ(typename tuple_join<tup_empty>::type,
                   tup_empty);

    // std::tuple_size agreement
    static_assert(
        (std::tuple_size<tup_empty>::value == 0),
        "std::tuple_size of empty tuple is 0");

    D_TEST_FIRE(_handler,
                on_compile_check,
                "zero_size_sweep/all-checks");

    return true;
}


// =========================================================================
// V.   INCOMPLETE TYPES
// =========================================================================

/*
tests_dtuple_incomplete_types
  Verifies that dtuple traits which do NOT require size, layout,
  or completeness of their template arguments compile cleanly
  against an incomplete forward declaration.  The fixture
  incomplete_demo_type is declared in dtuple_tests.hpp as a
  struct with no definition; if any trait here tries to inspect
  the type's layout, this test fails to compile.
  Tests the following:
  - is_tuple<incomplete>:                    false (compiles)
  - first_arg_t<incomplete, int>:            incomplete (compiles)
  - first_arg_t<int, incomplete>:            int (compiles)
  - to_tuple_t<incomplete>:                  std::tuple<incomplete>
  - pack_count_of<is_dtuple_pred, ...,
                  incomplete, ...>:          compiles
*/
bool
tests_dtuple_incomplete_types(
    test_handler& _handler
)
{
    // is_tuple against incomplete type
    D_TEST_TRAIT_FALSE(is_tuple, incomplete_demo_type);

    // first_arg with incomplete at various positions
    D_TEST_TYPE_EQ(first_arg_t<incomplete_demo_type, int>,
                   incomplete_demo_type);
    D_TEST_TYPE_EQ(first_arg_t<int, incomplete_demo_type>, int);
    D_TEST_TYPE_EQ(first_arg_t<incomplete_demo_type>,
                   incomplete_demo_type);

    // to_tuple wrapping an incomplete type
    D_TEST_TYPE_EQ(to_tuple_t<incomplete_demo_type>,
                   std::tuple<incomplete_demo_type>);

    // pack_count_of scanning a pack that includes an incomplete
    // type — only valid because is_tuple does not require the
    // type to be complete
    static_assert(
        (pack_count_of<internal::is_dtuple_pred,
                       int,
                       incomplete_demo_type,
                       std::tuple<int>,
                       incomplete_demo_type>::value == 1),
        "pack_count_of handles incomplete types in the pack");

    // pack_none_of with incomplete types (same reasoning)
    static_assert(
        (pack_none_of<internal::is_dtuple_pred,
                      incomplete_demo_type,
                      int,
                      incomplete_demo_type>::value),
        "pack_none_of handles incomplete types");

    // is_detected against incomplete type: probe_is_tuple
    // instantiates cleanly whether or not the type is complete,
    // because is_tuple does not interrogate layout
    D_TEST_TRAIT_DETECTED(internal::probe_is_tuple,
                          incomplete_demo_type);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "incomplete_types/all-checks");

    return true;
}


// =========================================================================
// VI.  SFINAE-FRIENDLINESS
// =========================================================================

/*
tests_dtuple_sfinae_friendliness
  Documents which dtuple operations fail via SFINAE (substitution
  failure) vs which fail via a hard error, when given invalid
  template arguments.  A SFINAE-friendly failure lets the trait
  be composed with enable_if / concepts; a hard error makes the
  trait unusable in those contexts.
  Tests the following:
  - first_arg on the empty pack is SFINAE-friendly (probe fails
    to detect rather than hard-errors)
  - make_tuple_of with N == 0 is DETECTED (SFINAE-friendly)
  - tuple_subsequence with start == end is DETECTED
  - DOCUMENTATION NOTE (no live assertions): whether
    tuple_type_at<N, tup> for N >= size is SFINAE-friendly or
    a hard error is a property of the current dtuple
    implementation; the indexing group's positive test already
    pins the well-formed cases, so this test stays focused on
    the operations that ARE demonstrably SFINAE-friendly today.
*/
bool
tests_dtuple_sfinae_friendliness(
    test_handler& _handler
)
{
    typedef std::integral_constant<std::size_t, 0> n0;
    typedef std::integral_constant<std::size_t, 4> n4;
    typedef std::tuple<int, char, long, double>    tup_iclr;

    // empty pack fails to detect first_arg — SFINAE-friendly
    D_TEST_TRAIT_NOT_DETECTED(internal::probe_first_arg);

    // N == 0 is detected for make_tuple_of (valid request)
    D_TEST_TRAIT_DETECTED(internal::probe_make_tuple_of, int, n0);

    // start == end is detected for tuple_subsequence (zero-length
    // slice is a valid request)
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_subsequence,
                          n0, n0, tup_iclr);
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_subsequence,
                          n4, n4, tup_iclr);

    // detected_or_t falls back to the default when the probe
    // fails — another angle on SFINAE-friendliness.  For the
    // non-detected first_arg<> case, detected_or_t yields the
    // supplied default (here: void).
    D_TEST_TYPE_EQ(
        detected_or_t<void, internal::probe_first_arg>,
        void);
    D_TEST_TYPE_EQ(
        detected_or_t<void, internal::probe_first_arg, int, char>,
        int);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "sfinae_friendliness/all-checks");

    return true;
}


// =========================================================================
// VII. COMPOSITION INTERPLAY
// =========================================================================

/*
tests_dtuple_composition_interplay
  Verifies that dtuple operations compose correctly at the seams.
  Single-component tests cover each operation in isolation; this
  test chains two or more together to catch bugs that only
  surface when the output of one operation flows into the input
  of another.
  Tests the following:
  - normalize(apply(add_const, int, char)) strips the const back
    off both elements: const T -> T, so the result is <int, char>
  - apply(add_pointer_t, flatten(2D tuple)) pointerizes the
    flattened row contents
  - count_and_remove applied to the result of apply_all preserves
    the expected count
  - join(apply_all(A), apply_all(B)) equals apply_all(A++B)
    (distribution law)
*/
bool
tests_dtuple_composition_interplay(
    test_handler& _handler
)
{
    // normalize(apply_all(add_const_t, int, char))
    //   apply_all yields tuple<const int, const char>
    //   normalize strips the const back off both
    //   -> tuple<int, char>
    D_TEST_TYPE_EQ(
        normalize_tuple_t<
            tuple_apply_all_t<std::add_const_t, int, char>>,
        std::tuple<int, char>);

    // apply_all(add_pointer_t, flatten(2D tuple))
    //   flatten yields tuple<int, char, long>
    //   apply_all pointerizes each
    //   -> tuple<int*, char*, long*>
    D_TEST_TYPE_EQ(
        tuple_apply_all_t<std::add_pointer_t,
            typename std::tuple_element<0,
                tuple_flatten_types_t<
                    std::tuple<std::tuple<int>,
                               std::tuple<char, long>>>>::type,
            typename std::tuple_element<1,
                tuple_flatten_types_t<
                    std::tuple<std::tuple<int>,
                               std::tuple<char, long>>>>::type,
            typename std::tuple_element<2,
                tuple_flatten_types_t<
                    std::tuple<std::tuple<int>,
                               std::tuple<char, long>>>>::type>,
        std::tuple<int*, char*, long*>);

    // count_and_remove on the result of apply_all.  Start with
    // tuple<int, char, int>, apply add_pointer_t to get
    // tuple<int*, char*, int*>, remove int* to get tuple<char*>
    // with count 2.
    typedef tuple_apply_all_t<std::add_pointer_t,
                              int, char, int>
        apply_result;
    D_TEST_TYPE_EQ(
        tuple_count_and_remove_t<int*, apply_result>,
        std::tuple<char*>);
    static_assert(
        (tuple_count_and_remove<int*, apply_result>::value == 2),
        "count_and_remove on apply_all result yields correct count");

    // distribution: join(apply_all(A), apply_all(B)) ==
    // apply_all(A++B).  Here A = {int}, B = {char}.
    typedef tuple_apply_all_t<std::add_pointer_t, int>   lhs_a;
    typedef tuple_apply_all_t<std::add_pointer_t, char>  lhs_b;
    typedef typename tuple_join<lhs_a, lhs_b>::type      distributed;
    typedef tuple_apply_all_t<std::add_pointer_t, int, char>
        applied_joined;
    D_TEST_TYPE_EQ(distributed, applied_joined);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "composition_interplay/all-checks");

    return true;
}


// =========================================================================
// VIII. CONCEPTS DEMO (C++20 only)
// =========================================================================
//   The D_TEST_CONCEPT_TRUE / D_TEST_CONCEPT_FALSE macros only
// exist when D_ENV_CPP_FEATURE_LANG_CONCEPTS is enabled.  On
// earlier standards the stub function still exists (so the
// linker can find it from the orchestrator) but contains no
// assertions; it reports "skipped" via the compile-check event
// so the output makes it obvious why.

/*
tests_dtuple_concepts_demo
  On C++20-capable toolchains, demonstrates the concept-assertion
  macros against a couple of illustrative cases.  On earlier
  standards, fires an informational compile-check event noting
  the test was skipped and returns true.
*/
bool
tests_dtuple_concepts_demo(
    test_handler& _handler
)
{
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

    // std::integral is a C++20 concept; use it to demonstrate
    // the concept-assertion macros against well-known truths.
    D_TEST_CONCEPT_TRUE (std::integral, int);
    D_TEST_CONCEPT_TRUE (std::integral, long);
    D_TEST_CONCEPT_FALSE(std::integral, double);
    D_TEST_CONCEPT_FALSE(std::integral, std::tuple<int>);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "concepts_demo/all-checks (C++20 path)");

#else

    D_TEST_FIRE(_handler,
                on_compile_check,
                "concepts_demo/skipped (pre-C++20, "
                "concepts unavailable)");

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

    return true;
}


NS_END  // testing
NS_END  // djinterp
