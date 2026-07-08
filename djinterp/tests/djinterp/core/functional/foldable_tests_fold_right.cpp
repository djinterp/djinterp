/******************************************************************************
* djinterp [test]                                 foldable_tests_fold_right.cpp
*
*   Section II.2 of the foldable.hpp suite: fold_right, derived by materializing
* the foldable (fold_to_vector) and folding the buffer in reverse.  Covers the
* empty and non-empty cases, RIGHT association (distinguished from the left fold
* by a non-commutative reducer), the reduction-order the reversed walk implies
* (prepend preserves original order; append reverses it), an accumulator whose
* type differs from the element type, rvalue-init forwarding, and the same fold
* over the non-std seq<T> fixture.
*
* path:      /tests/djinterp/core/functional/foldable_tests_fold_right.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_fold_right_sum
  Tests the following:
  - a commutative reducer over [1,2,3,4] from 0 sums to 10 (matching the left
    fold, as it must for (+)).
*/
static bool
tests_fold_right_sum()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    result = ::djinterp::fold_right(
        xs,
        0,
        [](int _x, int _acc) -> int
        {
            return _x + _acc;
        });

    return (result == 10);
}

/*
tests_fold_right_empty
  Tests the following:
  - an empty foldable returns the initial accumulator untouched (nothing to
    materialize, no reduction steps).
*/
static bool
tests_fold_right_empty()
{
    std::vector<int> xs;
    int              result;

    result = ::djinterp::fold_right(
        xs,
        42,
        [](int _x, int _acc) -> int
        {
            return _x + _acc;
        });

    return (result == 42);
}

/*
tests_fold_right_single
  Tests the following:
  - a one-element foldable applies the reducer once, as f(x, init).
*/
static bool
tests_fold_right_single()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(7);

    result = ::djinterp::fold_right(
        xs,
        0,
        [](int _x, int _acc) -> int
        {
            return _x + _acc;
        });

    return (result == 7);
}

/*
tests_fold_right_right_associative
  Tests the following:
  - association is to the RIGHT: subtraction over [1,2,3] from 0 gives
    1-(2-(3-0)) == 2, distinct from the left fold's -6.
*/
static bool
tests_fold_right_right_associative()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_right(
        xs,
        0,
        [](int _x, int _acc) -> int
        {
            return _x - _acc;
        });

    return (result == 2);
}

/*
tests_fold_right_prepend_preserves_order
  Tests the following:
  - the reversed walk visits elements last-to-first, so PREPENDING each digit to
    the accumulator reconstructs the original order: [1,2,3] yields "123".
*/
static bool
tests_fold_right_prepend_preserves_order()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_right(
        xs,
        std::string(),
        [](int _x, std::string _acc) -> std::string
        {
            return std::string(1, static_cast<char>('0' + _x)) + _acc;
        });

    return (result == "123");
}

/*
tests_fold_right_append_reverses_order
  Tests the following:
  - APPENDING each digit to the accumulator during the last-to-first walk yields
    the reversed string "321", exhibiting the right-to-left visitation directly.
*/
static bool
tests_fold_right_append_reverses_order()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_right(
        xs,
        std::string(),
        [](int _x, std::string _acc) -> std::string
        {
            return _acc + std::string(1, static_cast<char>('0' + _x));
        });

    return (result == "321");
}

/*
tests_fold_right_acc_type_differs
  Tests the following:
  - the accumulator type may differ from the element type: prepending ints into
    a std::vector<int> during the reversed walk rebuilds [1,2,3] in order.
*/
static bool
tests_fold_right_acc_type_differs()
{
    std::vector<int> xs;
    std::vector<int> out;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    out = ::djinterp::fold_right(
        xs,
        std::vector<int>(),
        [](int _x, std::vector<int> _acc) -> std::vector<int>
        {
            _acc.insert(_acc.begin(), _x);

            return _acc;
        });

    return ( (out.size() == 3) &&
             (out[0] == 1)     &&
             (out[1] == 2)     &&
             (out[2] == 3) );
}

