/******************************************************************************
* djinterp [testing]                                  dtuple_tests_indexing.cpp
*
*   Indexing-group test definitions for the dtuple test suite:
* tuple_type_at, tuple_subsequence, tuple_split, plus the
* reference-qualified round-trip and the instantiation-depth stress
* test.
*
*   The reference-qualifier round-trip is the high-value addition:
* metaprograms regularly drop reference qualifiers in the course of
* tuple manipulation, and the index-based operations are the most
* common site of that bug.  A tuple<int&, const char&&, volatile
* long*> is passed through every index-based op and the exact
* reference-qualified type is demanded back.
*
*   The instantiation-depth stress test pins the cost characteristic
* of tuple_subsequence: an index-sequence-based implementation is
* O(1) in template-instantiation depth, while a hand-rolled
* recursion is O(N).  The 32-element tuple is built two ways (via
* repeat_t, and hand-written) so the two representations are
* cross-checked against each other.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_indexing.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   TUPLE_TYPE_AT
// =========================================================================

/*
tests_dtuple_tuple_type_at
  Verifies tuple_type_at extracts the type at a given index from
  either a tuple argument or a raw pack.
  Tests the following:
  - first / middle / last index accesses
  - both pack and tuple input forms agree
  - dtuple's tuple_type_at agrees with std::tuple_element for the
    same indices
  - alias consistency for tuple_type_at_t
  - detection probe succeeds for in-range indices on a tuple
*/
bool
tests_dtuple_tuple_type_at(
    test_handler& _handler
)
{
    typedef std::tuple<int, char, long, double>     tup_iclr;
    typedef std::integral_constant<std::size_t, 0>  i0;
    typedef std::integral_constant<std::size_t, 3>  i3;

    // basic positions on a pack
    D_TEST_TYPE_EQ(tuple_type_at_t<0, int, char, long>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<1, int, char, long>, char);
    D_TEST_TYPE_EQ(tuple_type_at_t<2, int, char, long>, long);

    // the tuple form
    D_TEST_TYPE_EQ(tuple_type_at_t<0, tup_iclr>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<3, tup_iclr>, double);

    // cross-check against std::tuple_element across all indices
    D_TEST_TYPE_EQ(
        tuple_type_at_t<0, tup_iclr>,
        typename std::tuple_element<0, tup_iclr>::type);
    D_TEST_TYPE_EQ(
        tuple_type_at_t<1, tup_iclr>,
        typename std::tuple_element<1, tup_iclr>::type);
    D_TEST_TYPE_EQ(
        tuple_type_at_t<2, tup_iclr>,
        typename std::tuple_element<2, tup_iclr>::type);
    D_TEST_TYPE_EQ(
        tuple_type_at_t<3, tup_iclr>,
        typename std::tuple_element<3, tup_iclr>::type);

    // detection probe for in-range indices.  The out-of-range
    // SFINAE-friendliness contract is tested separately in the
    // meta group (tests_dtuple_sfinae_friendliness) because the
    // current helper may hard-error rather than SFINAE-fail;
    // pinning that behavior in one place avoids double-counting.
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_type_at,
                          i0, tup_iclr);
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_type_at,
                          i3, tup_iclr);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "tuple_type_at/all-checks");

    return true;
}


// =========================================================================
// II.  TUPLE_SUBSEQUENCE
// =========================================================================

/*
tests_dtuple_tuple_subsequence
  Verifies tuple_subsequence's half-open slicing.
  Tests the following:
  - [0, 0) yields the empty tuple (both-endpoints-zero degenerate)
  - [0, size) yields the whole tuple
  - interior slice [1, 3) yields the expected elements
  - zero-length slice at interior position yields the empty tuple
  - detection probe succeeds for every well-formed [start, end) pair
*/
bool
tests_dtuple_tuple_subsequence(
    test_handler& _handler
)
{
    typedef std::tuple<int, char, long, double>     tup_iclr;
    typedef std::integral_constant<std::size_t, 0>  n0;
    typedef std::integral_constant<std::size_t, 1>  n1;
    typedef std::integral_constant<std::size_t, 2>  n2;
    typedef std::integral_constant<std::size_t, 3>  n3;
    typedef std::integral_constant<std::size_t, 4>  n4;

    // half-open [start, end)
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 0, tup_iclr>,
                   std::tuple<>);
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 4, tup_iclr>,
                   tup_iclr);
    D_TEST_TYPE_EQ(tuple_subsequence_t<1, 3, tup_iclr>,
                   std::tuple<char, long>);

    // zero-length interior slice
    D_TEST_TYPE_EQ(tuple_subsequence_t<2, 2, tup_iclr>,
                   std::tuple<>);

    // empty-tuple slice (zero-size-in, zero-size-out)
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 0, std::tuple<>>,
                   std::tuple<>);

    // detection probe: well-formed slices are detected
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_subsequence,
                          n0, n0, tup_iclr);
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_subsequence,
                          n0, n4, tup_iclr);
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_subsequence,
                          n1, n3, tup_iclr);
    D_TEST_TRAIT_DETECTED(internal::probe_tuple_subsequence,
                          n2, n2, tup_iclr);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "tuple_subsequence/all-checks");

    return true;
}


