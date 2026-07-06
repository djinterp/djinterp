/******************************************************************************
* djinterp [container]                                hierarchical_iterator.hpp
*
*   The foundational HIERARCHICAL iterator: a flattening traversal that unfolds
* one level of nesting.  Where the flat iterator visits a container's own leaf
* positions, this visits the leaves BENEATH a node summand - given a container
* whose elements are themselves containers (the formal T = tau + F[T], with F[T]
* the node summand, a container of components), it enumerates the leaves of every
* inner range in turn, skipping empty ones.  Applied where the inner elements are
* leaves it flattens a depth-2 structure to its elements; deeper nesting unfolds a
* level at a time.
*
*   It realises the iterability axes as the flat iterator does, within the limits
* nesting imposes (the spec, Iterability):
*     STAGE.   Observation and comparison are D_CONSTEXPR.  Advancing must skip
*              past exhausted and empty inner ranges - a loop - so the traversal is
*              constexpr from C++14 (relaxed constexpr) and a runtime operation
*              before that; there is no cheaper functional form, since locating the
*              next leaf is inherently a search.
*     MODE.    Constness follows the outer iterator - a const outer iterator yields
*              const leaves - and may additionally be FORCED const by the type
*              parameter, giving a read-only traversal over a mutable structure.
*
*   The category is forward: a flattening admits neither random access nor a cheap
* step backward.
*
*   PORTABILITY:
*   C++11 baseline.  Observation and comparison are constexpr throughout; traversal
* and construction are constexpr from C++14.
*
*
* path:      /inc/djinterp/core/container/iterator/hierarchical_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_HIERARCHICAL_ITERATOR_
#define DJINTERP_CONTAINER_HIERARCHICAL_ITERATOR_ 1

// std
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"   // D_CONSTEXPR, NS_*, feature macros


// D_ITER_CONSTEXPR_MUT
//   an operation that must loop (advancing past empty inner ranges, or settling a
// freshly constructed iterator) is constexpr only from C++14 (relaxed constexpr).
#ifndef D_ITER_CONSTEXPR_MUT
    #if ( D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VAL >= 201304L )
        #define D_ITER_CONSTEXPR_MUT  constexpr
    #else
        #define D_ITER_CONSTEXPR_MUT
    #endif
#endif


NS_DJINTERP


// hierarchical_iterator
//   class: a forward iterator flattening the leaves beneath an outer range whose
// elements are inner ranges.  Parameterised on the outer iterator type and on a
// force-const flag; the leaf constness is the outer iterator's, tightened by the
// flag.
template<typename _OuterIter,
         bool     _Const = false>
class hierarchical_iterator
{
private:
    // the inner range is what the outer iterator dereferences to; the inner
    // iterator traverses it, and its dereference is the leaf.
    using inner_iterator =
        decltype(std::begin(*std::declval<_OuterIter&>()));
    using leaf_natural_reference =
        decltype(*std::declval<inner_iterator&>());
    using leaf_value =
        typename std::remove_reference<leaf_natural_reference>::type;

public:
    using value_type =
        typename std::remove_cv<leaf_value>::type;
    using reference =
        typename std::conditional<_Const,
            const leaf_value&, leaf_natural_reference>::type;
    using pointer =
        typename std::remove_reference<reference>::type*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    // ------------------------------------------------------------------
    //  construction
    // ------------------------------------------------------------------

    // hierarchical_iterator (default)
    //   a singular past-the-end iterator.
    hierarchical_iterator()
        : m_outer(), m_outer_end(), m_inner()
    {}

    // hierarchical_iterator (range)
    //   an iterator at the first leaf of [_outer, _outer_end), skipping any
    // leading empty inner ranges.  (Constexpr from C++14 - it settles by looping.)
    D_ITER_CONSTEXPR_MUT hierarchical_iterator(
        _OuterIter _outer,
        _OuterIter _outer_end
    )
        : m_outer(_outer), m_outer_end(_outer_end), m_inner()
    {
        if (m_outer != m_outer_end)
        {
            m_inner = std::begin(*m_outer);
        }

        settle();
    }

    // ------------------------------------------------------------------
    //  access (observing - constexpr throughout)
    // ------------------------------------------------------------------

    // operator*
    //   the current leaf (settable when non-const and the outer iterator is).
    constexpr reference operator*() const
    {
        return static_cast<reference>(*m_inner);
    }

    // operator->
    //   a pointer to the current leaf.
    pointer operator->() const
    {
        return &(*m_inner);
    }

    // ------------------------------------------------------------------
    //  traversal (constexpr from C++14)
    // ------------------------------------------------------------------

    // operator++ (pre)
    //   advances to the next leaf, crossing inner-range boundaries as needed.
    D_ITER_CONSTEXPR_MUT hierarchical_iterator& operator++()
    {
        ++m_inner;
        settle();

        return *this;
    }

    // operator++ (post)
    D_ITER_CONSTEXPR_MUT hierarchical_iterator operator++(int)
    {
        hierarchical_iterator _tmp(*this);
        ++(*this);

        return _tmp;
    }

    // ------------------------------------------------------------------
    //  comparison (constexpr throughout)
    // ------------------------------------------------------------------

    // operator==
    //   two iterators are equal when they rest on the same outer position and,
    // unless both are at the end, the same inner position.
    constexpr bool operator==(const hierarchical_iterator& _o) const
    {
        return (    m_outer == _o.m_outer
                 && (    m_outer == m_outer_end
                      || m_inner == _o.m_inner ) );
    }

    // operator!=
    constexpr bool operator!=(const hierarchical_iterator& _o) const
    {
        return !(*this == _o);
    }

private:
    // settle
    //   advances past exhausted / empty inner ranges until either a leaf is in
    // view or the outer range is spent.
    D_ITER_CONSTEXPR_MUT void settle()
    {
        while (    m_outer != m_outer_end
                && m_inner == std::end(*m_outer) )
        {
            ++m_outer;

            if (m_outer != m_outer_end)
            {
                m_inner = std::begin(*m_outer);
            }
        }

        return;
    }

    _OuterIter    m_outer;
    _OuterIter    m_outer_end;
    inner_iterator m_inner;
};


// ===========================================================================
//  range wrapper + factories
// ===========================================================================

// hierarchical_view
//   class: a lightweight range over the flattened leaves of a container of inner
// ranges, so the traversal may drive a range-for.  Holds only iterators; it owns
// nothing.  Constness of the leaves follows _Container's.
template<typename _Container,
         bool     _Const = false>
class hierarchical_view
{
private:
    using outer_iterator =
        decltype(std::begin(std::declval<_Container&>()));

public:
    using iterator = hierarchical_iterator<outer_iterator, _Const>;

    // hierarchical_view (container)
    explicit hierarchical_view(_Container& _c)
        : m_begin(std::begin(_c)),
          m_end(std::end(_c))
    {}

    // begin
    //   an iterator at the first leaf.
    D_ITER_CONSTEXPR_MUT iterator begin() const
    {
        return iterator(m_begin, m_end);
    }

    // end
    //   the past-the-end iterator.
    D_ITER_CONSTEXPR_MUT iterator end() const
    {
        return iterator(m_end, m_end);
    }

private:
    outer_iterator m_begin;
    outer_iterator m_end;
};

// make_hierarchical_iterator
//   factory: the flattening iterator at the first leaf of [_outer, _outer_end).
template<typename _OuterIter>
D_ITER_CONSTEXPR_MUT hierarchical_iterator<_OuterIter, false>
make_hierarchical_iterator(
    _OuterIter _outer,
    _OuterIter _outer_end
)
{
    return hierarchical_iterator<_OuterIter, false>(_outer, _outer_end);
}

// make_hierarchical_view
//   factory: a flattened range over _c.
template<typename _Container>
hierarchical_view<_Container, false>
make_hierarchical_view(_Container& _c)
{
    return hierarchical_view<_Container, false>(_c);
}

// make_const_hierarchical_view
//   factory: a read-only flattened range over _c.
template<typename _Container>
hierarchical_view<_Container, true>
make_const_hierarchical_view(_Container& _c)
{
    return hierarchical_view<_Container, true>(_c);
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_HIERARCHICAL_ITERATOR_
