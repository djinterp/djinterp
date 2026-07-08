/******************************************************************************
* djinterp [test]                                   foldable_tests_collect.cpp
*
*   Sections II.4 - II.6 of the foldable.hpp suite: the materialize-and-measure
* derivations.  fold_to_vector collects elements in fold order (covering the
* push_helper reducer); fold_length counts them (the count_helper reducer);
* fold_is_empty reports emptiness (the emptiness_helper reducer).  Covers empty,
* single, and multi-element inputs (including preserved duplicates and order), a
* class element type, a larger sequence, the non-std seq<T> fixture, and the
* cross-invariants tying the three together.
*
* path:      /tests/djinterp/core/functional/foldable_tests_collect.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


///////////////////////////////////////////////////////////////////////////////
///                fold_to_vector                                            ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_to_vector_basic
  Tests the following:
  - elements are collected in fold (left-to-right) order into a std::vector.
*/
static bool
tests_to_vector_basic()
{
    std::vector<int> xs;
    std::vector<int> out;
    std::vector<int> expected;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    expected.push_back(1);
    expected.push_back(2);
    expected.push_back(3);

    out = ::djinterp::fold_to_vector(xs);

    return (out == expected);
}

/*
tests_to_vector_empty
  Tests the following:
  - an empty foldable collects to an empty vector.
*/
static bool
tests_to_vector_empty()
{
    std::vector<int> xs;
    std::vector<int> out;

    out = ::djinterp::fold_to_vector(xs);

    return (out.empty());
}

/*
tests_to_vector_single
  Tests the following:
  - a one-element foldable collects to a one-element vector.
*/
static bool
tests_to_vector_single()
{
    std::vector<int> xs;
    std::vector<int> out;

    xs.push_back(42);

    out = ::djinterp::fold_to_vector(xs);

    return ( (out.size() == 1) &&
             (out[0] == 42) );
}

/*
tests_to_vector_preserves_duplicates_and_order
  Tests the following:
  - duplicates are kept and original order is preserved: [3,1,2,1] round-trips
    unchanged.
*/
static bool
tests_to_vector_preserves_duplicates_and_order()
{
    std::vector<int> xs;
    std::vector<int> out;
    std::vector<int> expected;

    xs.push_back(3);
    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(1);

    expected.push_back(3);
    expected.push_back(1);
    expected.push_back(2);
    expected.push_back(1);

    out = ::djinterp::fold_to_vector(xs);

    return (out == expected);
}

/*
tests_to_vector_class_elements
  Tests the following:
  - a non-scalar element type (std::string) is collected correctly.
*/
static bool
tests_to_vector_class_elements()
{
    std::vector<std::string> xs;
    std::vector<std::string> out;

    xs.push_back("a");
    xs.push_back("b");

    out = ::djinterp::fold_to_vector(xs);

    return ( (out.size() == 2) &&
             (out[0] == "a")   &&
             (out[1] == "b") );
}

/*
tests_to_vector_over_seq
  Tests the following:
  - the non-std seq<T> fixture materializes to a std::vector in order.
*/
static bool
tests_to_vector_over_seq()
{
    seq<int>         s;
    std::vector<int> out;
    std::vector<int> expected;

    s.data.push_back(5);
    s.data.push_back(6);
    s.data.push_back(7);

    expected.push_back(5);
    expected.push_back(6);
    expected.push_back(7);

    out = ::djinterp::fold_to_vector(s);

    return (out == expected);
}


///////////////////////////////////////////////////////////////////////////////
///                fold_length                                               ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_length_counts
  Tests the following:
  - the element count is 0 / 1 / 3 for empty / single / three-element foldables.
*/
static bool
tests_length_counts()
{
    std::vector<int> empty;
    std::vector<int> one;
    std::vector<int> three;

    one.push_back(7);

    three.push_back(1);
    three.push_back(2);
    three.push_back(3);

    return ( (::djinterp::fold_length(empty) == static_cast<std::size_t>(0)) &&
             (::djinterp::fold_length(one)   == static_cast<std::size_t>(1)) &&
             (::djinterp::fold_length(three) == static_cast<std::size_t>(3)) );
}

/*
tests_length_large
  Tests the following:
  - the count is exact for a larger sequence (100 elements).
*/
static bool
tests_length_large()
{
    std::vector<int> xs;
    int              i;

    for (i = 0; i < 100; ++i)
    {
        xs.push_back(i);
    }

    return (::djinterp::fold_length(xs) == static_cast<std::size_t>(100));
}

/*
tests_length_over_seq
  Tests the following:
  - fold_length counts the non-std seq<T> fixture (5 elements) and reports 0 for
    an empty seq<T>.
*/
static bool
tests_length_over_seq()
{
    seq<int> s;
    seq<int> empty;

    s.data.push_back(1);
    s.data.push_back(2);
    s.data.push_back(3);
    s.data.push_back(4);
    s.data.push_back(5);

    return ( (::djinterp::fold_length(s)     == static_cast<std::size_t>(5)) &&
             (::djinterp::fold_length(empty) == static_cast<std::size_t>(0)) );
}


