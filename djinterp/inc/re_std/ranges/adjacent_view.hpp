/*******************************************************************************
* djinterp [re_std]                                            adjacent_view.hpp
*
* adjacent_view view header:
*   adjacent_view<V, N> - every window of N consecutive elements of ONE range.
* adjacent_transform_view applies a callable to each window.
*
*   IT IS NOT zip OF A RANGE WITH ITSELF, and the difference is what makes it
* a separate type.  zip holds N iterators into N ranges, advanced in lockstep
* from a common start.  adjacent holds N iterators into ONE range, STAGGERED -
* the k'th starts k positions in - and advanced together.  The shapes look
* alike and the end conditions do not.
*
*   A RANGE SHORTER THAN N YIELDS NOTHING AT ALL.  Constructing begin() has to
* walk the last iterator N-1 positions forward, and if it hits the end on the
* way the view is empty.  That is checked during construction rather than
* inferred later, because for an input range there is no way to ask "how many
* are left" without consuming them.  A naive implementation that only compared
* the FIRST iterator against the end would happily yield a window running past
* the end of the range - undefined behaviour, not a wrong count.
*
*   THE END CONDITION IS ON THE LAST ITERATOR, not the first, for the same
* reason: the window is exhausted when its trailing edge reaches the end.
*
*   STD IS C++23; re_std IS C++11.
*   INTERFACE ASSUMPTIONS: see ADAPTOR_ASSUMPTIONS.txt in this directory.
*
*
* path:      /inc/re_std/ranges/adjacent_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_ADJACENT_VIEW_HPP
#define RE_STD_RANGES_ADJACENT_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../utility/make_integer_sequence.hpp"
#include "../tuple/tuple.hpp"
#include "../tuple/tuple_get.hpp"
#include "../functional/invoke.hpp"
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

namespace internal
{

    // repeat_tuple
    //   trait: tuple<Type, Type, ...> with Count members.  Needed because
    // adjacent's window is N copies of ONE reference type, which no pack
    // expansion over the view list can produce.
    template<typename Type, size_t Count, typename... Acc>
    struct repeat_tuple : repeat_tuple<Type, Count - 1, Type, Acc...> {};

    template<typename Type, typename... Acc>
    struct repeat_tuple<Type, 0, Acc...>
    { typedef tuple<Acc...> type; };

    // adjacent_result
    //   trait: the type of f applied to Count copies of Ref.  Needed
    // because the window is N repeats of ONE type, and there is no pack to
    // expand - the same reason repeat_tuple exists, but yielding a call
    // result rather than a tuple.
    template<typename Func, typename Ref, size_t Count, typename... Acc>
    struct adjacent_result
        : adjacent_result<Func, Ref, Count - 1, Ref, Acc...> {};

    template<typename Func, typename Ref, typename... Acc>
    struct adjacent_result<Func, Ref, 0, Acc...>
    {
        typedef decltype(re_std::invoke(declval<const Func&>(),
                                        declval<Acc>()...)) type;
    };

}  // internal


// adjacent_view
//   class: sliding windows of N consecutive elements.
template<typename View, size_t Count>
class adjacent_view : public view_interface<adjacent_view<View, Count> >
{
    typedef iterator_t<View> _BaseIter;
    typedef sentinel_t<View> _BaseSent;

    View m_base;

public:
    typedef typename internal::repeat_tuple<
        range_reference_t<View>, Count>::type window_type;

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
        _BaseIter m_its[Count];
        bool      m_valid;

        template<size_t... I>
        window_type deref(index_sequence<I...>) const
        { return window_type(*m_its[I]...); }

    public:
        typedef window_type        reference;
        typedef window_type        value_type;
        typedef ptrdiff_t          difference_type;
        typedef void               pointer;
        typedef input_iterator_tag iterator_category;

        iterator() : m_valid(false) {}

        //   Staggered construction. If the range runs out before the window
        // is filled, the iterator is marked invalid and compares equal to the
        // sentinel immediately - see the header note.
        iterator(_BaseIter first, const _BaseSent& last) : m_valid(true)
        {
            //   Fill the window one position at a time, checking BEFORE each
            // store. If the range runs out first the window can never be
            // formed and the iterator is born equal to the sentinel.
            _BaseIter it = first;
            for (size_t i = 0; i < Count; ++i)
            {
                if (it == last) { m_valid = false; break; }
                m_its[i] = it;
                ++it;
            }
        }

        bool valid() const { return m_valid; }

        //   The TRAILING edge decides exhaustion.
        const _BaseIter& last_iter() const { return m_its[Count - 1]; }

        reference operator*() const
        { return deref(make_index_sequence<Count>()); }

        iterator& operator++()
        {
            for (size_t i = 0; i < Count; ++i) { ++m_its[i]; }
            return *this;
        }
        iterator operator++(int) { iterator t = *this; ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_its[0] == b.m_its[0]; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_its[0] == b.m_its[0]); }
        friend bool operator==(const iterator& a, const sentinel& s)
        { return !a.m_valid || a.m_its[Count - 1] == s.base(); }
        friend bool operator==(const sentinel& s, const iterator& a)
        { return !a.m_valid || a.m_its[Count - 1] == s.base(); }
        friend bool operator!=(const iterator& a, const sentinel& s)
        { return !(a == s); }
        friend bool operator!=(const sentinel& s, const iterator& a)
        { return !(a == s); }
    };

    adjacent_view() : m_base() {}
    explicit adjacent_view(View base) : m_base(static_cast<View&&>(base)) {}

    iterator begin() { return iterator(ranges::begin(m_base), ranges::end(m_base)); }
    sentinel end()   { return sentinel(ranges::end(m_base)); }
};


// adjacent_transform_view
//   class: f applied to each window, as zip_transform is to zip.
template<typename View, typename Func, size_t Count>
class adjacent_transform_view
    : public view_interface<adjacent_transform_view<View, Func, Count> >
{
    typedef adjacent_view<View, Count> _Adjacent;

    Func     m_func;
    _Adjacent m_adjacent;

public:
    typedef typename _Adjacent::sentinel sentinel;

    class iterator
    {
        const Func*                    m_func;
        typename _Adjacent::iterator    m_it;

    public:
        //   Computed from the trait, not from decltype of a member function -
        // naming call() here would make the typedef refer to the class being
        // defined.
        typedef typename internal::adjacent_result<
            Func, range_reference_t<View>, Count>::type reference;

    private:
        template<size_t... I>
        reference call(index_sequence<I...>) const
        {
            typename _Adjacent::window_type w = *m_it;
            return re_std::invoke(*m_func, re_std::get<I>(w)...);
        }

    public:
        typedef typename remove_cv<
            typename remove_reference<reference>::type>::type value_type;
        typedef ptrdiff_t          difference_type;
        typedef void               pointer;
        typedef input_iterator_tag iterator_category;

        iterator() : m_func(0), m_it() {}
        iterator(const Func& f, const typename _Adjacent::iterator& it)
            : m_func(&f), m_it(it) {}

        reference operator*() const { return call(make_index_sequence<Count>()); }

        iterator& operator++() { ++m_it; return *this; }
        iterator  operator++(int) { iterator t = *this; ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_it == b.m_it; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_it == b.m_it); }
        friend bool operator==(const iterator& a, const sentinel& s)
        { return a.m_it == s; }
        friend bool operator==(const sentinel& s, const iterator& a)
        { return a.m_it == s; }
        friend bool operator!=(const iterator& a, const sentinel& s)
        { return !(a.m_it == s); }
        friend bool operator!=(const sentinel& s, const iterator& a)
        { return !(a.m_it == s); }
    };

    adjacent_transform_view() : m_func(), m_adjacent() {}
    adjacent_transform_view(View base, Func f)
        : m_func(static_cast<Func&&>(f)),
          m_adjacent(static_cast<View&&>(base)) {}

    iterator begin() { return iterator(m_func, m_adjacent.begin()); }
    sentinel end()   { return m_adjacent.end(); }
};

}  // ranges
}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_RANGES_ADJACENT_VIEW_HPP
