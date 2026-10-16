/*******************************************************************************
* djinterp [core]                                 sequence_tests_subsequence.cpp
*
*
* path:      /tests/djinterp/core/paradigm/sequence/sequence_tests_subsequence.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp [test] : sequence_tests_subsequence.cpp
//   Subsequence extraction: sub, head and tail. All three return a NEW sequence
// (head and tail are defined in terms of sub, which copies out of the source's
// contiguous storage), so the source is never disturbed - that independence is
// pinned explicitly below.
//
//   The boundary cases are the ones worth stating: a zero-length request is
// legal anywhere in range, including at the very end (sub(size(), 0) forms a
// one-past-the-end pointer, which is valid to compute and compare), and both
// head(size()) and tail(size()) return the whole sequence.

// std
#include <cstddef>
// djinterp
#include "sequence_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_sub
  Verifies extraction of an interior range.
  Tests the following:
  - sub(offset, count) yields the half-open range [offset, offset+count)
  - a range starting at zero and one ending at the last element both work
*/
bool
tests_sub()
{
    int_sequence s{ 1, 2, 3, 4, 5 };

    bool ok = true;

    ok = ok && (matches(s.sub(1, 3), { 2, 3, 4 }));   // interior
    ok = ok && (matches(s.sub(0, 2), { 1, 2 }));      // from the start
    ok = ok && (matches(s.sub(3, 2), { 4, 5 }));      // to the end
    ok = ok && (matches(s.sub(2, 1), { 3 }));         // a single element

    return ok;
}

/*
tests_sub_zero_count
  Verifies a zero-length request.
  Tests the following:
  - sub(offset, 0) yields an empty sequence, wherever the offset sits
*/
bool
tests_sub_zero_count()
{
    int_sequence s{ 1, 2, 3 };

    bool ok = true;

    ok = ok && (s.sub(0, 0).empty());
    ok = ok && (s.sub(1, 0).empty());
    ok = ok && (s.sub(2, 0).empty());

    return ok;
}

/*
tests_sub_full
  Verifies extraction of the whole sequence.
  Tests the following:
  - sub(0, size()) reproduces the source
*/
bool
tests_sub_full()
{
    int_sequence s{ 1, 2, 3, 4 };

    int_sequence whole = s.sub(0, s.size());

    bool ok = true;

    ok = ok && (matches(whole, { 1, 2, 3, 4 }));
    ok = ok && (whole == s);

    return ok;
}

/*
tests_sub_at_end
  Verifies a zero-length request positioned at the very end.
  Tests the following:
  - sub(size(), 0) is legal and yields an empty sequence
  - the same holds on an empty sequence, where the storage pointer may be null
*/
bool
tests_sub_at_end()
{
    int_sequence s{ 1, 2, 3 };

    int_sequence at_end = s.sub(s.size(), 0);

    bool ok = (at_end.empty());

    int_sequence none;
    int_sequence from_empty = none.sub(0, 0);

    ok = ok && (from_empty.empty());

    return ok;
}

/*
tests_head
  Verifies extraction of the leading elements.
  Tests the following:
  - head(count) yields the first count elements, in order
*/
bool
tests_head()
{
    int_sequence s{ 1, 2, 3, 4, 5 };

    bool ok = true;

    ok = ok && (matches(s.head(2), { 1, 2 }));
    ok = ok && (matches(s.head(1), { 1 }));
    ok = ok && (matches(s.head(4), { 1, 2, 3, 4 }));

    return ok;
}

/*
tests_head_zero
  Verifies a zero-length head.
  Tests the following:
  - head(0) yields an empty sequence, including from an empty source
*/
bool
tests_head_zero()
{
    int_sequence s{ 1, 2, 3 };
    int_sequence none;

    bool ok = true;

    ok = ok && (s.head(0).empty());
    ok = ok && (none.head(0).empty());

    return ok;
}

/*
tests_head_full
  Verifies a head spanning the whole sequence.
  Tests the following:
  - head(size()) reproduces the source
*/
bool
tests_head_full()
{
    int_sequence s{ 1, 2, 3 };

    int_sequence whole = s.head(s.size());

    return (whole == s && matches(whole, { 1, 2, 3 }));
}

/*
tests_tail
  Verifies extraction of the trailing elements.
  Tests the following:
  - tail(count) yields the LAST count elements, in order
*/
bool
tests_tail()
{
    int_sequence s{ 1, 2, 3, 4, 5 };

    bool ok = true;

    ok = ok && (matches(s.tail(2), { 4, 5 }));
    ok = ok && (matches(s.tail(1), { 5 }));
    ok = ok && (matches(s.tail(4), { 2, 3, 4, 5 }));

    return ok;
}

/*
tests_tail_zero
  Verifies a zero-length tail.
  Tests the following:
  - tail(0) yields an empty sequence - the offset it computes lands exactly one
    past the last element, which is a valid position to form
  - the same holds from an empty source
*/
bool
tests_tail_zero()
{
    int_sequence s{ 1, 2, 3 };
    int_sequence none;

    bool ok = true;

    ok = ok && (s.tail(0).empty());
    ok = ok && (none.tail(0).empty());

    return ok;
}

/*
tests_tail_full
  Verifies a tail spanning the whole sequence.
  Tests the following:
  - tail(size()) reproduces the source
*/
bool
tests_tail_full()
{
    int_sequence s{ 1, 2, 3 };

    int_sequence whole = s.tail(s.size());

    return (whole == s && matches(whole, { 1, 2, 3 }));
}

/*
tests_subsequence_is_a_copy
  Verifies the extracted sequence is an independent copy.
  Tests the following:
  - mutating an extracted subsequence does not disturb the source
  - mutating the source does not disturb a previously extracted subsequence
*/
bool
tests_subsequence_is_a_copy()
{
    int_sequence source{ 1, 2, 3, 4 };

    int_sequence part = source.sub(1, 2);   // {2, 3}

    part[0] = 99;
    part.push_back(100);

    bool ok = true;

    ok = ok && (matches(source, { 1, 2, 3, 4 }));   // source untouched
    ok = ok && (matches(part, { 99, 3, 100 }));

    source[1] = 55;
    ok = ok && (matches(part, { 99, 3, 100 }));     // the copy is untouched

    return ok;
}


NS_END  // testing
NS_END  // djinterp
