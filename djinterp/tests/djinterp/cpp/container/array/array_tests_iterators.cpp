/*******************************************************************************
* djinterp [cpp][container]                            array_tests_iterators.cpp
*
*   Iterator tests for array_iterator.hpp: the DChunkRef view, the circular
* iterator, and the chunk and window iterators with their factories.
*
*
* path:      /tests/djinterp/cpp/container/array/array_tests_iterators.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.14
*                                                            revised: 2026.11.14
*******************************************************************************/

// std
#include <cstddef>      // std::size_t
#include <iterator>     // std::iterator_traits, std::distance
#include <numeric>      // std::accumulate
#include <type_traits>  // std::is_same
#include <vector>       // std::vector
// djinterp
#include "array_tests.hpp"  // suite header (normal face)


NS_DJINTERP
NS_TESTING

/*
tests_iterators_chunk_ref
  DChunkRef is a (data, size) view.
  Tests the following:
  - operator[] reads relative to data
  - [begin, end) spans size elements
  - empty() is size == 0
  - DWindowRef is the same type
*/
bool tests_iterators_chunk_ref()
{
    int buf[5] = { 1, 2, 3, 4, 5 };

    DChunkRef<int> r{ buf + 1, 3 };
    D_AY_CHECK(r[0] == 2 && r[2] == 4);
    D_AY_CHECK(r.end() - r.begin() == 3);
    D_AY_CHECK(r.begin() == buf + 1);
    D_AY_CHECK(!r.empty());
    D_AY_CHECK(std::accumulate(r.begin(), r.end(), 0) == 9);

    DChunkRef<int> e{ buf, 0 };
    D_AY_CHECK(e.empty() && e.begin() == e.end());

    D_AY_CHECK((std::is_same<DWindowRef<int>, DChunkRef<int>>::value));

    return true;
}

/*
tests_iterators_circular_traversal
  A circular iterator walks a ring from its head.
  Tests the following:
  - from head 2 of a 5-slot ring, [index 0, index 5) visits slots
    2, 3, 4, 0, 1
  - physical_index() and logical_index() along the way
  - capacity() is the ring size
  - a std::vector built from the range holds the rotated order
  - the category is random access
*/
bool tests_iterators_circular_traversal()
{
    const int ring[5] = { 10, 11, 12, 13, 14 };

    auto first = make_circular_iterator(ring, 5, 2, 0);
    auto last  = make_circular_iterator(ring, 5, 2, 5);

    const std::size_t physical[5] = { 2, 3, 4, 0, 1 };
    std::size_t       k           = 0;

    for (auto it = first; it != last; ++it, ++k)
    {
        D_AY_CHECK(it.logical_index() == k);
        D_AY_CHECK(it.physical_index() == physical[k]);
        D_AY_CHECK(*it == ring[physical[k]]);
    }

    D_AY_CHECK(k == 5);
    D_AY_CHECK(first.capacity() == 5);

    std::vector<int> v(first, last);
    D_AY_CHECK(v.size() == 5);
    D_AY_CHECK(v[0] == 12 && v[2] == 14 && v[3] == 10 && v[4] == 11);

    using category =
        std::iterator_traits<DCircularIterator<int>>::iterator_category;
    D_AY_CHECK((std::is_same<category,
                             std::random_access_iterator_tag>::value));

    return true;
}

/*
tests_iterators_circular_arithmetic
  The random-access operations.
  Tests the following:
  - += and -= move the logical index, and dereference wraps
  - it + n, n + it and it - n
  - the difference of two iterators, and std::distance
  - <, <=, >, >=, == and !=
  - operator[] relative to the current position
  - pre and post increment and decrement; operator->
*/
bool tests_iterators_circular_arithmetic()
{
    const int ring[5] = { 10, 11, 12, 13, 14 };

    DCircularIterator<int> it(ring, 5, 2, 0);

    it += 3;
    D_AY_CHECK(*it == 10);         // slot (2 + 3) % 5 = 0
    it -= 1;
    D_AY_CHECK(*it == 14);         // slot 4

    D_AY_CHECK(*(it + 2) == 11);   // logical 4 -> slot 1
    D_AY_CHECK(*(2 + it) == 11);
    D_AY_CHECK(*(it - 2) == 12);   // logical 0 -> slot 2

    DCircularIterator<int> b(ring, 5, 2, 0);
    DCircularIterator<int> e(ring, 5, 2, 5);
    D_AY_CHECK(e - b == 5);
    D_AY_CHECK(std::distance(b, e) == 5);
    D_AY_CHECK(b < e && b <= e && e > b && e >= b);
    D_AY_CHECK(b <= b && b >= b && !(b < b));
    D_AY_CHECK(b != e && b == DCircularIterator<int>(ring, 5, 2, 0));

    D_AY_CHECK(b[0] == 12 && b[3] == 10 && b[4] == 11);

    DCircularIterator<int> p = b++;
    D_AY_CHECK(*p == 12 && *b == 13);
    DCircularIterator<int> q = b--;
    D_AY_CHECK(*q == 13 && *b == 12);
    --e;
    D_AY_CHECK(*e == 11 && e.logical_index() == 4);
    D_AY_CHECK(e.operator->() == &ring[1]);

    return true;
}