///////////////////////////////////////////////////////////////////////////////
///                fold_is_empty                                             ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_is_empty_true_on_empty
  Tests the following:
  - fold_is_empty is true for a foldable with no elements.
*/
static bool
tests_is_empty_true_on_empty()
{
    std::vector<int> xs;

    return (::djinterp::fold_is_empty(xs));
}

/*
tests_is_empty_false_on_nonempty
  Tests the following:
  - fold_is_empty is false for one- and many-element foldables (the first
    element flips the accumulator).
*/
static bool
tests_is_empty_false_on_nonempty()
{
    std::vector<int> one;
    std::vector<int> many;

    one.push_back(1);

    many.push_back(1);
    many.push_back(2);
    many.push_back(3);

    return ( (!::djinterp::fold_is_empty(one)) &&
             (!::djinterp::fold_is_empty(many)) );
}

/*
tests_is_empty_over_seq
  Tests the following:
  - fold_is_empty is true for an empty seq<T> and false for a non-empty one.
*/
static bool
tests_is_empty_over_seq()
{
    seq<int> empty;
    seq<int> one;

    one.data.push_back(1);

    return ( (::djinterp::fold_is_empty(empty)) &&
             (!::djinterp::fold_is_empty(one)) );
}


///////////////////////////////////////////////////////////////////////////////
///                CROSS-INVARIANTS                                          ///
///////////////////////////////////////////////////////////////////////////////

/*
tests_collect_invariants
  Tests the following:
  - the three derivations agree: for empty, single, and multi-element inputs,
    fold_length(xs) == fold_to_vector(xs).size() and
    fold_is_empty(xs) == (fold_length(xs) == 0).
*/
static bool
tests_collect_invariants()
{
    std::vector<int> empty;
    std::vector<int> one;
    std::vector<int> many;
    bool             ok;

    one.push_back(9);

    many.push_back(4);
    many.push_back(5);
    many.push_back(6);
    many.push_back(7);

    ok = true;

    ok = ok &&
         (::djinterp::fold_length(empty) == ::djinterp::fold_to_vector(empty).size());
    ok = ok &&
         (::djinterp::fold_length(one)   == ::djinterp::fold_to_vector(one).size());
    ok = ok &&
         (::djinterp::fold_length(many)  == ::djinterp::fold_to_vector(many).size());

    ok = ok &&
         (::djinterp::fold_is_empty(empty) ==
          (::djinterp::fold_length(empty) == static_cast<std::size_t>(0)));
    ok = ok &&
         (::djinterp::fold_is_empty(one) ==
          (::djinterp::fold_length(one) == static_cast<std::size_t>(0)));
    ok = ok &&
         (::djinterp::fold_is_empty(many) ==
          (::djinterp::fold_length(many) == static_cast<std::size_t>(0)));

    return ok;
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
foldable_collect_block()
{
    dt::block_spec block;

    block.name       = "II.4-6 collect / length / is_empty";
    block.descriptor =
        "fold_to_vector, fold_length, fold_is_empty and their invariants";

    block.tests.push_back(dt::test_spec{
        "to_vector: basic order",
        "elements collected left-to-right",
        &tests_to_vector_basic });

    block.tests.push_back(dt::test_spec{
        "to_vector: empty",
        "empty foldable -> empty vector",
        &tests_to_vector_empty });

    block.tests.push_back(dt::test_spec{
        "to_vector: single",
        "one element -> one-element vector",
        &tests_to_vector_single });

    block.tests.push_back(dt::test_spec{
        "to_vector: duplicates & order",
        "[3,1,2,1] round-trips unchanged",
        &tests_to_vector_preserves_duplicates_and_order });

    block.tests.push_back(dt::test_spec{
        "to_vector: class elements",
        "std::string elements collected correctly",
        &tests_to_vector_class_elements });

    block.tests.push_back(dt::test_spec{
        "to_vector: over seq",
        "seq<T> materializes in order",
        &tests_to_vector_over_seq });

    block.tests.push_back(dt::test_spec{
        "length: 0 / 1 / 3",
        "empty, single, three-element counts",
        &tests_length_counts });

    block.tests.push_back(dt::test_spec{
        "length: large",
        "100-element count is exact",
        &tests_length_large });

    block.tests.push_back(dt::test_spec{
        "length: over seq",
        "seq<T> counted; empty seq is 0",
        &tests_length_over_seq });

    block.tests.push_back(dt::test_spec{
        "is_empty: true on empty",
        "no elements -> true",
        &tests_is_empty_true_on_empty });

    block.tests.push_back(dt::test_spec{
        "is_empty: false on non-empty",
        "one and many elements -> false",
        &tests_is_empty_false_on_nonempty });

    block.tests.push_back(dt::test_spec{
        "is_empty: over seq",
        "empty seq true, non-empty seq false",
        &tests_is_empty_over_seq });

    block.tests.push_back(dt::test_spec{
        "cross-invariants",
        "length == to_vector.size(); is_empty == (length==0)",
        &tests_collect_invariants });

    return block;
}


NS_END  // testing
NS_END  // djinterp
