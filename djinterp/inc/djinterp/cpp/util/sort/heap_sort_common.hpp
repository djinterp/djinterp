/*******************************************************************************
* djinterp [cpp]                                            heap_sort_common.hpp
*
*   The primitives every heap sort is built from.  A heap sort is a driver
* wrapped around one operation -- sift an element down an implicit binary
* max-heap until it dominates its children -- and the index arithmetic that
* locates a node's parent and children.  Heapify and extraction are both
* sequences of sifts; they differ only in which roots get sifted and over how
* much of the range.
*
*     - The sequential driver (heap_sort.hpp) sifts every internal node from
*       the last parent up to the root, then repeatedly exchanges the root
*       with the end of the heap and sifts the new root over what remains.
*     - A partial-sort or top-k driver builds the same heap and stops
*       extracting early, which is why the sift is a primitive here rather
*       than a step inside heap_sort_apply.
*
*   THE SIFT MOVES A HOLE, NOT AN ELEMENT.  The sinking element is held while
* each larger child is assigned up into the slot above it, and is written once,
* where it comes to rest: one assignment per level, against the three a swap
* would cost.  The comparisons are the ones a swapping sift makes, in the same
* order, so the heap it leaves is the same one -- only the writes are fewer.
* This is the move insertion_sort_common.hpp makes for the same reason.
*
*   STRICT COMPARISONS KEEP EQUALS WHERE THEY ARE.  A child replaces its parent
* only when the parent strictly precedes it, so a run of equivalent elements is
* never disturbed by a sift.  That does not make heap sort stable -- extraction
* exchanges across the whole range -- but it does mean a sift over an already
* valid heap writes nothing.
*
*
* path:      /inc/djinterp/cpp/util/sort/heap_sort_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_UTIL_SORT_HEAP_SORT_COMMON_HPP
#define DJINTERP_UTIL_SORT_HEAP_SORT_COMMON_HPP 1

// std
#include <iterator>               // std::iterator_traits
// djinterp
#include "../../djinterp.hpp"  // framework root


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// heap_sort_traits
//   struct: compile-time properties of the heap sort family, so a caller
// choosing between algorithms can branch on the property rather than the name.
//   - is_stable:   no   (extraction exchanges the root with the far end of the
//                        heap, past any number of equivalents)
//   - is_in_place: yes  (O(1) auxiliary memory; one held element)
//   - is_adaptive: no   (heapify reorders ordered input like any other, so
//                        every input costs O(n log n))
struct heap_sort_traits
{
    static const bool is_stable   = false;
    static const bool is_in_place = true;
    static const bool is_adaptive = false;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  PRIMITIVES                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // heap_parent
    //   function: the index of the parent of the node at _index in a
    // zero-based implicit binary heap.  Undefined for the root, which has none.
    template<typename Difference>
    Difference heap_parent(
        Difference _index
    )
    {
        return ((_index - 1) / 2);
    }

    // heap_left_child
    //   function: the index of the left child of the node at _index in a
    // zero-based implicit binary heap.  The right child is the next index.
    template<typename Difference>
    Difference heap_left_child(
        Difference _index
    )
    {
        return ((2 * _index) + 1);
    }

    // heap_larger_child
    //   function: the index of whichever child of _node is the larger -- or
    // _node itself when neither child strictly follows _value, the element
    // being sifted, which conceptually occupies _node.  _left is _node's left
    // child and must lie inside the heap of _size elements.
    //
    //   The comparisons are those of the textbook sift, in its order: the
    // sinking element against the left child, then the winner so far against
    // the right child.  A child must STRICTLY follow to win, so an equivalent
    // child never displaces its parent.
    template<typename RandomIterator,
             typename Difference,
             typename Type,
             typename Comparator>
    Difference heap_larger_child(
        RandomIterator _first,
        Difference     _size,
        Difference     _node,
        Difference     _left,
        const Type&    _value,
        Comparator&    _comparator
    )
    {
        Difference largest;

        largest = _node;

        // the left child wins when the sinking element precedes it
        if (_comparator(_value, _first[_left]))
        {
            largest = _left;
        }

        // the right child, when there is one, must beat the winner so far
        if ((_left + 1) < _size)
        {
            if (largest == _node)
            {
                if (_comparator(_value, _first[_left + 1]))
                {
                    largest = _left + 1;
                }
            }
            else if (_comparator(_first[_left], _first[_left + 1]))
            {
                largest = _left + 1;
            }
        }

        return largest;
    }

    // heap_sift_down
    //   function: restores the max-heap property for the sub-tree rooted at
    // _root inside the heap [_first, _first + _size), on the assumption that
    // both of _root's sub-trees are already heaps.
    //
    //   The element at _root is compared with its children before anything
    // moves, so a root that already dominates them -- including one equal to
    // them -- leaves the range untouched.  Otherwise it is held while each
    // larger child is assigned up into the hole above it, level by level,
    // and written once where it comes to rest: at a leaf, or above children
    // that do not strictly follow it.
    //
    //   The caller owns the range.  A sift does not check _root against
    // _size, because it is called by a driver that checked once per sort.
    template<typename RandomIterator,
             typename Difference,
             typename Comparator>
    void heap_sift_down(
        RandomIterator _first,
        Difference     _size,
        Difference     _root,
        Comparator     _comparator
    )
    {
        typedef typename std::iterator_traits<RandomIterator>::value_type
            value_type;

        Difference hole;
        Difference left;
        Difference larger;

        left = heap_left_child(_root);

        // a leaf has nothing below it to fall behind
        if (left >= _size)
        {
            return;
        }

        larger = heap_larger_child(_first,
                                   _size,
                                   _root,
                                   left,
                                   _first[_root],
                                   _comparator);

        // already dominates its children: nothing moves
        if (larger == _root)
        {
            return;
        }

        // the sinking element, held out of the range while the hole descends.
        // Copy-initialised, so the element type needs no default constructor.
        value_type sinking = _first[_root];

        hole = _root;

        while (larger != hole)
        {
            _first[hole] = _first[larger];
            hole         = larger;
            left         = heap_left_child(hole);

            // the hole reached a leaf
            if (left >= _size)
            {
                break;
            }

            larger = heap_larger_child(_first,
                                       _size,
                                       hole,
                                       left,
                                       sinking,
                                       _comparator);
        }

        _first[hole] = sinking;

        return;
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTIL_SORT_HEAP_SORT_COMMON_HPP
