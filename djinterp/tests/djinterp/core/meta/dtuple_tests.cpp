/******************************************************************************
* djinterp [testing]                                          dtuple_tests.cpp
*
*   Implementation of the DTest framework demonstration test suite for the 
* djinterp dtuple module.  See tests_dtuple.hpp for the API and the full list
* of framework features exercised here.
* 
* 
* path:      /tests/djinterp/core/meta/dtuple_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.17
******************************************************************************/
#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING



// =========================================================================
// PROBE ALIASES
// =========================================================================
// Detection-idiom probes must live at namespace scope (alias templates
// cannot be declared inside a function body in C++11/14/17).  Each probe
// is a one-line alias whose ill-formedness is the trait being tested;
// `is_detected<probe, args...>` then yields a clean true/false answer.

namespace {

    // probe_first_arg
    //   probe: yields the first type in a pack via dtuple's
    // first_arg_t.  Detection succeeds iff the pack is non-empty.
    template<typename... _Ts>
    using probe_first_arg = first_arg_t<_Ts...>;

    // probe_tuple_type_at
    //   probe: yields the type at the supplied index of a tuple.
    // Detection succeeds iff the index is in range.
    template<typename _IndexConst,
             typename _Tuple>
    using probe_tuple_type_at =
        tuple_type_at_t<_IndexConst::value, _Tuple>;

    // probe_make_tuple_of
    //   probe: yields a tuple of N copies of T.  Detection
    // succeeds for every non-negative N (including 0).
    template<typename _T,
             typename _CountConst>
    using probe_make_tuple_of =
        make_tuple_of_t<_T, _CountConst::value>;

}  // anonymous namespace


// =========================================================================
// PREDICATE WRAPPERS
// =========================================================================
// The pack quantifiers in test_trait.hpp expect predicates of the form
// `template<typename> class P`.  Several dtuple traits take additional
// non-type or template-template parameters and must be one-line wrapped
// before they can be quantified across a type pack.

namespace {

    // is_dtuple_pred
    //   trait: unary predicate forwarding to djinterp::is_tuple.
    template<typename _T>
    struct is_dtuple_pred
    {
        static constexpr bool value = is_tuple<_T>::value;
    };

    // is_dtuple_homogeneous_pred
    //   trait: unary predicate forwarding to is_tuple_homogeneous.
    template<typename _T>
    struct is_dtuple_homogeneous_pred
    {
        static constexpr bool value =
            is_tuple_homogeneous<_T>::value;
    };

    // is_dtuple_2d_pred
    //   trait: unary predicate forwarding to is_2d_tuple.
    template<typename _T>
    struct is_dtuple_2d_pred
    {
        static constexpr bool value = is_2d_tuple<_T>::value;
    };

}  // anonymous namespace


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
  - detection fails on the empty pack (ill-formed by design)
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
    D_TEST_TYPE_EQ(first_arg_t<int* const, char>,   int* const);

    // alias consistency: ::type vs the _t suffix
    D_TEST_TRAIT_ALIAS_CONSISTENT(first_arg, first_arg_t, int);

    // detection probe — empty pack must fail
    D_TEST_TRAIT_DETECTED    (probe_first_arg, int);
    D_TEST_TRAIT_DETECTED    (probe_first_arg, int, char);
    D_TEST_TRAIT_NOT_DETECTED(probe_first_arg);

    _handler.fire<on_compile_check>("first_arg/all-checks");

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
  - documents the (intentional) non-cv-stability of is_tuple
