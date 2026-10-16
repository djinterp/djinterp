/*******************************************************************************
* djinterp [core]                                       heap_sort_tests_sift.cpp
*
*
* path:      /tests/djinterp/core/util/sort/heap_sort_tests_sift.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
// djinterp [test]
#include "heap_sort_tests.hpp"


NS_DJINTERP
NS_TESTING


// tests_heap_sort_index_arithmetic
bool tests_heap_sort_index_arithmetic()
{
    verdict        result;
    std::ptrdiff_t index;
    std::ptrdiff_t left;
    std::ptrdiff_t right;

    //   The implicit-heap index maps, checked as a round trip rather than
    // as two separate formulas: the parent of either child of a node has to
    // be that node again, for every node in a useful range.
    for (index = 0; index < 1024; ++index)
    {
        left  = ::djinterp::internal::heap_left_child(index);
        right = (left + 1);

        result.check(left == ((2 * index) + 1));
        result.check(::djinterp::internal::heap_parent(left) == index);
        result.check(::djinterp::internal::heap_parent(right) == index);
    }

    //   The root is its own parent: (0 - 1) / 2 truncates toward zero in
    // C++, so heap_parent(0) is 0 rather than -1.  heap_sort_apply relies on
    // exactly that when it walks the heapify loop down to the root and stops
    // on i == 0.
    result.check(::djinterp::internal::heap_parent(
                     static_cast<std::ptrdiff_t>(0)) == 0);
    result.check(::djinterp::internal::heap_parent(
                     static_cast<std::ptrdiff_t>(1)) == 0);
    result.check(::djinterp::internal::heap_parent(
                     static_cast<std::ptrdiff_t>(2)) == 0);
    result.check(::djinterp::internal::heap_left_child(
                     static_cast<std::ptrdiff_t>(0)) == 1);

    return result.ok();
}


// tests_heap_sort_sift_down_leaf_exit
bool tests_heap_sort_sift_down_leaf_exit()
{
    verdict          result;
    std::vector<int> data;
    std::vector<int> original;

    //   A root whose left child is off the end is a leaf: the loop breaks on
    // its first test and nothing moves.  This is the exit every sift
    // eventually takes on the way down.
    data     = shape_descending(8);
    original = data;

    heap_sift_ints(data.begin(), 8, 4);
    heap_sift_ints(data.begin(), 8, 7);

    result.check(ranges_equal(data, original));

    //   The same exit reached because the heap has been shrunk out from
    // under the node: index 1 is an internal node of an 8-element heap but a
    // leaf of a 3-element one.
    data     = shape_descending(8);
    original = data;

    heap_sift_ints(data.begin(), 3, 1);

    result.check(ranges_equal(data, original));

    //   A single-element heap: the root is also a leaf.
    data.clear();
    data.push_back(5);

    heap_sift_ints(data.begin(), 1, 0);

    result.check(data.size() == 1);
    result.check(data[0] == 5);

    return result.ok();
}


// tests_heap_sort_sift_down_missing_right_child
bool tests_heap_sort_sift_down_missing_right_child()
{
    verdict          result;
    std::vector<int> data;

    //   A node with a left child but no right one.  The right-hand test is
    // guarded by (left + 1) < _size, and that guard short-circuits BEFORE
    // the comparator runs -- so this is the case that would read off the end
    // of the heap if the two operands were transposed.
    //
    //   Left child larger: it must be promoted.
    data.clear();
    data.push_back(1);
    data.push_back(9);

    heap_sift_ints(data.begin(), 2, 0);

    result.check(data[0] == 9);
    result.check(data[1] == 1);

    //   Left child smaller: nothing moves.
    data.clear();
    data.push_back(9);
    data.push_back(1);

    heap_sift_ints(data.begin(), 2, 0);

    result.check(data[0] == 9);
    result.check(data[1] == 1);

    //   An odd-sized heap has exactly one such node, and it is never the
    // root: in a 4-element heap it is index 1, whose left child is 3 and
    // whose right child would be 4.
    data.clear();
    data.push_back(9);
    data.push_back(2);
    data.push_back(8);
    data.push_back(7);

    heap_sift_ints(data.begin(), 4, 1);

    result.check(data[0] == 9);
    result.check(data[1] == 7);
    result.check(data[2] == 8);
    result.check(data[3] == 2);
    result.check(is_max_heap(data.begin(), data.end(), less<int>()));

    return result.ok();
}


// tests_heap_sort_sift_down_already_satisfied
bool tests_heap_sort_sift_down_already_satisfied()
{
    verdict                  result;
    std::vector<int>         data;
    std::vector<int>         original;
    std::vector<tracked_int> tracked;
    std::size_t              swaps;
    std::size_t              i;

    //   A root that already dominates both children takes the
    // `largest == current` exit -- the branch that is NOT the leaf exit and
    // NOT a descent.  Nothing may move.
    data.clear();
    data.push_back(9);
    data.push_back(4);
    data.push_back(6);
    data.push_back(1);
    data.push_back(3);
    data.push_back(5);
    data.push_back(2);
    original = data;

    heap_sift_ints(data.begin(), 7, 0);

    result.check(ranges_equal(data, original));

    //   Equivalent children take the same exit: the comparisons are strict,
    // so an equal child never displaces its parent.  Asserted through the
    // swap counter, because "nothing moved" and "moved and moved back" are
    // indistinguishable from the values alone.
    swaps = 0;

    tracked.reserve(7);

    for (i = 0; i < 7; ++i)
    {
        tracked.push_back(make_tracked(4, &swaps));
    }

    ::djinterp::internal::heap_sift_down(tracked.begin(),
                                         static_cast<std::ptrdiff_t>(7),
                                         static_cast<std::ptrdiff_t>(0),
                                         tracked_int_less());

    result.check(swaps == 0);

    return result.ok();
}


// tests_heap_sort_sift_down_cascade
bool tests_heap_sort_sift_down_cascade()
{
    verdict          result;
    std::vector<int> data;

    //   A root that has to travel more than one level: the loop must keep
    // descending from the position it was swapped into rather than stopping
    // after the first exchange.
    //
    //     start   1 8 7 6 5 4 3
    //     step 1  8 1 7 6 5 4 3   (1 trades with its larger child, 8)
    //     step 2  8 6 7 1 5 4 3   (1 trades again, with 6)
    //     step 3  index 3 is a leaf -- stop
    data.clear();
    data.push_back(1);
    data.push_back(8);
    data.push_back(7);
    data.push_back(6);
    data.push_back(5);
    data.push_back(4);
    data.push_back(3);

    heap_sift_ints(data.begin(), 7, 0);

    result.check(data[0] == 8);
    result.check(data[1] == 6);
    result.check(data[2] == 7);
    result.check(data[3] == 1);
    result.check(data[4] == 5);
    result.check(data[5] == 4);
    result.check(data[6] == 3);
    result.check(is_max_heap(data.begin(), data.end(), less<int>()));

    //   The RIGHT child winning is a separate branch from the left child
    // winning, and it only fires when the right child beats a left child
    // that has already been promoted.
    data.clear();
    data.push_back(1);
    data.push_back(2);
    data.push_back(9);

    heap_sift_ints(data.begin(), 3, 0);

    result.check(data[0] == 9);
    result.check(data[1] == 2);
    result.check(data[2] == 1);

    //   A full descent from the root of a larger heap, where every level
    // below the root is already correct: the minimum has to sink all the
    // way to a leaf.
    data.clear();
    data.push_back(0);
    data.push_back(14);
    data.push_back(13);
    data.push_back(12);
    data.push_back(11);
    data.push_back(10);
    data.push_back(9);
    data.push_back(8);
    data.push_back(7);
    data.push_back(6);
    data.push_back(5);
    data.push_back(4);
    data.push_back(3);
    data.push_back(2);
    data.push_back(1);

    heap_sift_ints(data.begin(), 15, 0);

    result.check(is_max_heap(data.begin(), data.end(), less<int>()));
    result.check(data[0] == 14);
    result.check(data[7] == 0);

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
