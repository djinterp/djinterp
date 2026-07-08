/******************************************************************************
* djinterp [test]                                  foldable_tests_fold_left.cpp
*
*   Section II.1 of the foldable.hpp suite: fold_left, the one obligation every
* other fold is derived from.  Covers the empty (zero-iteration) and non-empty
* loops of the std::vector instance, left-associative ordering, left-to-right
* visitation, accumulators whose type differs from the element type, the
* move-threaded collecting accumulator, const-lvalue and rvalue foldable
* arguments, and the same fold over the non-std seq<T> fixture.
*
* path:      /tests/djinterp/core/functional/foldable_tests_fold_left.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_fold_left_sum
  Tests the following:
  - the canonical case: (+) over a multi-element vector accumulates every
    element from the initial value.
*/
static bool
tests_fold_left_sum()
{
    std::vector<int> xs;
    int              sum;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    sum = ::djinterp::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (sum == 10);
}

/*
tests_fold_left_empty
  Tests the following:
  - the zero-iteration path: an empty foldable returns the initial accumulator
    untouched.
*/
static bool
tests_fold_left_empty()
{
    std::vector<int> xs;
    int              result;

    result = ::djinterp::fold_left(
        xs,
        42,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (result == 42);
}

/*
tests_fold_left_single
  Tests the following:
  - the single-iteration path: a one-element foldable applies the reducer once.
*/
static bool
tests_fold_left_single()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(7);

    result = ::djinterp::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (result == 7);
}

/*
tests_fold_left_left_associative
  Tests the following:
  - association is to the LEFT and visitation is in order: subtraction over
    [1,2,3] from 0 gives ((0-1)-2)-3 == -6, a value only the left fold yields.
*/
static bool
tests_fold_left_left_associative()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc - _x;
        });

    return (result == -6);
}

/*
tests_fold_left_order_string_append
  Tests the following:
  - left-to-right visitation, observed by appending each element's digit to an
    accumulating string: [1,2,3] yields "123".
*/
static bool
tests_fold_left_order_string_append()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_left(
        xs,
        std::string(),
        [](std::string _acc, int _x) -> std::string
        {
            _acc.push_back(static_cast<char>('0' + _x));

            return _acc;
        });

    return (result == "123");
}

/*
tests_fold_left_acc_type_differs
  Tests the following:
  - the accumulator type need not equal the element type: folding vector<int>
    into a std::string is well-formed and deduces the string result type.
*/
static bool
tests_fold_left_acc_type_differs()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);
    xs.push_back(5);

    result = ::djinterp::fold_left(
        xs,
        std::string(),
        [](std::string _acc, int _x) -> std::string
        {
            _acc.push_back(static_cast<char>('0' + _x));

            return _acc;
        });

    return (result == "12345");
}

/*
tests_fold_left_accumulate_into_vector
  Tests the following:
  - a collecting accumulator (a std::vector) is threaded correctly by move and
    grows in order; here each element is doubled as it is appended.
*/
static bool
tests_fold_left_accumulate_into_vector()
{
    std::vector<int> xs;
    std::vector<int> out;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    out = ::djinterp::fold_left(
        xs,
        std::vector<int>(),
        [](std::vector<int> _acc, int _x) -> std::vector<int>
        {
            _acc.push_back(_x * 2);

            return _acc;
        });

    return ( (out.size() == 3) &&
             (out[0] == 2)     &&
             (out[1] == 4)     &&
             (out[2] == 6) );
}

