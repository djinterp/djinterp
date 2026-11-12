/*******************************************************************************
* djinterp [cpp]                                        heap_sort_tests_sift.cpp
*
*
* path:      /tests/djinterp/cpp/util/sort/heap_sort_tests_sift.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.12
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
    result.check(is_max_heap(data.begin(), data.end(), std::less<int>()));

    return result.ok();
}


// tests_heap_sort_sift_down_already_satisfied
bool tests_heap_sort_sift_down_already_satisfied()
{
    verdict                  result;
    std::vector<int>         data;
    std::vector<int>         original;
    std::vector<DKeyedValue> keyed;
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
    // tags of seven equal keys, because "nothing moved" and "moved and moved
    // back" are indistinguishable from the values alone -- and a sift that
    // moves a hole writes by assignment, which no swap counter would see.
    keyed.reserve(7);

    for (i = 0; i < 7; ++i)
    {
        keyed.push_back(make_keyed(4, static_cast<int>(i)));
    }

    ::djinterp::internal::heap_sift_down(keyed.begin(),
                                         static_cast<std::ptrdiff_t>(7),
                                         static_cast<std::ptrdiff_t>(0),
                                         keyed_value_less());

    for (i = 0; i < 7; ++i)
    {
        result.check(keyed[i].tag == static_cast<int>(i));
    }

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
    result.check(is_max_heap(data.begin(), data.end(), std::less<int>()));

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

    result.check(is_max_heap(data.begin(), data.end(), std::less<int>()));
    result.check(data[0] == 14);
    result.check(data[7] == 0);

    return result.ok();
}


// swapping_sift_reference
//   helper: the textbook sift, which exchanges the sinking element with its
// larger child level by level.  heap_sift_down moves a hole instead; the two
// make the same comparisons in the same order and must leave the same heap.
template<typename Compare>
static
void swapping_sift_reference(
    std::vector<int>& _data,
    std::ptrdiff_t    _size,
    std::ptrdiff_t    _root,
    Compare           _compare
)
{
    std::ptrdiff_t current;
    std::ptrdiff_t left;
    std::ptrdiff_t largest;

    current = _root;

    for (;;)
    {
        left = (2 * current) + 1;

        if (left >= _size)
        {
            break;
        }

        largest = current;

        if (_compare(_data[static_cast<std::size_t>(largest)],
                     _data[static_cast<std::size_t>(left)]))
        {
            largest = left;
        }

        if ( ((left + 1) < _size) &&
             (_compare(_data[static_cast<std::size_t>(largest)],
                       _data[static_cast<std::size_t>(left + 1)])) )
        {
            largest = left + 1;
        }

        if (largest == current)
        {
            break;
        }

        std::swap(_data[static_cast<std::size_t>(current)],
                  _data[static_cast<std::size_t>(largest)]);

        current = largest;
    }

    return;
}


// tests_heap_sort_larger_child_ties
bool tests_heap_sort_larger_child_ties()
{
    verdict                  result;
    std::vector<DKeyedValue> keyed;
    keyed_value_less         compare;

    //   heap_larger_child names the child a sift would promote; it takes its
    // comparator by reference, being called once per level.  A child must
    // STRICTLY follow to win, so on a tie between the two children the
    // LEFT one is kept -- one of the two choices that pin where heap sort
    // leaves equivalent elements.
    keyed.push_back(make_keyed(1, 0));
    keyed.push_back(make_keyed(5, 1));
    keyed.push_back(make_keyed(5, 2));

    result.check(::djinterp::internal::heap_larger_child(
                     keyed.begin(),
                     static_cast<std::ptrdiff_t>(3),
                     static_cast<std::ptrdiff_t>(0),
                     static_cast<std::ptrdiff_t>(1),
                     keyed[0],
                     compare) == 1);

    //   A strictly larger right child does win.
    keyed[2] = make_keyed(6, 2);

    result.check(::djinterp::internal::heap_larger_child(
                     keyed.begin(),
                     static_cast<std::ptrdiff_t>(3),
                     static_cast<std::ptrdiff_t>(0),
                     static_cast<std::ptrdiff_t>(1),
                     keyed[0],
                     compare) == 2);

    //   And a sinking element equal to both children keeps its place: the
    // node itself is reported, so nothing moves.
    keyed[0] = make_keyed(6, 0);
    keyed[1] = make_keyed(6, 1);

    result.check(::djinterp::internal::heap_larger_child(
                     keyed.begin(),
                     static_cast<std::ptrdiff_t>(3),
                     static_cast<std::ptrdiff_t>(0),
                     static_cast<std::ptrdiff_t>(1),
                     keyed[0],
                     compare) == 0);

    return result.ok();
}


// tests_heap_sort_sift_down_matches_swapping_sift
bool tests_heap_sort_sift_down_matches_swapping_sift()
{
    verdict          result;
    std::vector<int> moved;
    std::vector<int> swapped;
    std::size_t      size_index;
    std::size_t      shape_index;
    std::ptrdiff_t   size;
    std::ptrdiff_t   root;

    //   Every root of every battery input, sifted both ways from the same
    // start: the hole-moving sift must leave exactly the arrangement the
    // swapping one does.  Duplicates are where the two could differ, and the
    // battery's constant and few-unique shapes are full of them.
    for (size_index = 0; size_index < battery_size_count; ++size_index)
    {
        for (shape_index = 0;
             shape_index < battery_shape_count;
             ++shape_index)
        {
            moved = make_shape(shape_index, battery_size_at(size_index));
            size  = static_cast<std::ptrdiff_t>(moved.size());

            for (root = 0; root < size; ++root)
            {
                swapped = moved;

                heap_sift_ints(moved.begin(), size, root);
                swapping_sift_reference(swapped,
                                        size,
                                        root,
                                        std::less<int>());

                result.check(ranges_equal(moved, swapped));
            }
        }
    }

    return result.ok();
}


NS_END  // testing
NS_END  // djinterp