*/
bool
tests_dtuple_is_tuple(
    test_handler& _handler
)
{
    // positive cases
    D_TEST_TRAIT_TRUE(is_tuple, std::tuple<>);
    D_TEST_TRAIT_TRUE(is_tuple, std::tuple<int>);
    D_TEST_TRAIT_TRUE(is_tuple, std::tuple<int, char, long>);

    // negative cases
    D_TEST_TRAIT_FALSE(is_tuple, int);
    D_TEST_TRAIT_FALSE(is_tuple, void);
    typedef std::pair<int, int> int_pair;
    D_TEST_TRAIT_FALSE(is_tuple, int_pair);

    // pack quantification
    D_TEST_TRAIT_ALL_OF(is_dtuple_pred,
                        std::tuple<>,
                        std::tuple<int>,
                        std::tuple<int, char>);

    D_TEST_TRAIT_NONE_OF(is_dtuple_pred,
                         int,
                         char,
                         double);

    D_TEST_TRAIT_ANY_OF(is_dtuple_pred,
                        int,
                        std::tuple<char>,
                        double);

    // documented non-stability: is_tuple is NOT cv-stable as
    // currently written — `const std::tuple<int>` falls through to
    // the primary template and reports false.  This assertion
    // pins that behavior so any future change is caught.
    D_TEST_TRAIT_FALSE(is_tuple, const std::tuple<int>);

    _handler.fire<on_compile_check>("is_tuple/all-checks");

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
    // wrap a non-tuple
    D_TEST_TYPE_EQ(to_tuple_t<int>,  std::tuple<int>);
    D_TEST_TYPE_EQ(to_tuple_t<void>, std::tuple<void>);

    // pass through an existing tuple
    typedef std::tuple<int, char> tup_ic;
    D_TEST_TYPE_EQ(to_tuple_t<tup_ic>, tup_ic);
    D_TEST_TYPE_EQ(to_tuple_t<std::tuple<>>, std::tuple<>);

    // multi-arg pack
    D_TEST_TYPE_EQ(to_tuple_t<int, char, long>,
                   std::tuple<int, char, long>);

    // alias consistency
    D_TEST_TRAIT_ALIAS_CONSISTENT(to_tuple, to_tuple_t, int);
    D_TEST_TRAIT_ALIAS_CONSISTENT(to_tuple, to_tuple_t, std::tuple<int>);

    _handler.fire<on_compile_check>("to_tuple/all-checks");

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
*/
bool
tests_dtuple_make_tuple_of(
    test_handler& _handler
)
{
    // degenerate cases
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 0>, std::tuple<>);
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 1>, std::tuple<int>);

    // non-degenerate
    D_TEST_TYPE_EQ(make_tuple_of_t<int, 3>,
                   std::tuple<int, int, int>);
    D_TEST_TYPE_EQ(make_tuple_of_t<char, 4>,
                   std::tuple<char, char, char, char>);

    // detection across a value range
    typedef std::integral_constant<std::size_t, 0> n0;
    typedef std::integral_constant<std::size_t, 1> n1;
    typedef std::integral_constant<std::size_t, 8> n8;
    D_TEST_TRAIT_DETECTED(probe_make_tuple_of, int, n0);
    D_TEST_TRAIT_DETECTED(probe_make_tuple_of, int, n1);
    D_TEST_TRAIT_DETECTED(probe_make_tuple_of, int, n8);

    _handler.fire<on_compile_check>("make_tuple_of/all-checks");

    return true;
}


// =========================================================================
// V.   WRAP_ALL AND MODIFIERS
// =========================================================================

/*
tests_dtuple_wrap_all_and_modifiers
  Verifies wrap_all composes unary type modifiers right-to-left
  and that the supplied to_lvalue_reference / to_rvalue_reference /
  to_pointer / to_type modifiers behave as advertised.
  Tests the following:
  - empty modifier pack is the identity
  - single modifier round-trips through std::add_pointer_t
  - to_pointer / to_lvalue_reference / to_rvalue_reference functor
    aliases match their std equivalents
  - to_type is the identity functor
*/
bool
tests_dtuple_wrap_all_and_modifiers(
    test_handler& _handler
)
{
    // empty modifier pack -> identity
    D_TEST_TYPE_EQ(wrap_all_t<int>, int);

    // single modifier
    D_TEST_TYPE_EQ(wrap_all_t<int, std::add_pointer>, int*);

    // composition: order is left-to-right (outer first per the
    // helper's recursive expansion).  Pin the actual behavior so
    // callers can reason about it.
    D_TEST_TYPE_EQ(wrap_all_t<int, std::add_pointer, std::add_const>,
        const int*);

    // functor-style modifiers
    D_TEST_TYPE_EQ(to_pointer::type<int>,           int*);
    D_TEST_TYPE_EQ(to_lvalue_reference::type<int>,  int&);
    D_TEST_TYPE_EQ(to_rvalue_reference::type<int>,  int&&);

    // to_type as identity
    D_TEST_TYPE_EQ(to_type_t<int>, int);
    D_TEST_TYPE_EQ(to_type_t<const volatile char*>,
                   const volatile char*);

    _handler.fire<on_compile_check>("wrap_all_and_modifiers/all-checks");

    return true;
}


// =========================================================================
// VI.  TUPLE_JOIN
// =========================================================================