// =========================================================================
// III. TUPLE_SPLIT
// =========================================================================

/*
tests_dtuple_tuple_split
  Verifies tuple_split cleanly divides a tuple at an arbitrary
  index into a ::before and ::after pair that, when rejoined,
  reproduce the original tuple exactly.
  Tests the following:
  - split at index 0 yields empty/full
  - split at the middle yields the expected halves
  - split at end yields full/empty
  - round-trip: before ++ after == original
  - splitting an empty tuple at 0 yields empty/empty
*/
bool
tests_dtuple_tuple_split(
    test_handler& _handler
)
{
    typedef std::tuple<int, char, long, double>  tup_iclr;
    typedef tuple_split<2, tup_iclr>             split_mid;

    // boundary splits
    D_TEST_TYPE_EQ(typename tuple_split<0, tup_iclr>::before,
                   std::tuple<>);
    D_TEST_TYPE_EQ(typename tuple_split<0, tup_iclr>::after,
                   tup_iclr);
    D_TEST_TYPE_EQ(typename tuple_split<4, tup_iclr>::before,
                   tup_iclr);
    D_TEST_TYPE_EQ(typename tuple_split<4, tup_iclr>::after,
                   std::tuple<>);

    // middle split
    D_TEST_TYPE_EQ(typename split_mid::before,
                   std::tuple<int, char>);
    D_TEST_TYPE_EQ(typename split_mid::after,
                   std::tuple<long, double>);

    // round-trip: split + join == original
    D_TEST_TYPE_EQ(
        typename tuple_join<typename split_mid::before,
                            typename split_mid::after>::type,
        tup_iclr);

    // empty tuple split at 0
    D_TEST_TYPE_EQ(typename tuple_split<0, std::tuple<>>::before,
                   std::tuple<>);
    D_TEST_TYPE_EQ(typename tuple_split<0, std::tuple<>>::after,
                   std::tuple<>);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "tuple_split/all-checks");

    return true;
}


// =========================================================================
// IV.  REFERENCE-QUALIFIED ROUND-TRIP
// =========================================================================

/*
tests_dtuple_indexing_refqual_roundtrip
  Verifies every index-based operation preserves reference and cv
  qualifiers on tuple elements.  std::tuple can legitimately hold
  reference-qualified types as elements — and metaprograms
  routinely drop those qualifiers by accident.  This test makes
  sure nothing in the indexing surface does so.
  Tests the following:
  - tuple_type_at returns the exact qualified element type
  - tuple_subsequence preserves the exact qualified element types
  - tuple_split's before / after halves preserve qualifiers
  - round-trip (split + join) reproduces the original type exactly,
    references and cv-qualifiers intact
*/
bool
tests_dtuple_indexing_refqual_roundtrip(
    test_handler& _handler
)
{
    // A deliberately nasty pack: lvalue reference, const rvalue
    // reference, volatile pointer, plain value.  Each element
    // stresses a different category of qualifier preservation.
    typedef std::tuple<int&,
                       const char&&,
                       volatile long*,
                       double>            tup_ref;
    typedef tuple_split<2, tup_ref>       split_mid;

    // tuple_type_at preserves references
    D_TEST_TYPE_EQ(tuple_type_at_t<0, tup_ref>, int&);
    D_TEST_TYPE_EQ(tuple_type_at_t<1, tup_ref>, const char&&);
    D_TEST_TYPE_EQ(tuple_type_at_t<2, tup_ref>, volatile long*);
    D_TEST_TYPE_EQ(tuple_type_at_t<3, tup_ref>, double);

    // tuple_type_at agrees with std::tuple_element (which IS
    // required to preserve qualifiers).  A divergence here
    // indicates dtuple is dropping a qualifier the std does not.
    D_TEST_TYPE_EQ(
        tuple_type_at_t<0, tup_ref>,
        typename std::tuple_element<0, tup_ref>::type);
    D_TEST_TYPE_EQ(
        tuple_type_at_t<1, tup_ref>,
        typename std::tuple_element<1, tup_ref>::type);
    D_TEST_TYPE_EQ(
        tuple_type_at_t<2, tup_ref>,
        typename std::tuple_element<2, tup_ref>::type);

    // tuple_subsequence preserves qualifiers across a slice
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 2, tup_ref>,
                   std::tuple<int&, const char&&>);
    D_TEST_TYPE_EQ(tuple_subsequence_t<2, 4, tup_ref>,
                   std::tuple<volatile long*, double>);
    D_TEST_TYPE_EQ(tuple_subsequence_t<1, 3, tup_ref>,
                   std::tuple<const char&&, volatile long*>);

    // tuple_split halves preserve qualifiers
    D_TEST_TYPE_EQ(typename split_mid::before,
                   std::tuple<int&, const char&&>);
    D_TEST_TYPE_EQ(typename split_mid::after,
                   std::tuple<volatile long*, double>);

    // round-trip: split + join reproduces the ORIGINAL qualified
    // tuple type exactly
    D_TEST_TYPE_EQ(
        typename tuple_join<typename split_mid::before,
                            typename split_mid::after>::type,
        tup_ref);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "indexing_refqual_roundtrip/all-checks");

    return true;
}