/*
tests_iterators_chunk
  Chunks of n, the last one short.
  Tests the following:
  - 7 elements in chunks of 3: sizes 3, 3, 1, starting at 0, 3, 6
  - make_chunk_end is reached after the last chunk
  - 6 elements in chunks of 3: two full chunks
  - an empty range: no chunk
  - chunk_size(), base() and post-increment
*/
bool tests_iterators_chunk()
{
    const int seq[7] = { 0, 1, 2, 3, 4, 5, 6 };

    const std::size_t sizes[3]  = { 3, 3, 1 };
    const int         starts[3] = { 0, 3, 6 };
    std::size_t       n         = 0;

    auto first = make_chunk_iterator(seq, seq + 7, 3);
    auto last  = make_chunk_end(seq + 7, 3);

    for (auto it = first; it != last; ++it, ++n)
    {
        D_AY_CHECK(n < 3);
        D_AY_CHECK((*it).size == sizes[n]);
        D_AY_CHECK((*it)[0] == starts[n]);
    }

    D_AY_CHECK(n == 3);
    D_AY_CHECK(first.chunk_size() == 3 && first.base() == seq);

    auto six = make_chunk_iterator(seq, seq + 6, 3);
    D_AY_CHECK(std::distance(six, make_chunk_end(seq + 6, 3)) == 2);

    D_AY_CHECK(make_chunk_iterator(seq, seq, 3) == make_chunk_end(seq, 3));

    auto it  = first;
    auto old = it++;
    D_AY_CHECK(old.base() == seq && it.base() == seq + 3);

    return true;
}

/*
tests_iterators_window
  Full windows of w, sliding by one.
  Tests the following:
  - 5 elements, windows of 3: windows start at 0, 1, 2, each of size 3
  - a window as wide as the range: exactly one
  - windows of 1: one per element
  - window_size(), base() and post-increment
*/
bool tests_iterators_window()
{
    const int seq[5] = { 0, 1, 2, 3, 4 };

    std::size_t n = 0;

    auto first = make_window_iterator(seq, seq + 5, 3);
    auto last  = make_window_end(seq, seq + 5, 3);

    for (auto it = first; it != last; ++it, ++n)
    {
        D_AY_CHECK(n < 3);
        D_AY_CHECK((*it).size == 3);
        D_AY_CHECK((*it)[0] == static_cast<int>(n));
        D_AY_CHECK((*it)[2] == static_cast<int>(n) + 2);
    }

    D_AY_CHECK(n == 3);
    D_AY_CHECK(first.window_size() == 3 && first.base() == seq);

    auto whole = make_window_iterator(seq, seq + 5, 5);
    D_AY_CHECK(std::distance(whole, make_window_end(seq, seq + 5, 5)) == 1);
    D_AY_CHECK((*whole).size == 5);

    auto ones = make_window_iterator(seq, seq + 5, 1);
    D_AY_CHECK(std::distance(ones, make_window_end(seq, seq + 5, 1)) == 5);

    auto it  = first;
    auto old = it++;
    D_AY_CHECK(old.base() == seq && it.base() == seq + 1);

    return true;
}

/*
tests_iterators_window_shorter_than_range
  A range shorter than the window has no full window.
  Tests the following:
  - 2 elements, window 3: begin == end, so no window is yielded
  - an empty range, window 1: no window
  (Before 2026.11.14 the begin iterator stood at the first element, and the
  range yielded windows of 2 and 1.)
*/
bool tests_iterators_window_shorter_than_range()
{
    const int seq[2] = { 7, 8 };

    auto first = make_window_iterator(seq, seq + 2, 3);
    auto last  = make_window_end(seq, seq + 2, 3);
    D_AY_CHECK(first == last);

    std::size_t n = 0;

    for (auto it = first; it != last; ++it)
    {
        ++n;
    }

    D_AY_CHECK(n == 0);

    D_AY_CHECK(make_window_iterator(seq, seq, 1) ==
               make_window_end(seq, seq, 1));

    return true;
}

/*
tests_iterators_over_array
  Chunk and window iterators over an array<>.
  Tests the following:
  - chunks of 4 over six elements: sums 10 and 11
  - windows of 5 over six elements: sums 15 and 20
*/
bool tests_iterators_over_array()
{
    DMutableIterableArray<int, 6> a(1, 2, 3, 4, 5, 6);

    std::vector<int> chunk_sums;

    for (auto it  = make_chunk_iterator(a.begin(), a.end(), 4);
              it != make_chunk_end(a.end(), 4);
              ++it)
    {
        chunk_sums.push_back(
            std::accumulate((*it).begin(), (*it).end(), 0));
    }

    D_AY_CHECK(chunk_sums.size() == 2);
    D_AY_CHECK(chunk_sums[0] == 10 && chunk_sums[1] == 11);

    std::vector<int> window_sums;

    for (auto it  = make_window_iterator(a.begin(), a.end(), 5);
              it != make_window_end(a.begin(), a.end(), 5);
              ++it)
    {
        window_sums.push_back(
            std::accumulate((*it).begin(), (*it).end(), 0));
    }

    D_AY_CHECK(window_sums.size() == 2);
    D_AY_CHECK(window_sums[0] == 15 && window_sums[1] == 20);

    return true;
}

NS_END  // testing
NS_END  // djinterp