/*
tests_dtuple_tuple_join
  Verifies tuple_join concatenates an arbitrary number of tuples
  while obeying the algebraic identity laws of concatenation.
  Tests the following:
  - joining nothing yields the empty tuple
  - joining a single tuple is identity
  - joining two tuples concatenates them
  - the empty tuple is a left and right identity
  - joining is associative across three tuples
*/
bool
tests_dtuple_tuple_join(
    test_handler& _handler
)
{
    typedef std::tuple<>                 t_empty;
    typedef std::tuple<int>              t_i;
    typedef std::tuple<char>             t_c;
    typedef std::tuple<long>             t_l;
    typedef std::tuple<int, char>        t_ic;
    typedef std::tuple<int, char, long>  t_icl;

    // identity laws
    D_TEST_TYPE_EQ(typename tuple_join<>::type,            t_empty);
    D_TEST_TYPE_EQ(typename tuple_join<t_i>::type,         t_i);
    D_TEST_TYPE_EQ(typename tuple_join<t_empty, t_i>::type, t_i);
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_empty>::type, t_i);

    // basic concatenation
    D_TEST_TYPE_EQ(typename tuple_join<t_i, t_c>::type, t_ic);

    // associativity: (i ++ c) ++ l  ==  i ++ (c ++ l)
    typedef typename tuple_join<typename tuple_join<t_i, t_c>::type, t_l>::type
        left_assoc;
    typedef typename tuple_join<t_i, typename tuple_join<t_c, t_l>::type>::type
        right_assoc;
    D_TEST_TYPE_EQ(left_assoc,  t_icl);
    D_TEST_TYPE_EQ(right_assoc, t_icl);
    D_TEST_TYPE_EQ(left_assoc,  right_assoc);

    _handler.fire<on_compile_check>("tuple_join/all-checks");

    return true;
}


// =========================================================================
// VII. TUPLE_TYPE_AT
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
    typedef std::tuple<int, char, long, double> tup_iclr;

    // basic positions on a pack
    D_TEST_TYPE_EQ(tuple_type_at_t<0, int, char, long>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<1, int, char, long>, char);
    D_TEST_TYPE_EQ(tuple_type_at_t<2, int, char, long>, long);

    // the tuple form
    D_TEST_TYPE_EQ(tuple_type_at_t<0, tup_iclr>, int);
    D_TEST_TYPE_EQ(tuple_type_at_t<3, tup_iclr>, double);

    // cross-check against std::tuple_element across all indices
    D_TEST_TYPE_EQ(tuple_type_at_t<0, tup_iclr>,
                   typename std::tuple_element<0, tup_iclr>::type);
    D_TEST_TYPE_EQ(tuple_type_at_t<1, tup_iclr>,
                   typename std::tuple_element<1, tup_iclr>::type);
    D_TEST_TYPE_EQ(tuple_type_at_t<2, tup_iclr>,
                   typename std::tuple_element<2, tup_iclr>::type);
    D_TEST_TYPE_EQ(tuple_type_at_t<3, tup_iclr>,
                   typename std::tuple_element<3, tup_iclr>::type);

    // detection probe for in-range vs out-of-range indices.  The
    // helper pattern fails to match for indices >= size, yielding
    // a hard error rather than a SFINAE-friendly false.  We
    // therefore only test the positive detection case here.
    typedef std::integral_constant<std::size_t, 0> i0;
    typedef std::integral_constant<std::size_t, 3> i3;
    D_TEST_TRAIT_DETECTED(probe_tuple_type_at, i0, tup_iclr);
    D_TEST_TRAIT_DETECTED(probe_tuple_type_at, i3, tup_iclr);

    _handler.fire<on_compile_check>("tuple_type_at/all-checks");

    return true;
}


// =========================================================================
// VIII. TUPLE_APPLY_ALL
// =========================================================================

/*
tests_dtuple_tuple_apply_all
  Verifies tuple_apply_all maps a unary trait across every type
  in a pack and reassembles the results into a tuple.
  Tests the following:
  - applying std::add_pointer pointer-decorates every element
  - applying std::add_const const-decorates every element
  - applying the identity-yielding to_type leaves the tuple unchanged
  - the empty pack yields the empty tuple
*/
bool
tests_dtuple_tuple_apply_all(
    test_handler& _handler
)
{
    // NOTE on trait- vs alias-form templates:
    //   tuple_apply_all applies its _UnaryTrait directly (i.e. uses
    // the result of `_UnaryTrait<_Head>` as the element type — it
    // does NOT extract a nested `::type`).  Consequently the tests
    // below supply ALIAS-form templates (`std::add_pointer_t`,
    // `std::add_const_t`), NOT trait-form (`std::add_pointer`,
    // `std::add_const`).  This is the opposite convention from
    // wrap_all, which DOES extract `::type` and therefore takes
    // trait-form modifiers.  Both conventions are self-consistent
    // within their respective component.

    // pointer-decorate every element
    D_TEST_TYPE_EQ(tuple_apply_all_t<std::add_pointer_t, int, char, long>,
        std::tuple<int*, char*, long*>);

    // const-decorate every element
    D_TEST_TYPE_EQ(tuple_apply_all_t<std::add_const_t, int, char>,
        std::tuple<const int, const char>);

    // empty pack yields the empty tuple.  This exercises the
    // `to_tuple<>` empty-pack specialization in dtuple.hpp — before
    // that specialization existed, instantiating tuple_apply_all_t
    // with no type arguments triggered an undefined-template error
    // through to_tuple's std::conditional branch.
    D_TEST_TYPE_EQ(tuple_apply_all_t<std::add_pointer_t>,
        std::tuple<>);

    _handler.fire<on_compile_check>("tuple_apply_all/all-checks");

    return true;
}


