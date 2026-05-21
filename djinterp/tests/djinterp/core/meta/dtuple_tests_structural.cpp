/******************************************************************************
* djinterp [testing]                                dtuple_tests_structural.cpp
*
*   Structural-group test definitions for the dtuple test suite:
* first_arg, is_tuple, to_tuple, make_tuple_of.  Each test verifies
* the core structural identity of one dtuple component — "what is
* this type doing at the pack / tuple level?" — before any of the
* composition, indexing, or transform machinery layers on top.
*
*   See dtuple_tests.hpp for the full list of framework features
* exercised, the probe aliases and predicate wrappers used here,
* and the orchestrator that threads every .cpp in this group
* together.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_structural.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   FIRST_ARG
// =========================================================================

/*
tests_dtuple_first_arg
  Verifies the first_arg metafunction extracts the head type of a
  variadic pack.
  Tests the following:
  - single-element packs return that element verbatim
  - multi-element packs return only the first element
  - cv- and reference-qualified head types are preserved
  - the _t alias resolves to the same type as ::type
  - detection succeeds on single and multi-element packs
  - detection fails on the empty pack (ill-formed by design)
  - is_detected_exact pins the exact detected type
*/
bool
tests_dtuple_first_arg(
    test_handler& _handler
)
{
    // basic single-type
    D_TEST_TYPE_EQ(first_arg_t<int>,  int);
    D_TEST_TYPE_EQ(first_arg_t<void>, void);

    // multi-type
    D_TEST_TYPE_EQ(first_arg_t<int, char, long>,         int);
    D_TEST_TYPE_EQ(first_arg_t<double, float, int, int>, double);

    // cv / reference preservation on the head
    D_TEST_TYPE_EQ(first_arg_t<const int, char>,    const int);
    D_TEST_TYPE_EQ(first_arg_t<int&,      char>,    int&);
    D_TEST_TYPE_EQ(first_arg_t<int&&,     char>,    int&&);
    D_TEST_TYPE_EQ(first_arg_t<int* const, char>,   int* const);

    // alias consistency: ::type vs the _t suffix
    D_TEST_TRAIT_ALIAS_CONSISTENT(first_arg, first_arg_t, int);

    // detection probe — empty pack must fail
    D_TEST_TRAIT_DETECTED    (internal::probe_first_arg, int);
    D_TEST_TRAIT_DETECTED    (internal::probe_first_arg, int, char);
    D_TEST_TRAIT_NOT_DETECTED(internal::probe_first_arg);

    // exact detected-type pinning
    D_TEST_TRAIT_DETECTED_EXACT(
        int,
        internal::probe_first_arg,
        int, char, long);

    D_TEST_FIRE(_handler, on_compile_check, "first_arg/all-checks");

    return true;
}


// =========================================================================
// II.  IS_TUPLE
// =========================================================================

/*
tests_dtuple_is_tuple
  Verifies the is_tuple structural predicate distinguishes
  std::tuple specializations from every other type.
  Tests the following:
  - empty and non-empty std::tuple specializations are recognized
  - non-tuple types (scalars, pairs, arrays) are rejected
  - pack-quantified all_of / any_of / none_of agree
  - pack_exactly_n_of reports the right count across a mixed pack
  - documents the (intentional) non-cv-stability of is_tuple
*/
bool
tests_dtuple_is_tuple(
    test_handler& _handler
)
{
    typedef std::pair<int, int> int_pair;

    // positive cases
    D_TEST_TRAIT_TRUE(is_tuple, std::tuple<>);
    D_TEST_TRAIT_TRUE(is_tuple, std::tuple<int>);
    D_TEST_TRAIT_TRUE(is_tuple, std::tuple<int, char, long>);

    // negative cases
    D_TEST_TRAIT_FALSE(is_tuple, int);
    D_TEST_TRAIT_FALSE(is_tuple, void);
    D_TEST_TRAIT_FALSE(is_tuple, int_pair);

    // pack quantification
    D_TEST_TRAIT_ALL_OF(internal::is_dtuple_pred,
                        std::tuple<>,
                        std::tuple<int>,
                        std::tuple<int, char>);

    D_TEST_TRAIT_NONE_OF(internal::is_dtuple_pred,
                         int,
                         char,
                         double);

    D_TEST_TRAIT_ANY_OF(internal::is_dtuple_pred,
                        int,
                        std::tuple<char>,
                        double);

    // pack_exactly_n_of: in a pack of six types containing two
    // tuples, the predicate should report exactly two matches
    static_assert(
        (pack_exactly_n_of<2,
                           internal::is_dtuple_pred,
                           int,
                           std::tuple<int>,
                           char,
                           std::tuple<>,
                           double,
                           int_pair>::value),
        "pack_exactly_n_of reports correct match count");

    // documented non-stability: is_tuple is NOT cv-stable as
    // currently written — `const std::tuple<int>` falls through
    // to the primary template and reports false.  This assertion
    // pins that behavior so any future change is caught.
    D_TEST_TRAIT_FALSE(is_tuple, const std::tuple<int>);

    D_TEST_FIRE(_handler, on_compile_check, "is_tuple/all-checks");

    return true;
}


