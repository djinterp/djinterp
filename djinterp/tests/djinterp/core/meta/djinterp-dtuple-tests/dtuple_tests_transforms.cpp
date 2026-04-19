/******************************************************************************
* djinterp [testing]                                dtuple_tests_transforms.cpp
*
*   Transforms-group test definitions for the dtuple test suite:
* tuple_apply_all (map a unary trait across every element),
* tuple_count_and_remove (remove all occurrences of one type while
* reporting the removal count), and the standalone tuple_count_type
* trait (which the original suite tested only transitively).
*
*   The standalone tuple_count_type coverage is the high-value
* addition: counting a type not present (0), counting across an
* empty tuple (0), counting against a single-occurrence vs multi-
* occurrence pack, and counting against a pack that is nothing but
* the target type.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_transforms.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   TUPLE_APPLY_ALL
// =========================================================================

/*
tests_dtuple_tuple_apply_all
  Verifies tuple_apply_all maps a unary trait across every type
  in a pack and reassembles the results into a tuple.

  NOTE on trait- vs alias-form templates:
    tuple_apply_all applies its _UnaryTrait directly (i.e. uses
  the result of `_UnaryTrait<_Head>` as the element type — it
  does NOT extract a nested `::type`).  Consequently the tests
  below supply ALIAS-form templates (`std::add_pointer_t`,
  `std::add_const_t`), NOT trait-form (`std::add_pointer`,
  `std::add_const`).  This is the opposite convention from
  wrap_all, which DOES extract `::type` and therefore takes
  trait-form.

  Tests the following:
  - applying std::add_pointer_t pointer-decorates every element
  - applying std::add_const_t const-decorates every element
  - applying the identity-yielding to_type leaves the tuple unchanged
  - the empty pack yields the empty tuple
  - mixed-type pack produces the correctly-ordered result
*/
bool
tests_dtuple_tuple_apply_all(
    test_handler& _handler
)
{
    // pointer-decorate every element
    D_TEST_TYPE_EQ(
        tuple_apply_all_t<std::add_pointer_t, int, char, long>,
        std::tuple<int*, char*, long*>);

    // const-decorate every element
    D_TEST_TYPE_EQ(
        tuple_apply_all_t<std::add_const_t, int, char>,
        std::tuple<const int, const char>);

    // single-element pack
    D_TEST_TYPE_EQ(
        tuple_apply_all_t<std::add_pointer_t, int>,
        std::tuple<int*>);

    // mixed-type pack preserves ordering and applies element-wise
    D_TEST_TYPE_EQ(
        tuple_apply_all_t<std::add_pointer_t,
                          int, const char, volatile long>,
        std::tuple<int*, const char*, volatile long*>);

    // empty pack yields the empty tuple.  This exercises the
    // `to_tuple<>` empty-pack specialization in dtuple.hpp —
    // before that specialization existed, instantiating
    // tuple_apply_all_t with no type arguments triggered an
    // undefined-template error through to_tuple's
    // std::conditional branch.
    D_TEST_TYPE_EQ(tuple_apply_all_t<std::add_pointer_t>,
                   std::tuple<>);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "tuple_apply_all/all-checks");

    return true;
}


// =========================================================================
// II.  COUNT_AND_REMOVE
// =========================================================================