/*
tests_fold_left_over_seq
  Tests the following:
  - the generic fold_left dispatches over the non-std seq<T> fixture exactly as
    it does over std::vector.
*/
static bool
tests_fold_left_over_seq()
{
    seq<int> s;
    int      sum;

    s.data.push_back(5);
    s.data.push_back(10);
    s.data.push_back(15);

    sum = ::djinterp::fold_left(
        s,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (sum == 30);
}

/*
tests_fold_left_over_seq_empty
  Tests the following:
  - the zero-iteration path over the seq<T> fixture returns the initial value.
*/
static bool
tests_fold_left_over_seq_empty()
{
    seq<int> s;
    int      result;

    result = ::djinterp::fold_left(
        s,
        99,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (result == 99);
}

/*
tests_fold_left_const_lvalue
  Tests the following:
  - a const-lvalue foldable is accepted (the forwarding parameter decays before
    dispatch), and folds correctly.
*/
static bool
tests_fold_left_const_lvalue()
{
    const std::vector<int> xs = std::vector<int>({ 10, 20, 30 });
    int                    sum;

    sum = ::djinterp::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (sum == 60);
}

/*
tests_fold_left_rvalue_foldable
  Tests the following:
  - an rvalue foldable (a temporary vector) is accepted by the forwarding
    parameter and folds correctly.
*/
static bool
tests_fold_left_rvalue_foldable()
{
    int sum;

    sum = ::djinterp::fold_left(
        std::vector<int>({ 4, 5, 6 }),
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (sum == 15);
}

/*
tests_fold_left_product
  Tests the following:
  - a different reducer and initial value: the running product of [1,2,3,4]
    from 1 is 24.
*/
static bool
tests_fold_left_product()
{
    std::vector<int> xs;
    int              product;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    product = ::djinterp::fold_left(
        xs,
        1,
        [](int _acc, int _x) -> int
        {
            return _acc * _x;
        });

    return (product == 24);
}

/*
tests_fold_left_negatives
  Tests the following:
  - mixed-sign elements accumulate correctly: [-1, 2, -3, 4] sums to 2.
*/
static bool
tests_fold_left_negatives()
{
    std::vector<int> xs;
    int              sum;

    xs.push_back(-1);
    xs.push_back(2);
    xs.push_back(-3);
    xs.push_back(4);

    sum = ::djinterp::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return (sum == 2);
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
foldable_fold_left_block()
{
    dt::block_spec block;

    block.name       = "II.1 fold_left";
    block.descriptor =
        "strict left fold: delegation, ordering, accumulator threading, empties";

    block.tests.push_back(dt::test_spec{
        "sum over vector",
        "(+) accumulates all elements",
        &tests_fold_left_sum });

    block.tests.push_back(dt::test_spec{
        "empty returns init",
        "zero-iteration fold returns the initial accumulator",
        &tests_fold_left_empty });

    block.tests.push_back(dt::test_spec{
        "single element",
        "reducer applied exactly once",
        &tests_fold_left_single });

    block.tests.push_back(dt::test_spec{
        "left-associative order",
        "subtraction gives ((0-1)-2)-3 == -6",
        &tests_fold_left_left_associative });

    block.tests.push_back(dt::test_spec{
        "left-to-right visitation",
        "digit-append yields \"123\"",
        &tests_fold_left_order_string_append });

    block.tests.push_back(dt::test_spec{
        "accumulator type differs",
        "fold vector<int> into a std::string",
        &tests_fold_left_acc_type_differs });

    block.tests.push_back(dt::test_spec{
        "collect into vector",
        "move-threaded vector accumulator grows in order",
        &tests_fold_left_accumulate_into_vector });

    block.tests.push_back(dt::test_spec{
        "over seq fixture",
        "generic fold_left dispatches over seq<T>",
        &tests_fold_left_over_seq });

    block.tests.push_back(dt::test_spec{
        "empty seq returns init",
        "zero-iteration fold over seq<T>",
        &tests_fold_left_over_seq_empty });

    block.tests.push_back(dt::test_spec{
        "const-lvalue foldable",
        "const vector accepted and folded",
        &tests_fold_left_const_lvalue });

    block.tests.push_back(dt::test_spec{
        "rvalue foldable",
        "temporary vector accepted and folded",
        &tests_fold_left_rvalue_foldable });

    block.tests.push_back(dt::test_spec{
        "product reducer",
        "running product of [1,2,3,4] is 24",
        &tests_fold_left_product });

    block.tests.push_back(dt::test_spec{
        "mixed-sign elements",
        "[-1,2,-3,4] sums to 2",
        &tests_fold_left_negatives });

    return block;
}


NS_END  // testing
NS_END  // djinterp