// =========================================================================
// IX.  COUNT_AND_REMOVE
// =========================================================================

/*
tests_dtuple_count_and_remove
  Verifies tuple_count_and_remove and tuple_count_type produce
  consistent counts and that the post-removal tuple is correct.
  Tests the following:
  - removing a type not in the pack leaves the tuple unchanged and
    yields a count of zero
  - removing the only occurrence of a type leaves an empty residual
  - removing a type that occurs multiple times removes every copy
  - tuple_count_type's value matches tuple_count_and_remove's value
    for every test case
*/
bool
tests_dtuple_count_and_remove(
    test_handler& _handler
)
{
    typedef std::tuple<int, char, int, long, int> tup_iclil;

    // type not present
    D_TEST_TYPE_EQ(tuple_count_and_remove_t<double, int, char>,
                   std::tuple<int, char>);
    static_assert(
        (tuple_count_and_remove<double, int, char>::value == 0),
        "removing absent type yields count 0");

    // single occurrence
    D_TEST_TYPE_EQ(tuple_count_and_remove_t<char, tup_iclil>,
                   std::tuple<int, int, long, int>);
    static_assert(
        (tuple_count_and_remove<char, tup_iclil>::value == 1),
        "single occurrence yields count 1");

    // multiple occurrences
    D_TEST_TYPE_EQ(tuple_count_and_remove_t<int, tup_iclil>,
                   std::tuple<char, long>);
    static_assert(
        (tuple_count_and_remove<int, tup_iclil>::value == 3),
        "three occurrences of int counted");

    // count_type agrees with count_and_remove::value
    static_assert(
        (tuple_count_type<int, tup_iclil>::value ==
         tuple_count_and_remove<int, tup_iclil>::value),
        "tuple_count_type and tuple_count_and_remove::value agree");
    static_assert(
        (tuple_count_type<char, tup_iclil>::value ==
         tuple_count_and_remove<char, tup_iclil>::value),
        "tuple_count_type and tuple_count_and_remove::value agree");
    static_assert(
        (tuple_count_type<double, tup_iclil>::value ==
         tuple_count_and_remove<double, tup_iclil>::value),
        "tuple_count_type and tuple_count_and_remove::value agree");

    _handler.fire<on_compile_check>("count_and_remove/all-checks");

    return true;
}


// =========================================================================
// X.   SPLIT AND SUBSEQUENCE
// =========================================================================

/*
tests_dtuple_split_and_subsequence
  Verifies tuple_split's before/after halves and tuple_subsequence's
  half-open slicing, plus the algebraic round-trip property.
  Tests the following:
  - split at index 0 yields empty/full
  - split at the middle yields the expected halves
  - split at end yields full/empty
  - subsequence selects the correct range
  - splitting then rejoining the halves reproduces the original
    tuple
*/
bool
tests_dtuple_split_and_subsequence(
    test_handler& _handler
)
{
    typedef std::tuple<int, char, long, double> tup_iclr;
    typedef tuple_split<2, tup_iclr>            split_mid;

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
    D_TEST_TYPE_EQ(typename tuple_join<typename split_mid::before,
                             typename split_mid::after>::type,
        tup_iclr);

    // subsequence: half-open [start, end)
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 0, tup_iclr>,
                   std::tuple<>);
    D_TEST_TYPE_EQ(tuple_subsequence_t<0, 4, tup_iclr>,
                   tup_iclr);
    D_TEST_TYPE_EQ(tuple_subsequence_t<1, 3, tup_iclr>,
                   std::tuple<char, long>);

    _handler.fire<on_compile_check>("split_and_subsequence/all-checks");

    return true;
}


// =========================================================================
// XI.  TYPE_SELECTOR
// =========================================================================