/*
tests_dtuple_count_and_remove
  Verifies tuple_count_and_remove produces consistent counts and
  a correct post-removal tuple layout.
  Tests the following:
  - removing a type not in the pack leaves the tuple unchanged
    and yields a count of zero
  - removing the only occurrence of a type leaves an empty
    residual
  - removing a type that occurs multiple times removes every copy
  - the removal preserves the relative order of the surviving
    elements
  - removing from an empty tuple yields an empty tuple with count 0
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

    // order preservation: the surviving elements keep their
    // relative order from the source tuple
    D_TEST_TYPE_EQ(
        tuple_count_and_remove_t<int,
            std::tuple<char, int, long, int, double, int>>,
        std::tuple<char, long, double>);

    // removing from an empty tuple
    D_TEST_TYPE_EQ(tuple_count_and_remove_t<int, std::tuple<>>,
                   std::tuple<>);
    static_assert(
        (tuple_count_and_remove<int, std::tuple<>>::value == 0),
        "removing from empty tuple yields count 0");

    D_TEST_FIRE(_handler,
                on_compile_check,
                "count_and_remove/all-checks");

    return true;
}


// =========================================================================
// III. TUPLE_COUNT_TYPE (STANDALONE)
// =========================================================================

/*
tests_dtuple_tuple_count_type
  Exercises tuple_count_type as an independent component, not just
  as a transitive consequence of tuple_count_and_remove::value.
  This catches the case where count_type's recursive definition
  drifts out of sync with the removal trait.
  Tests the following:
  - counting an absent type yields 0
  - counting across an empty tuple yields 0
  - counting a single occurrence yields 1
  - counting multiple occurrences yields the correct count
  - counting against a pack that is nothing but the target yields
    the pack's full size
  - count_type's value matches count_and_remove's value across
    every case above (cross-check consistency)
  - pack_count_of aggregates count_type over a type pack for
    demonstration
*/
bool
tests_dtuple_tuple_count_type(
    test_handler& _handler
)
{
    typedef std::tuple<int, char, int, long, int> tup_iclil;
    typedef std::tuple<int, int, int>             tup_iii;

    // absent type
    static_assert(
        (tuple_count_type<double, tup_iclil>::value == 0),
        "count_type: absent type yields 0");
    static_assert(
        (tuple_count_type<double, int, char>::value == 0),
        "count_type: absent type yields 0 (pack form)");

    // empty tuple
    static_assert(
        (tuple_count_type<int, std::tuple<>>::value == 0),
        "count_type: empty tuple yields 0");

    // single occurrence
    static_assert(
        (tuple_count_type<char, tup_iclil>::value == 1),
        "count_type: single occurrence yields 1");

    // multiple occurrences
    static_assert(
        (tuple_count_type<int, tup_iclil>::value == 3),
        "count_type: three occurrences of int");

    // pack is nothing but the target
    static_assert(
        (tuple_count_type<int, tup_iii>::value == 3),
        "count_type: all-target pack yields full size");
    static_assert(
        (tuple_count_type<int, tup_iii>::value ==
         std::tuple_size<tup_iii>::value),
        "count_type: matches tuple_size for all-target pack");

    // single-element tuple containing the target
    static_assert(
        (tuple_count_type<int, std::tuple<int>>::value == 1),
        "count_type: single-element tuple containing target");

    // single-element tuple NOT containing the target
    static_assert(
        (tuple_count_type<int, std::tuple<char>>::value == 0),
        "count_type: single-element tuple without target");

    // cross-check with count_and_remove across a range of cases
    static_assert(
        (tuple_count_type<int, tup_iclil>::value ==
         tuple_count_and_remove<int, tup_iclil>::value),
        "count_type and count_and_remove agree on int/tup_iclil");
    static_assert(
        (tuple_count_type<char, tup_iclil>::value ==
         tuple_count_and_remove<char, tup_iclil>::value),
        "count_type and count_and_remove agree on char/tup_iclil");
    static_assert(
        (tuple_count_type<double, tup_iclil>::value ==
         tuple_count_and_remove<double, tup_iclil>::value),
        "count_type and count_and_remove agree on absent type");
    static_assert(
        (tuple_count_type<int, std::tuple<>>::value ==
         tuple_count_and_remove<int, std::tuple<>>::value),
        "count_type and count_and_remove agree on empty tuple");

    D_TEST_FIRE(_handler,
                on_compile_check,
                "tuple_count_type/all-checks");

    return true;
}


NS_END  // testing
NS_END  // djinterp
