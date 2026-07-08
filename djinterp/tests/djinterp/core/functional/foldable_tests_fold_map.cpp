/******************************************************************************
* djinterp [test]                                   foldable_tests_fold_map.cpp
*
*   Section II.3 of the foldable.hpp suite: fold_map, which maps each element to
* a monoid value and combines the images left-associated from an explicit
* identity.  Covers a numeric monoid (sum of squares), the identity returned
* unchanged on an empty foldable, a string-concatenation monoid, a mapping that
* changes the value type (element -> count, and a CSV join with a non-trivial
* combine), left-association shown by a non-commutative combine, a single
* element, and the same over the non-std seq<T> fixture.
*
* path:      /tests/djinterp/core/functional/foldable_tests_fold_map.cpp
* link(s):   TBA
* author(s): djinterp DTest suite                          created: 2026.07.07
******************************************************************************/

// djinterp
#include "foldable_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


/*
tests_fold_map_sum_of_squares
  Tests the following:
  - each element is mapped (x -> x*x) then combined (+) from identity 0:
    [1,2,3,4] gives 1+4+9+16 == 30.
*/
static bool
tests_fold_map_sum_of_squares()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);
    xs.push_back(4);

    result = ::djinterp::fold_map(
        xs,
        [](int _x) -> int
        {
            return _x * _x;
        },
        0,
        [](int _a, int _b) -> int
        {
            return _a + _b;
        });

    return (result == 30);
}

/*
tests_fold_map_empty_returns_identity
  Tests the following:
  - on an empty foldable the identity is returned unchanged, for both the zero
    identity and a non-zero one (7).
*/
static bool
tests_fold_map_empty_returns_identity()
{
    std::vector<int> xs;
    int              zero_identity;
    int              seven_identity;

    zero_identity = ::djinterp::fold_map(
        xs,
        [](int _x) -> int { return _x * _x; },
        0,
        [](int _a, int _b) -> int { return _a + _b; });

    seven_identity = ::djinterp::fold_map(
        xs,
        [](int _x) -> int { return _x * _x; },
        7,
        [](int _a, int _b) -> int { return _a + _b; });

    return ( (zero_identity == 0) &&
             (seven_identity == 7) );
}

/*
tests_fold_map_string_concat
  Tests the following:
  - a string monoid: mapping each digit to a one-char string and concatenating
    from "" yields "123", left-associated.
*/
static bool
tests_fold_map_string_concat()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_map(
        xs,
        [](int _x) -> std::string
        {
            return std::string(1, static_cast<char>('0' + _x));
        },
        std::string(),
        [](std::string _a, std::string _b) -> std::string
        {
            return _a + _b;
        });

    return (result == "123");
}

/*
tests_fold_map_length_via_const_map
  Tests the following:
  - mapping every element to 1 and summing computes the element count: four
    elements give 4 (fold_map can express fold_length).
*/
static bool
tests_fold_map_length_via_const_map()
{
    std::vector<int> xs;
    std::size_t      result;

    xs.push_back(9);
    xs.push_back(9);
    xs.push_back(9);
    xs.push_back(9);

    result = ::djinterp::fold_map(
        xs,
        [](int) -> std::size_t
        {
            return static_cast<std::size_t>(1);
        },
        static_cast<std::size_t>(0),
        [](std::size_t _a, std::size_t _b) -> std::size_t
        {
            return _a + _b;
        });

    return (result == static_cast<std::size_t>(4));
}

/*
tests_fold_map_csv_join
  Tests the following:
  - a non-trivial combine (comma-join that skips a leading separator) over a
    type-changing map yields "1,2,3", confirming combine sees the running
    accumulator as its LEFT operand.
*/
static bool
tests_fold_map_csv_join()
{
    std::vector<int> xs;
    std::string      result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_map(
        xs,
        [](int _x) -> std::string
        {
            return std::string(1, static_cast<char>('0' + _x));
        },
        std::string(),
        [](std::string _a, std::string _b) -> std::string
        {
            return _a.empty() ? _b : (_a + "," + _b);
        });

    return (result == "1,2,3");
}