/*
tests_dtuple_type_selector
  Verifies type_selector resolves to the type of the first
  type_case whose condition is true, or to void when none match.
  Tests the following:
  - first matching case wins
  - later cases are ignored even when also true
  - empty selector yields void
  - selector with all-false cases yields void
*/
bool
tests_dtuple_type_selector(
    test_handler& _handler
)
{
    // first match wins
    D_TEST_TYPE_EQ(type_select_t<type_case<true,  int>,
                       type_case<false, char>>,
        int);

    // skip false case to pick later true case
    D_TEST_TYPE_EQ(type_select_t<type_case<false, int>,
                       type_case<true,  char>>,
        char);

    // multiple true cases — first wins
    D_TEST_TYPE_EQ(type_select_t<type_case<true,  long>,
                       type_case<true,  char>,
                       type_case<true,  int>>,
        long);

    // no cases -> void
    D_TEST_TYPE_EQ(type_select_t<>, void);

    // all-false -> void
    D_TEST_TYPE_EQ(type_select_t<type_case<false, int>,
                       type_case<false, char>>,
        void);

    _handler.fire<on_compile_check>("type_selector/all-checks");

    return true;
}


// =========================================================================
// XII. HOMOGENEITY AND 2D
// =========================================================================

/*
tests_dtuple_homogeneity_and_2d
  Verifies is_tuple_homogeneous and is_2d_tuple classify tuples
  correctly across single-element, multi-element, mixed, nested,
  and degenerate inputs.
  Tests the following:
  - single-element tuples are homogeneous
  - multi-element tuples of one type are homogeneous
  - mixed-type tuples are NOT homogeneous
  - empty tuple's homogeneity classification (current behavior)
  - 2D tuples are recognized as 2D
  - flat tuples are NOT recognized as 2D
  - empty tuple is recognized as vacuously 2D (per current code)
*/
bool
tests_dtuple_homogeneity_and_2d(
    test_handler& _handler
)
{
    // homogeneity — positive
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous, std::tuple<int>);
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous, std::tuple<int, int>);
    D_TEST_TRAIT_TRUE(is_tuple_homogeneous,
                      std::tuple<char, char, char>);

    // homogeneity — negative
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous,
                       std::tuple<int, char>);
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous,
                       std::tuple<int, int, char>);

    // empty tuple is NOT homogeneous as currently written (falls
    // through to the primary template).  Pin the behavior.
    D_TEST_TRAIT_FALSE(is_tuple_homogeneous, std::tuple<>);

    // 2D — positive
    D_TEST_TRAIT_TRUE(is_2d_tuple, std::tuple<>);
    D_TEST_TRAIT_TRUE(is_2d_tuple,
                      std::tuple<std::tuple<int>>);
    D_TEST_TRAIT_TRUE(is_2d_tuple,
                      std::tuple<std::tuple<int>,
                                  std::tuple<char, long>>);

    // 2D — negative
    D_TEST_TRAIT_FALSE(is_2d_tuple, int);
    D_TEST_TRAIT_FALSE(is_2d_tuple,
                       std::tuple<int, char>);
    D_TEST_TRAIT_FALSE(is_2d_tuple,
                       std::tuple<std::tuple<int>, char>);

    // outer size of a 2D tuple
    static_assert(
        (tuple_outer_size<std::tuple<std::tuple<int>,
                                     std::tuple<char>>>::value == 2),
        "outer size of 2x1 is 2");

    _handler.fire<on_compile_check>("homogeneity_and_2d/all-checks");

    return true;
}


// =========================================================================
// XIII. FLATTEN AND NORMALIZE
// =========================================================================

/*
tests_dtuple_flatten_and_normalize
  Verifies tuple_flatten_types collapses a 2D tuple into its
  concatenated row contents, and that normalize_tuple strips
  cv- and reference-qualifiers from every element.
  Tests the following:
  - flattening the empty 2D tuple yields the empty tuple
  - flattening a single-row 2D tuple yields its row
  - flattening multiple rows yields their concatenation
  - normalize strips const, volatile, references, and combinations
  - normalize is idempotent
*/
bool
tests_dtuple_flatten_and_normalize(
    test_handler& _handler
)
{
    // flatten edge cases
    D_TEST_TYPE_EQ(tuple_flatten_types_t<std::tuple<>>,
                   std::tuple<>);
    D_TEST_TYPE_EQ(tuple_flatten_types_t<std::tuple<std::tuple<int>>>,
        std::tuple<int>);

    // flatten multi-row
    D_TEST_TYPE_EQ(tuple_flatten_types_t<
            std::tuple<std::tuple<int, char>,
                       std::tuple<long, double>>>,
        std::tuple<int, char, long, double>);

    // normalize
    D_TEST_TYPE_EQ(normalize_tuple_t<std::tuple<const int, volatile char>>,
        std::tuple<int, char>);

    D_TEST_TYPE_EQ(normalize_tuple_t<std::tuple<int&, const char&&>>,
        std::tuple<int, char>);

    D_TEST_TYPE_EQ(normalize_tuple_t<std::tuple<const volatile int&,
                                      char* const>>,
        std::tuple<int, char*>);  // clean_t on `char* const` removes
                                  // the top-level const qualifying the
                                  // POINTER (it becomes `char*`); the
                                  // pointee type (`char`) is unaffected.
                                  // A test that wants to preserve
                                  // pointee-qualifications should pass
                                  // a pointer-to-const instead, e.g.
                                  // `const char*`.

    // idempotence: normalize(normalize(x)) == normalize(x)
    typedef normalize_tuple_t<std::tuple<const int&, char>> norm_once;
    typedef normalize_tuple_t<norm_once>                     norm_twice;
    D_TEST_TYPE_EQ(norm_once, norm_twice);

    _handler.fire<on_compile_check>("flatten_and_normalize/all-checks");

    return true;
}


