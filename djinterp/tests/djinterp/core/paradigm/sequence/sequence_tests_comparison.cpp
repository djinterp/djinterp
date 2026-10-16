/*******************************************************************************
* djinterp [core]                                  sequence_tests_comparison.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_comparison.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_comparison.cpp
//   Comparison: the friend operator== (same size AND element-wise equal) and
// operator!=, which is defined as its negation. Both size mismatch and element
// mismatch are checked separately, since the implementation short-circuits on
// size before ever touching an element.

// std
#include <cstddef>
#include <string>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_equality
  Verifies equality of two sequences with the same contents.
  Tests the following:
  - equal contents compare equal, and not unequal
  - the same holds for a class element type
  - order matters: a permutation is not equal
*/
bool
tests_equality()
{
    int_sequence a{ 1, 2, 3 };
    int_sequence b{ 1, 2, 3 };

    bool ok = true;

    ok = ok && (a == b);
    ok = ok && (!(a != b));

    string_sequence sa(std::string("x"), std::string("y"));
    string_sequence sb(std::string("x"), std::string("y"));

    ok = ok && (sa == sb);

    int_sequence permuted{ 3, 2, 1 };
    ok = ok && (!(a == permuted));         // order is part of the value

    return ok;
}

/*
tests_equality_self
  Verifies a sequence equals itself.
  Tests the following:
  - comparison with itself is true, for both a populated and an empty sequence
*/
bool
tests_equality_self()
{
    int_sequence s{ 1, 2, 3 };
    int_sequence none;

    bool ok = true;

    ok = ok && (s == s);
    ok = ok && (!(s != s));
    ok = ok && (none == none);

    return ok;
}

/*
tests_equality_different_size
  Verifies a size mismatch compares unequal - the short-circuit path, taken
  before any element is examined.
  Tests the following:
  - a prefix is not equal to the longer sequence it prefixes
  - an empty sequence is not equal to a populated one, in either order
*/
bool
tests_equality_different_size()
{
    int_sequence three{ 1, 2, 3 };
    int_sequence two{ 1, 2 };
    int_sequence none;

    bool ok = true;

    ok = ok && (!(three == two));          // a common prefix is not enough
    ok = ok && (three != two);
    ok = ok && (!(none == three));
    ok = ok && (!(three == none));

    return ok;
}

/*
tests_equality_different_elements
  Verifies an element mismatch compares unequal - the element-wise path, with
  the sizes agreeing.
  Tests the following:
  - a difference in the last element is detected
  - a difference in the first element is detected
*/
bool
tests_equality_different_elements()
{
    int_sequence a{ 1, 2, 3 };
    int_sequence last_differs{ 1, 2, 4 };
    int_sequence first_differs{ 9, 2, 3 };

    bool ok = true;

    ok = ok && (!(a == last_differs));
    ok = ok && (a != last_differs);
    ok = ok && (!(a == first_differs));
    ok = ok && (a != first_differs);

    return ok;
}

/*
tests_equality_empty
  Verifies comparison of empty sequences.
  Tests the following:
  - two empty sequences are equal
  - a cleared sequence equals a never-populated one
*/
bool
tests_equality_empty()
{
    int_sequence a;
    int_sequence b;

    bool ok = true;

    ok = ok && (a == b);
    ok = ok && (!(a != b));

    int_sequence cleared{ 1, 2, 3 };
    cleared.clear();

    ok = ok && (cleared == a);

    return ok;
}

/*
tests_inequality
  Verifies operator!= is exactly the negation of operator==.
  Tests the following:
  - for every pairing, (a != b) equals !(a == b)
*/
bool
tests_inequality()
{
    int_sequence a{ 1, 2, 3 };
    int_sequence same{ 1, 2, 3 };
    int_sequence shorter{ 1, 2 };
    int_sequence differing{ 1, 2, 4 };
    int_sequence none;

    bool ok = true;

    ok = ok && ((a != same)      == !(a == same));
    ok = ok && ((a != shorter)   == !(a == shorter));
    ok = ok && ((a != differing) == !(a == differing));
    ok = ok && ((a != none)      == !(a == none));

    return ok;
}


NS_END  // testing
NS_END  // djinterp