// =========================================================================
// V.   INSTANTIATION-DEPTH STRESS
// =========================================================================

/*
tests_dtuple_indexing_stress_depth
  Verifies the index-based operations remain well-scaled on large
  tuples (32 elements).  tuple_subsequence implemented via
  std::index_sequence is O(1) in template instantiation depth; a
  hand-rolled recursive implementation is O(N).  A slow test here
  is a signal that the implementation has regressed to the recursive
  form.
  Tests the following:
  - the 32-element tuple generated by repeat_t agrees with the
    hand-written 32-element tuple (cross-check that repeat_t is
    producing what we expect)
  - tuple_subsequence of the first half, second half, and full
    range all yield the expected tuples
  - tuple_split at the midpoint rejoins to the original
  - tuple_type_at at index 0, 15, 16, and 31 return the expected
    element type (int in every position — the stress isn't about
    element variety, it's about size)
*/
bool
tests_dtuple_indexing_stress_depth(
    test_handler& _handler
)
{
    // generated via repeat_t (one line, relies on djinterp::repeat)
    typedef repeat_t<int, 32> tup_32_generated;

    // hand-written 32-element tuple (deliberately verbose to
    // avoid circular reliance on repeat_t — if repeat_t regresses,
    // the cross-check below catches it without confusing the
    // diagnosis)
    typedef std::tuple<int, int, int, int, int, int, int, int,
                       int, int, int, int, int, int, int, int,
                       int, int, int, int, int, int, int, int,
                       int, int, int, int, int, int, int, int>
        tup_32_handwritten;

    // the two representations must agree
    D_TEST_TYPE_EQ(tup_32_generated, tup_32_handwritten);

    // std::tuple_size reports 32 for both
    static_assert(
        (std::tuple_size<tup_32_generated>::value   == 32),
        "repeat_t yields a 32-element tuple");
    static_assert(
        (std::tuple_size<tup_32_handwritten>::value == 32),
        "hand-written 32-element tuple has size 32");

    // tuple_subsequence: first half
    D_TEST_TYPE_EQ(
        tuple_subsequence_t<0, 16, tup_32_generated>,
        repeat_t<int, 16>);

    // tuple_subsequence: second half
    D_TEST_TYPE_EQ(
        tuple_subsequence_t<16, 32, tup_32_generated>,
        repeat_t<int, 16>);

    // tuple_subsequence: full range
    D_TEST_TYPE_EQ(
        tuple_subsequence_t<0, 32, tup_32_generated>,
        tup_32_generated);

    // tuple_split at midpoint + rejoin == original
    typedef tuple_split<16, tup_32_generated> split_16;
    D_TEST_TYPE_EQ(
        typename tuple_join<typename split_16::before,
                            typename split_16::after>::type,
        tup_32_generated);

    // tuple_type_at across the span
    D_TEST_TYPE_EQ(tuple_type_at_t<0,  tup_32_generated>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<15, tup_32_generated>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<16, tup_32_generated>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<31, tup_32_generated>, int);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "indexing_stress_depth/all-checks");

    return true;
}


NS_END  // testing
NS_END  // djinterp