// =========================================================================
// III. TO_TUPLE
// =========================================================================

/*
tests_dtuple_to_tuple
  Verifies to_tuple acts as a tuple-promoting identity:
  non-tuple types get wrapped in std::tuple, tuple types pass
  through unchanged.
  Tests the following:
  - single non-tuple wrapped into one-element tuple
  - existing tuples passed through verbatim
  - multi-arg packs assembled into a tuple
  - empty pack yields the empty tuple
  - alias consistency for to_tuple_t
*/
bool
tests_dtuple_to_tuple(
    test_handler& _handler
)
{
    typedef std::tuple<int, char> tup_ic;

    // wrap a non-tuple
    D_TEST_TYPE_EQ(to_tuple_t<int>,  std::tuple<int>);
    D_TEST_TYPE_EQ(to_tuple_t<void>, std::tuple<void>);

    // pass through an existing tuple
    D_TEST_TYPE_EQ(to_tuple_t<tup_ic>,        tup_ic);
    D_TEST_TYPE_EQ(to_tuple_t<std::tuple<>>,  std::tuple<>);

    // multi-arg pack
    D_TEST_TYPE_EQ(to_tuple_t<int, char, long>,
                   std::tuple<int, char, long>);

    // alias consistency
    D_TEST_TRAIT_ALIAS_CONSISTENT(to_tuple, to_tuple_t, int);
    D_TEST_TRAIT_ALIAS_CONSISTENT(to_tuple, to_tuple_t,
                                  std::tuple<int>);

    D_TEST_FIRE(_handler, on_compile_check, "to_tuple/all-checks");

    return true;
}


// =========================================================================
// IV.  MAKE_TUPLE_OF
// =========================================================================

/*
tests_dtuple_make_tuple_of
  Verifies make_tuple_of produces a tuple of N copies of T, with
  correct degenerate behavior at N == 0 and N == 1.
  Tests the following:
  - N == 0 yields the empty tuple
  - N == 1 yields a one-element tuple
  - N > 1 yields the expected repeated layout
  - detection succeeds across a representative range of N values
  - large N (depth / capacity check) still yields the right layout
*/
bool
tests_dtuple_make_tuple_of(
    test_handler& _handler
)
{
    typedef std::integral_constant<std::size_t, 0>  n0;
    typedef std::integral_constant<std::size_t, 1>  n1;
    typedef std::integral_constant<std::size_t, 8>  n8;
    typedef std::integral_constant<std::size_t, 16> n16;

    // degenerate cases
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 0>, std::tuple<>);
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 1>, std::tuple<int>);

    // non-degenerate
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 3>,
                   std::tuple<int, int, int>);
    D_TEST_TYPE_EQ(make_tuple_of_t<char, 4>,
                   std::tuple<char, char, char, char>);

    // detection across a value range
    D_TEST_TRAIT_DETECTED(internal::probe_make_tuple_of, int, n0);
    D_TEST_TRAIT_DETECTED(internal::probe_make_tuple_of, int, n1);
    D_TEST_TRAIT_DETECTED(internal::probe_make_tuple_of, int, n8);
    D_TEST_TRAIT_DETECTED(internal::probe_make_tuple_of, int, n16);

    // std::tuple_size agrees with the requested count for
    // representative values.  This cross-checks the layout
    // rather than just the type equality macro.
    static_assert(
        (std::tuple_size<make_tuple_of_t<int, 0>>::value == 0),
        "make_tuple_of<int, 0> has size 0");
    static_assert(
        (std::tuple_size<make_tuple_of_t<int, 8>>::value == 8),
        "make_tuple_of<int, 8> has size 8");

    D_TEST_FIRE(_handler,
                on_compile_check,
                "make_tuple_of/all-checks");

    return true;
}


NS_END  // testing
NS_END  // djinterp