/*
tests_fold_map_left_associated_noncommutative
  Tests the following:
  - association is to the LEFT with the identity as the first operand: identity
    map with subtraction combine over [1,2,3] gives ((0-1)-2)-3 == -6.
*/
static bool
tests_fold_map_left_associated_noncommutative()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(1);
    xs.push_back(2);
    xs.push_back(3);

    result = ::djinterp::fold_map(
        xs,
        [](int _x) -> int
        {
            return _x;
        },
        0,
        [](int _a, int _b) -> int
        {
            return _a - _b;
        });

    return (result == -6);
}

/*
tests_fold_map_single_element
  Tests the following:
  - a one-element foldable maps and combines once: x -> x+1 over [5] from 0
    gives 6.
*/
static bool
tests_fold_map_single_element()
{
    std::vector<int> xs;
    int              result;

    xs.push_back(5);

    result = ::djinterp::fold_map(
        xs,
        [](int _x) -> int
        {
            return _x + 1;
        },
        0,
        [](int _a, int _b) -> int
        {
            return _a + _b;
        });

    return (result == 6);
}

/*
tests_fold_map_over_seq
  Tests the following:
  - fold_map dispatches over the non-std seq<T> fixture: sum of squares of
    [1,2,3,4] is 30.
*/
static bool
tests_fold_map_over_seq()
{
    seq<int> s;
    int      result;

    s.data.push_back(1);
    s.data.push_back(2);
    s.data.push_back(3);
    s.data.push_back(4);

    result = ::djinterp::fold_map(
        s,
        [](int _x) -> int
        {
            return _x * _x;
        },
        0,
        [](int _a, int _b) -> int
        {
            return _a + _b;
        });

    return (result == 30);
}

/*
tests_fold_map_over_seq_empty
  Tests the following:
  - an empty seq<T> returns the identity (100) unchanged.
*/
static bool
tests_fold_map_over_seq_empty()
{
    seq<int> s;
    int      result;

    result = ::djinterp::fold_map(
        s,
        [](int _x) -> int
        {
            return _x * _x;
        },
        100,
        [](int _a, int _b) -> int
        {
            return _a + _b;
        });

    return (result == 100);
}


///////////////////////////////////////////////////////////////////////////////
///                BLOCK PROVIDER                                            ///
///////////////////////////////////////////////////////////////////////////////

dt::block_spec
foldable_fold_map_block()
{
    dt::block_spec block;

    block.name       = "II.3 fold_map";
    block.descriptor =
        "map each element then combine over an explicit monoid";

    block.tests.push_back(dt::test_spec{
        "sum of squares",
        "x->x*x then (+) from 0 gives 30",
        &tests_fold_map_sum_of_squares });

    block.tests.push_back(dt::test_spec{
        "empty returns identity",
        "identity (0 and 7) returned unchanged on empty",
        &tests_fold_map_empty_returns_identity });

    block.tests.push_back(dt::test_spec{
        "string concat monoid",
        "digit map + concat yields \"123\"",
        &tests_fold_map_string_concat });

    block.tests.push_back(dt::test_spec{
        "length via const map",
        "map->1 then (+) counts elements",
        &tests_fold_map_length_via_const_map });

    block.tests.push_back(dt::test_spec{
        "csv join (non-trivial combine)",
        "comma-join yields \"1,2,3\"; acc is left operand",
        &tests_fold_map_csv_join });

    block.tests.push_back(dt::test_spec{
        "left-associated combine",
        "identity map + subtraction gives -6",
        &tests_fold_map_left_associated_noncommutative });

    block.tests.push_back(dt::test_spec{
        "single element",
        "map + combine applied once yields 6",
        &tests_fold_map_single_element });

    block.tests.push_back(dt::test_spec{
        "over seq fixture",
        "fold_map dispatches over seq<T>",
        &tests_fold_map_over_seq });

    block.tests.push_back(dt::test_spec{
        "empty seq returns identity",
        "identity returned on empty seq<T>",
        &tests_fold_map_over_seq_empty });

    return block;
}


NS_END  // testing
NS_END  // djinterp