/*
tests_fold_right_rvalue_init
  Tests the following:
  - an rvalue initial accumulator is forwarded into the fold: prepending onto a
    temporary "!" seed yields "123!".
*/
static bool
tests_fold_right_rvalue_init()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_right(
        xs,
        std::string("!"),
        [](int _x, std::string _acc) -> std::string
        {
            return std::string(1, static_cast<char>('0' + _x)) + _acc;
        });

    return (result == "123!");
}

/*
tests_fold_right_over_seq
  Tests the following:
  - fold_right materializes and folds the non-std seq<T> fixture: [2,4,6] from 0
    sums to 12.
*/
static bool
tests_fold_right_over_seq()
{
    seq<int> s;
    int      result;

    s.data.push_back(2);
    s.data.push_back(4);
    s.data.push_back(6);

    result = ::djinterp::fold_right(
        s,
        0,
        [](int _x, int _acc) -> int
        {
            return _x + _acc;
        });

    return (result == 12);
}

/*
tests_fold_right_over_seq_empty
  Tests the following:
  - an empty seq<T> yields the initial accumulator.
*/
static bool
tests_fold_right_over_seq_empty()
{
    seq<int> s;
    int      result;

    result = ::djinterp::fold_right(
        s,
        5,
        [](int _x, int _acc) -> int
        {
            return _x + _acc;
        });

    return (result == 5);
}

/*
tests_fold_right_matches_left_commutative
  Tests the following:
  - for a commutative, associative reducer the two folds agree: fold_left and
    fold_right of (+) over [5,5,5] are both 15.
*/
static bool
tests_fold_right_matches_left_commutative()
{
    std::vector<int> xs;
    int              right;
    int              left;

    xs.push_back(5);
    xs.push_back(5);
    xs.push_back(5);

    right = ::djinterp::fold_right(
        xs,
        0,
        [](int _x, int _acc) -> int
        {
            return _x + _acc;
        });

    left = ::djinterp::fold_left(
        xs,
        0,
        [](int _acc, int _x) -> int
        {
            return _acc + _x;
        });

    return ( (right == 15) &&
             (left == 15)  &&
             (right == left) );
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
foldable_fold_right_block()
{
    dt::block_spec block;

    block.name       = "II.2 fold_right";
    block.descriptor =
        "right fold via materialize + reverse: association, order, rvalue init";

    block.tests.push_back(dt::test_spec{
        "sum over vector",
        "commutative reducer sums to 10",
        &tests_fold_right_sum });

    block.tests.push_back(dt::test_spec{
        "empty returns init",
        "no elements: initial accumulator returned",
        &tests_fold_right_empty });

    block.tests.push_back(dt::test_spec{
        "single element",
        "reducer applied once as f(x, init)",
        &tests_fold_right_single });

    block.tests.push_back(dt::test_spec{
        "right-associative order",
        "subtraction gives 1-(2-(3-0)) == 2",
        &tests_fold_right_right_associative });

    block.tests.push_back(dt::test_spec{
        "prepend preserves order",
        "reversed walk + prepend reconstructs \"123\"",
        &tests_fold_right_prepend_preserves_order });

    block.tests.push_back(dt::test_spec{
        "append reverses order",
        "reversed walk + append yields \"321\"",
        &tests_fold_right_append_reverses_order });

    block.tests.push_back(dt::test_spec{
        "accumulator type differs",
        "prepend ints into a vector, rebuild [1,2,3]",
        &tests_fold_right_acc_type_differs });

    block.tests.push_back(dt::test_spec{
        "rvalue init forwarded",
        "temporary seed \"!\" yields \"123!\"",
        &tests_fold_right_rvalue_init });

    block.tests.push_back(dt::test_spec{
        "over seq fixture",
        "materialize + fold seq<T> sums to 12",
        &tests_fold_right_over_seq });

    block.tests.push_back(dt::test_spec{
        "empty seq returns init",
        "no elements over seq<T>",
        &tests_fold_right_over_seq_empty });

    block.tests.push_back(dt::test_spec{
        "agrees with left (commutative)",
        "fold_left == fold_right for (+) over [5,5,5]",
        &tests_fold_right_matches_left_commutative });

    return block;
}


NS_END  // testing
NS_END  // djinterp
