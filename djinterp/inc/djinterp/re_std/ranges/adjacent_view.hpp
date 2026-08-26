/******************************************************************************
* re_std [ranges]                                             adjacent_view.hpp
*
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
* path:      /inc/djinterp/re_std/ranges/adjacent_view.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_ADJACENT_VIEW_
#define DJINTERP_RE_STD_RANGES_ADJACENT_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../tuple/tuple.hpp"
#include "../functional/invoke.hpp"
#include "../iterator/iterator_tags.hpp"
#include "./range_traits.hpp"
#include "./range_access.hpp"
#include "./view_interface.hpp"

NS_RESTD
D_NAMESPACE(ranges)

NS_INTERNAL

    // repeat_tuple
    //   trait: tuple<_Type, _Type, ...> with _Count members.  Needed because
    // adjacent's window is N copies of ONE reference type, which no pack
    // expansion over the view list can produce.
    template<typename _Type, size_t _Count, typename... _Acc>
    struct repeat_tuple : repeat_tuple<_Type, _Count - 1, _Type, _Acc...> {};

    template<typename _Type, typename... _Acc>
    struct repeat_tuple<_Type, 0, _Acc...>
    { typedef tuple<_Acc...> type; };

    // adjacent_result
    //   trait: the type of f applied to _Count copies of _Ref.  Needed
    // because the window is N repeats of ONE type, and there is no pack to
    // expand - the same reason repeat_tuple exists, but yielding a call
    // result rather than a tuple.
    template<typename _Func, typename _Ref, size_t _Count, typename... _Acc>
    struct adjacent_result
        : adjacent_result<_Func, _Ref, _Count - 1, _Ref, _Acc...> {};

    template<typename _Func, typename _Ref, typename... _Acc>
    struct adjacent_result<_Func, _Ref, 0, _Acc...>
    {
        typedef decltype(re_std::invoke(declval<const _Func&>(),
                                        declval<_Acc>()...)) type;
    };

NS_END  // internal


// adjacent_view
//   class: sliding windows of N consecutive elements.
template<typename _View, size_t _Count>
class adjacent_view : public view_interface<adjacent_view<_View, _Count> >
{
    typedef iterator_t<_View> _BaseIter;
    typedef sentinel_t<_View> _BaseSent;

    _View m_base;

public:
    typedef typename internal::repeat_tuple<
        range_reference_t<_View>, _Count>::type window_type;

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
        _BaseIter m_its[_Count];
        bool      m_valid;

        template<size_t... _I>
        window_type deref(index_sequence<_I...>) const
        { return window_type(*m_its[_I]...); }

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
            for (size_t i = 0; i < _Count; ++i)
            {
                if (it == last) { m_valid = false; break; }
                m_its[i] = it;
                ++it;
            }
        }

        bool valid() const { return m_valid; }

        //   The TRAILING edge decides exhaustion.
        const _BaseIter& last_iter() const { return m_its[_Count - 1]; }

        reference operator*() const
        { return deref(make_index_sequence<_Count>()); }

        iterator& operator++()
        {
            for (size_t i = 0; i < _Count; ++i) { ++m_its[i]; }
            return *this;
        }
        iterator operator++(int) { iterator t = *this; ++(*this); return t; }

        friend bool operator==(const iterator& a, const iterator& b)
        { return a.m_its[0] == b.m_its[0]; }
        friend bool operator!=(const iterator& a, const iterator& b)
        { return !(a.m_its[0] == b.m_its[0]); }
        friend bool operator==(const iterator& a, const sentinel& s)
        { return !a.m_valid || a.m_its[_Count - 1] == s.base(); }
        friend bool operator==(const sentinel& s, const iterator& a)
        { return !a.m_valid || a.m_its[_Count - 1] == s.base(); }
        friend bool operator!=(const iterator& a, const sentinel& s)
        { return !(a == s); }
        friend bool operator!=(const sentinel& s, const iterator& a)
        { return !(a == s); }
    };

    adjacent_view() : m_base() {}
    explicit adjacent_view(_View base) : m_base(static_cast<_View&&>(base)) {}

    iterator begin() { return iterator(ranges::begin(m_base), ranges::end(m_base)); }
    sentinel end()   { return sentinel(ranges::end(m_base)); }
};


// adjacent_transform_view
//   class: f applied to each window, as zip_transform is to zip.
template<typename _View, typename _Func, size_t _Count>
class adjacent_transform_view
    : public view_interface<adjacent_transform_view<_View, _Func, _Count> >
{
    typedef adjacent_view<_View, _Count> _Adjacent;

    _Func     m_func;
    _Adjacent m_adjacent;

public:
    typedef typename _Adjacent::sentinel sentinel;

    class iterator
    {
        const _Func*                    m_func;
        typename _Adjacent::iterator    m_it;

    public:
        //   Computed from the trait, not from decltype of a member function -
        // naming call() here would make the typedef refer to the class being
        // defined.
        typedef typename internal::adjacent_result<
            _Func, range_reference_t<_View>, _Count>::type reference;

    private:
        template<size_t... _I>
        reference call(index_sequence<_I...>) const
        {
            typename _Adjacent::window_type w = *m_it;
            return re_std::invoke(*m_func, re_std::get<_I>(w)...);
        }

    public:
        typedef typename remove_cv<
            typename remove_reference<reference>::type>::type value_type;
        typedef ptrdiff_t          difference_type;
        typedef void               pointer;
        typedef input_iterator_tag iterator_category;

        iterator() : m_func(0), m_it() {}
        iterator(const _Func& f, const typename _Adjacent::iterator& it)
            : m_func(&f), m_it(it) {}

        reference operator*() const { return call(make_index_sequence<_Count>()); }

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
    adjacent_transform_view(_View base, _Func f)
        : m_func(static_cast<_Func&&>(f)),
          m_adjacent(static_cast<_View&&>(base)) {}

    iterator begin() { return iterator(m_func, m_adjacent.begin()); }
    sentinel end()   { return m_adjacent.end(); }
};

NS_END  // ranges
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_RANGES_ADJACENT_VIEW_