// =========================================================================
// XIV. ROBUSTNESS — CV/REF STABILITY
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
        (!trait_cv_stable<is_dtuple_pred,
                          std::tuple<int>>::value),
        "is_tuple is NOT cv-stable on tuples (current behavior)");

    static_assert(
        (!trait_ref_stable<is_dtuple_pred,
                           std::tuple<int>>::value),
        "is_tuple is NOT ref-stable on tuples (current behavior)");

    // BUT for non-tuple types is_tuple is uniformly false
    // and therefore trivially cv/ref-stable.
    D_TEST_TRAIT_CVREF_STABLE(is_dtuple_pred, int);
    D_TEST_TRAIT_CVREF_STABLE(is_dtuple_pred, double);

    // is_tuple_homogeneous and is_2d_tuple inherit the same
    // sensitivity for the same reason: cv-qualified tuples no
    // longer match the partial specialization.
    static_assert(
        (!trait_cv_stable<is_dtuple_homogeneous_pred,
                          std::tuple<int>>::value),
        "is_tuple_homogeneous not cv-stable on tuples");
    static_assert(
        (!trait_cv_stable<is_dtuple_2d_pred,
                          std::tuple<std::tuple<int>>>::value),
        "is_2d_tuple not cv-stable on tuples");

    _handler.fire<on_compile_check>("robustness/all-checks");

    return true;
}


// =========================================================================
// XV.  LOGICAL RELATIONSHIPS
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
  - is_2d_tuple and "non-tuple" are disjoint (a non-tuple cannot be
    a 2D tuple)
*/
bool
tests_dtuple_logical_relationships(
    test_handler& _handler
)
{
    // is_2d_tuple implies is_tuple over a representative pack
    D_TEST_TRAIT_IMPLIES(
        is_dtuple_2d_pred,
        is_dtuple_pred,
        int,
        char,
        std::tuple<>,
        std::tuple<int>,
        std::tuple<std::tuple<int>>,
        std::tuple<std::tuple<int>, std::tuple<char>>);

    // is_tuple_homogeneous implies is_tuple
    D_TEST_TRAIT_IMPLIES(
        is_dtuple_homogeneous_pred,
        is_dtuple_pred,
        int,
        std::tuple<>,
        std::tuple<int>,
        std::tuple<int, int>,
        std::tuple<int, char>);

    // is_2d_tuple and is_tuple are NOT equivalent: the empty tuple
    // is_2d=true and is_tuple=true (agree there), but a flat
    // tuple<int> has is_2d=false and is_tuple=true (disagree).
    // So the equivalence relation does NOT hold over this pack:
    static_assert(
        (!trait_equivalent_for<is_dtuple_2d_pred,
                               is_dtuple_pred,
                               std::tuple<int>>::value),
        "is_2d_tuple is not equivalent to is_tuple "
        "(a flat tuple is one but not the other)");

    _handler.fire<on_compile_check>("logical_relationships/all-checks");

    return true;
}


// =========================================================================
// XVI. ALIAS CONSISTENCY (BULK)
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
  - tuple_apply_all / tuple_apply_all_t agree (via wrapper trait)
  - tuple_count_and_remove / tuple_count_and_remove_t agree
  - normalize_tuple / normalize_tuple_t agree
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

    _handler.fire<on_compile_check>("alias_consistency/all-checks");

    return true;
}


// =========================================================================
// COMPILE-TIME AGGREGATE SUITE
// =========================================================================

namespace {

