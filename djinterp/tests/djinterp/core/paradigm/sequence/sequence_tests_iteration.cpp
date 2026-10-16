/*******************************************************************************
* djinterp [core]                                   sequence_tests_iteration.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_iteration.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_iteration.cpp
//   Iteration: begin / end in both forms, plus cbegin / cend. The iterators are
// the underlying vector's, so the point of these tests is that the sequence
// exposes them correctly - that the const overloads really are const, that
// range-based for works (it needs begin/end found on the type), that writing
// through a mutable iterator reaches the stored elements, and that the standard
// algorithms accept them.

// std
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <type_traits>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_begin_end
  Verifies forward traversal with the mutable iterators.
  Tests the following:
  - the half-open range covers every element in order
  - the distance between begin and end is the size
*/
bool
tests_begin_end()
{
    int_sequence s{ 1, 2, 3 };

    int sum   = 0;
    int first = 0;
    for (int_sequence::iterator it = s.begin(); it != s.end(); ++it)
    {
        if (it == s.begin()) { first = *it; }
        sum += *it;
    }

    bool ok = true;

    ok = ok && (sum == 6);
    ok = ok && (first == 1);
    ok = ok && (std::distance(s.begin(), s.end()) ==
                static_cast<int_sequence::difference_type>(s.size()));

    return ok;
}

/*
tests_begin_end_const
  Verifies traversal through a const reference.
  Tests the following:
  - the const overloads are selected, yielding const_iterators
  - the range still covers every element
*/
bool
tests_begin_end_const()
{
    int_sequence        mutable_seq{ 1, 2, 3 };
    const int_sequence& s = mutable_seq;

    static_assert(std::is_same<decltype(s.begin()),
                               int_sequence::const_iterator>::value,
                  "const begin yields a const_iterator");
    static_assert(std::is_same<decltype(s.end()),
                               int_sequence::const_iterator>::value,
                  "const end yields a const_iterator");

    int sum = 0;
    for (int_sequence::const_iterator it = s.begin(); it != s.end(); ++it)
    {
        sum += *it;
    }

    return (sum == 6);
}

/*
tests_cbegin_cend
  Verifies the explicitly-const iterator accessors.
  Tests the following:
  - cbegin / cend yield const_iterators even from a mutable sequence
  - the range covers every element
*/
bool
tests_cbegin_cend()
{
    int_sequence s{ 1, 2, 3 };

    static_assert(std::is_same<decltype(s.cbegin()),
                               int_sequence::const_iterator>::value,
                  "cbegin yields a const_iterator");
    static_assert(std::is_same<decltype(s.cend()),
                               int_sequence::const_iterator>::value,
                  "cend yields a const_iterator");

    int sum = 0;
    for (int_sequence::const_iterator it = s.cbegin(); it != s.cend(); ++it)
    {
        sum += *it;
    }

    bool ok = true;

    ok = ok && (sum == 6);
    ok = ok && (std::distance(s.cbegin(), s.cend()) == 3);

    return ok;
}

/*
tests_range_for
  Verifies range-based for.
  Tests the following:
  - a by-value range-for visits every element in order
  - a by-const-reference range-for does too
*/
bool
tests_range_for()
{
    int_sequence s{ 4, 5, 6 };

    int sum   = 0;
    int first = 0;
    for (int v : s)
    {
        if (sum == 0) { first = v; }
        sum += v;
    }

    int const_sum = 0;
    for (const int& v : s)
    {
        const_sum += v;
    }

    bool ok = true;

    ok = ok && (sum == 15);
    ok = ok && (first == 4);
    ok = ok && (const_sum == 15);

    return ok;
}

/*
tests_iterator_mutates
  Verifies writing through a mutable iterator.
  Tests the following:
  - assigning through a dereferenced iterator changes the stored element
  - a mutating range-for reaches every element
*/
bool
tests_iterator_mutates()
{
    int_sequence s{ 1, 2, 3 };

    *s.begin() = 99;

    bool ok = (matches(s, { 99, 2, 3 }));

    for (int& v : s)
    {
        v *= 2;
    }

    ok = ok && (matches(s, { 198, 4, 6 }));

    return ok;
}

/*
tests_iterator_std_algorithms
  Verifies the iterators satisfy the standard algorithms.
  Tests the following:
  - accumulate sums the range
  - find locates a present element and reports end() for an absent one
  - sort reorders the sequence in place
*/
bool
tests_iterator_std_algorithms()
{
    int_sequence s{ 5, 3, 8, 1 };

    bool ok = true;

    ok = ok && (std::accumulate(s.begin(), s.end(), 0) == 17);
    ok = ok && (std::find(s.begin(), s.end(), 8) != s.end());
    ok = ok && (std::find(s.begin(), s.end(), 99) == s.end());

    std::sort(s.begin(), s.end());
    ok = ok && (matches(s, { 1, 3, 5, 8 }));

    return ok;
}

/*
tests_iteration_empty
  Verifies iteration over an empty sequence.
  Tests the following:
  - begin equals end, so a range-for body never runs
  - the same holds for the const and cbegin/cend forms
*/
bool
tests_iteration_empty()
{
    int_sequence        s;
    const int_sequence& cs = s;

    int visits = 0;
    for (int v : s)
    {
        (void)v;
        ++visits;
    }

    bool ok = true;

    ok = ok && (visits == 0);
    ok = ok && (s.begin() == s.end());
    ok = ok && (cs.begin() == cs.end());
    ok = ok && (s.cbegin() == s.cend());

    return ok;
}


NS_END  // testing
NS_END  // djinterp
