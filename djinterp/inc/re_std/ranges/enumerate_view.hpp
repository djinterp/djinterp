/*******************************************************************************
* djinterp [re_std]                                           enumerate_view.hpp
*
* enumerate_view view header:
*   enumerate_view - pairs each element with its zero-based position.
*
*   THE INDEX IS CARRIED, NOT COMPUTED.  It would be tempting to derive the
* position by subtracting iterators, but that only works for random-access
* ranges; enumerate must work over an input range that can be traversed once
* and cannot be subtracted.  So the iterator holds a counter and increments it
* alongside the base iterator, which is why enumerate costs one extra integer
* per iterator and nothing else.
*
*   THE INDEX COMES FIRST in the yielded tuple - `(index, element)` - matching
* std.  It reads backwards to anyone expecting the element to lead, but it is
* what structured bindings make natural: `for (auto [i, x] : enumerate(r))`.
*
*   THE ELEMENT IS A REFERENCE, the index a value.  Writing through the second
* member mutates the underlying range; the index is a snapshot and assigning
* to it does nothing useful.
*
*   STD IS C++23; re_std IS C++11 - a twelve-year back-port.
*
*   INTERFACE ASSUMPTIONS: see ADAPTOR_ASSUMPTIONS.txt in this directory.
*
*
* path:      /inc/re_std/ranges/enumerate_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_ENUMERATE_VIEW_HPP
#define RE_STD_RANGES_ENUMERATE_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../tuple/tuple.hpp"
#include "./view_interface.hpp"
#include "../iterator/input_iterator_tag.hpp"
#include "./ranges_access.hpp"
#include "./iterator_t.hpp"
#include "./range_reference_t.hpp"
#include "./sentinel_t.hpp"

namespace re_std
{
namespace ranges
{

// enumerate_view
//   class: yields (index, element) for each element of the base range.
template<typename View>
class enumerate_view : public view_interface<enumerate_view<View> >
{
    typedef iterator_t<View> _BaseIter;
    typedef sentinel_t<View> _BaseSent;

    View m_base;

public:
    class sentinel
    {
        _BaseSent m_end;
    public:
        sentinel() : m_end() {}
        explicit sentinel(const _BaseSent& e) : m_end(e) {}
        const _BaseSent& base() const { return m_end; }
    };

    class iterator
    {
        _BaseIter m_it;
        ptrdiff_t m_index;

    public:
        typedef tuple<ptrdiff_t,
                      typename remove_reference<
                          range_reference_t<View> >::type> value_type;
        typedef tuple<ptrdiff_t, range_reference_t<View> > reference;
        typedef ptrdiff_t                                   difference_type;
        typedef void                                        pointer;
        typedef input_iterator_tag                          iterator_category;

        iterator() : m_it(), m_index(0) {}
        iterator(const _BaseIter& it, ptrdiff_t index)
            : m_it(it), m_index(index) {}

        const _BaseIter& base()  const { return m_it; }
        ptrdiff_t        index() const { return m_index; }

        //   Index first, matching std - see the header note.
        reference operator*() const { return reference(m_index, *m_it); }

        iterator& operator++() { ++m_it; ++m_index; return *this; }
        iterator  operator++(int) { iterator t = *this; ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_it == b.m_it; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_it == b.m_it); }
        friend bool operator==(const iterator& a, const sentinel& s)
        { return a.m_it == s.base(); }
        friend bool operator==(const sentinel& s, const iterator& a)
        { return a.m_it == s.base(); }
        friend bool operator!=(const iterator& a, const sentinel& s)
        { return !(a.m_it == s.base()); }
        friend bool operator!=(const sentinel& s, const iterator& a)
        { return !(a.m_it == s.base()); }
    };

    enumerate_view() : m_base() {}
    explicit enumerate_view(View base) : m_base(static_cast<View&&>(base)) {}

    iterator begin() { return iterator(ranges::begin(m_base), 0); }
    sentinel end()   { return sentinel(ranges::end(m_base)); }
};

}  // ranges
}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_RANGES_ENUMERATE_VIEW_HPP