    // dtuple_aggregate_suite
    //   trait: a compile-time trait_suite collecting one
    // representative trait_record per component.  Every record's
    // boolean parameter is a dtuple expression evaluated at
    // template-instantiation time, so the suite as a whole
    // exists only if every check is true.  The result is fed to
    // a trait_suite_object for runtime printer integration.
    using dtuple_aggregate_suite = trait_suite<
        trait_record< std::is_same<first_arg_t<int, char>,
                                    int>::value >,
        trait_record< is_tuple<std::tuple<int>>::value >,
        trait_record< std::is_same<to_tuple_t<int>,
                                    std::tuple<int>>::value >,
        trait_record< std::is_same<make_tuple_of_t<int, 3>,
                                    std::tuple<int, int, int>>::value >,
        trait_record< std::is_same<wrap_all_t<int, std::add_pointer>,
                                    int*>::value >,
        trait_record< std::is_same<typename tuple_join<std::tuple<int>,
                                                        std::tuple<char>
                                                       >::type,
                                    std::tuple<int, char>>::value >,
        trait_record< std::is_same<tuple_type_at_t<1, int, char, long>,
                                    char>::value >,
        trait_record< std::is_same<tuple_apply_all_t<std::add_pointer_t,
                                                      int, char>,
                                    std::tuple<int*, char*>>::value >,
        trait_record< (tuple_count_type<int,
                       std::tuple<int, char, int>>::value == 2) >,
        trait_record< std::is_same<tuple_subsequence_t<1, 3,
                                       std::tuple<int, char, long, double>>,
                                    std::tuple<char, long>>::value >,
        trait_record< std::is_same<type_select_t<type_case<true, int>>,
                                    int>::value >,
        trait_record< is_tuple_homogeneous<std::tuple<int, int>>::value >,
        trait_record< is_2d_tuple<std::tuple<std::tuple<int>>>::value >,
        trait_record< std::is_same<normalize_tuple_t<
                                       std::tuple<const int&>>,
                                    std::tuple<int>>::value >
    >;

}  // anonymous namespace


// =========================================================================
// LOCAL HELPER (FORWARD DECLARATION)
// =========================================================================
// Declared before run_dtuple_tests because the orchestrator calls it
// many times.  The definition appears further down to keep the
// orchestrator's narrative readable.

void
record_outcome(
    test_handler& _handler,
    const char*   _name,
    bool          _result
);


// =========================================================================
// ORCHESTRATOR
// =========================================================================

session_result
run_dtuple_tests(
    test_handler& _handler
)
{
    // ---- bind built-in lifecycle listeners ----

    // on_session_start: announce the demo
    _handler.on<events::on_session_start>(
        [](event_context& /*_ctx*/)
        {
            std::printf(
                "============================================\n"
                "  DTest demo: dtuple compile-time test suite\n"
                "============================================\n");
        });

    // on_session_end: report counters
    _handler.on<events::on_session_end>(
        [](event_context& /*_ctx*/,
           std::size_t    _passed,
           std::size_t    _failed)
        {
            std::printf(
                "--------------------------------------------\n"
                "  finished: %zu passed, %zu failed\n"
                "--------------------------------------------\n",
                _passed,
                _failed);
        });

    // on_test_passed: compact line-per-test
    listener_id passed_id =
        _handler.on<events::on_test_passed>(
            [](event_context&   /*_ctx*/,
               const basic_test* _obj)
            {
                std::printf("  [PASS] %s\n",
                            (_obj && _obj->name())
                                ? _obj->name() : "(unnamed)");
            });

    // on_test_failed: the canonical "‼️" warning.  Cannot fire
    // unless someone introduces a regression — kept as the
    // standing demonstration of how alarm-style listeners hook
    // in without any extra plumbing.
    _handler.on<events::on_test_failed>(
        [](event_context&    /*_ctx*/,
           const basic_test* _obj)
        {
            std::fputs("\xE2\x80\xBC THIS SHOULD NOT HAPPEN",
                       stderr);
            if ( (_obj) &&
                 (_obj->name()) )
            {
                std::fprintf(stderr, " (%s)", _obj->name());
            }
            std::fputc('\n', stderr);
        });

    // ---- bind custom-event listeners ----

    // on_dtuple_demo: empty-payload notification
    _handler.on<on_dtuple_demo>(
        [](event_context& /*_ctx*/)
        {
            std::printf("  (dtuple demo block engaged)\n");
        });

    // on_compile_check: print every fired check name.  We bind
    // it disabled-by-default so it does not flood the output;
    // a caller can enable it with handler.enable(check_id).
    listener_id check_id =
        _handler.on<on_compile_check>(
            [](event_context& /*_ctx*/,
               const char*    _name)
            {
                std::printf("    .. %s\n",
                            _name ? _name : "(unnamed)");
            });
    _handler.disable(check_id);

    // ---- demonstrate deferred dispatch ----

    // queue the demo notification before processing.  any other
    // events fired on the handler in this window happen
    // immediately — only the queued one waits.
    _handler.queue<on_dtuple_demo>();

    // ---- run the session ----

    _handler.start_session();

    // process the queued demo notification now that the session
    // has started.  this prints "(dtuple demo block engaged)"
    // before any test runs.
    _handler.process_all();

    // turn on the verbose check listener for the first test
    // function only — purely to show enable/disable mid-run
    _handler.enable(check_id);

    record_outcome(_handler,
                   "first_arg",
                   tests_dtuple_first_arg(_handler));

    // back to quiet mode for the rest of the run
    _handler.disable(check_id);

    record_outcome(_handler,
                   "is_tuple",
                   tests_dtuple_is_tuple(_handler));
    record_outcome(_handler,
                   "to_tuple",
                   tests_dtuple_to_tuple(_handler));
    record_outcome(_handler,
                   "make_tuple_of",
                   tests_dtuple_make_tuple_of(_handler));
    record_outcome(_handler,
                   "wrap_all_and_modifiers",
                   tests_dtuple_wrap_all_and_modifiers(_handler));
    record_outcome(_handler,
                   "tuple_join",
                   tests_dtuple_tuple_join(_handler));
    record_outcome(_handler,
                   "tuple_type_at",
                   tests_dtuple_tuple_type_at(_handler));
    record_outcome(_handler,
                   "tuple_apply_all",
                   tests_dtuple_tuple_apply_all(_handler));
    record_outcome(_handler,
                   "count_and_remove",
                   tests_dtuple_count_and_remove(_handler));
    record_outcome(_handler,
                   "split_and_subsequence",
                   tests_dtuple_split_and_subsequence(_handler));
    record_outcome(_handler,
                   "type_selector",
                   tests_dtuple_type_selector(_handler));
    record_outcome(_handler,
                   "homogeneity_and_2d",
                   tests_dtuple_homogeneity_and_2d(_handler));
    record_outcome(_handler,
                   "flatten_and_normalize",
                   tests_dtuple_flatten_and_normalize(_handler));
    record_outcome(_handler,
                   "robustness",
                   tests_dtuple_robustness(_handler));
    record_outcome(_handler,
                   "logical_relationships",
                   tests_dtuple_logical_relationships(_handler));
    record_outcome(_handler,
                   "alias_consistency",
                   tests_dtuple_alias_consistency(_handler));

    // ---- runtime adapter for the compile-time aggregate suite
    //      drive the printer from the static suite outcome ----

    trait_suite_object<dtuple_aggregate_suite> aggregate(
        "dtuple_compile_time_aggregate");

    // the aggregate is a different type from basic_test, so we
    // construct a parallel basic_test that mirrors its outcome
    // and fire the lifecycle event with that.  the trait_suite_object
    // remains useful for direct querying (passed/total/failed below)
    // and for placement into a homogeneously-typed printer pipeline.
    basic_test aggregate_node;
    aggregate_node.set_name("dtuple_compile_time_aggregate");
    aggregate_node.set_status(
        static_cast<bool>(aggregate)
            ? basic_test::status_passed
            : basic_test::status_failed);

    if (static_cast<bool>(aggregate))
    {
        _handler.fire<events::on_test_passed>(&aggregate_node);
        _handler.record(test_status::passed);
    }
    else
    {
        _handler.fire<events::on_test_failed>(&aggregate_node);
        _handler.record(test_status::failed);
    }

    std::printf(
        "  (compile-time aggregate: %zu / %zu records pass)\n",
        aggregate.passed(),
        aggregate.total());

    _handler.end_session();

    // suppress unused-variable warning when -Wunused is strict
    (void)passed_id;

    return _handler.result();
}


// =========================================================================
// LOCAL HELPER DEFINITION
// =========================================================================

// record_outcome
//   helper: fires the appropriate built-in lifecycle event
// with a stable const char* name, then updates the handler's
// pass/fail counters.  Constructs a transient basic_test
// solely so the standard lifecycle events have a nameable
// payload to carry.
void
record_outcome(
    test_handler& _handler,
    const char*   _name,
    bool          _result
)
{
    basic_test node;
    node.set_name(_name);
    node.set_status(_result ? basic_test::status_passed
                            : basic_test::status_failed);

    if (_result)
    {
        _handler.fire<events::on_test_passed>(&node);
        _handler.record(test_status::passed);
    }
    else
    {
        _handler.fire<events::on_test_failed>(&node);
        _handler.record(test_status::failed);
    }

    return;
}


NS_END  // testing
NS_END  // djinterp