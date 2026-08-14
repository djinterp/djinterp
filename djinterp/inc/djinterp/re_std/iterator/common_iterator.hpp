/******************************************************************************
* re_std [iterator]                                         common_iterator.hpp
*
*   common_iterator - erases an iterator/sentinel pair into a single type.
*
*   WHY IT IS NEEDED.
*   A C++20 range may end at a SENTINEL whose type differs from its iterator's.
* That is efficient - the sentinel can be an empty type testing a null
* terminator - but it is incompatible with every pre-C++20 algorithm, all of
* which take two iterators of the SAME type. common_iterator holds either an
* iterator or a sentinel in one type, so `common_iterator<I,S>(first)` and
* `common_iterator<I,S>(last)` form a pair the old algorithms accept.
*
*   THE STORAGE IS A TAGGED UNION, and it has to be: I and S are unrelated
* types, only one is ever live, and a struct holding both would require S to
* be default-constructible and would pay for the larger of the two plus
* padding on every copy. The union means the object is exactly as big as the
* larger alternative.
*
*   THE COMPARISON RULES ARE THE SUBTLE PART.
*     iterator vs iterator  -> compare the iterators
*     iterator vs sentinel  -> compare iterator against sentinel (the real test)
*     sentinel vs sentinel  -> TRUE, unconditionally
*
*   That last one is easy to get wrong by trying to compare the sentinels
*   themselves. Two common_iterators both holding a sentinel both denote the
*   end of the range, so they are equal whatever the sentinels contain - and S
*   is not required to be equality-comparable with itself at all, so comparing
*   them may not even compile.
*
*   PRECONDITION: dereferencing or incrementing while holding the SENTINEL is
* undefined. It is the end; there is nothing there. std says the same.
*
*   STD IS C++20; re_std IS C++11 - a nine-year back-port. std specifies it
* with the C++20 iterator concepts, but the adaptor itself needs only
* iterator_traits and a union, both available at C++11.
*
*
* path:      /inc/djinterp/re_std/iterator/common_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_ITERATOR_COMMON_ITERATOR_
#define RESTD_ITERATOR_COMMON_ITERATOR_ 1

// re_std
#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../memory/addressof.hpp"
#include "./iterator_traits.hpp"

NS_DJINTERP
NS_RESTD

// common_iterator
//   class: holds either an _Iter or a _Sent, presenting one iterator type.
template<typename _Iter, typename _Sent>
class common_iterator
{
    typedef iterator_traits<_Iter> _Traits;

    union
    {
        _Iter m_iter;
        _Sent m_sent;
    };
    bool m_is_iter;

    void destroy()
    {
        if (m_is_iter) { m_iter.~_Iter(); }
        else           { m_sent.~_Sent(); }
        return;
    }

    void construct_from(const common_iterator& other)
    {
        m_is_iter = other.m_is_iter;
        if (m_is_iter) { ::new (static_cast<void*>(re_std::addressof(m_iter))) _Iter(other.m_iter); }
        else           { ::new (static_cast<void*>(re_std::addressof(m_sent))) _Sent(other.m_sent); }
        return;
    }

public:
    typedef typename _Traits::value_type        value_type;
    typedef typename _Traits::difference_type   difference_type;
    typedef typename _Traits::reference         reference;
    typedef typename _Traits::pointer           pointer;
    typedef typename _Traits::iterator_category iterator_category;

    common_iterator() : m_iter(), m_is_iter(true) {}

    common_iterator(_Iter it) : m_iter(it), m_is_iter(true) {}

    common_iterator(_Sent se) : m_sent(se), m_is_iter(false) {}

    common_iterator(const common_iterator& other) { construct_from(other); }

    common_iterator& operator=(const common_iterator& other)
    {
        if (this != &other)
        {
            destroy();
            construct_from(other);
        }
        return *this;
    }

    ~common_iterator() { destroy(); }

    //   Undefined while holding the sentinel - see the header note.
    reference operator*()        { return *m_iter; }
    reference operator*()  const { return *m_iter; }
    pointer   operator->() const { return m_iter; }

    common_iterator& operator++()
    {
        ++m_iter;
        return *this;
    }

    common_iterator operator++(int)
    {
        common_iterator tmp = *this;
        ++m_iter;
        return tmp;
    }

    bool holds_iterator() const D_NOEXCEPT { return m_is_iter; }

    const _Iter& iter() const { return m_iter; }
    const _Sent& sent() const { return m_sent; }
};

// operator==
//   function: three cases, and the sentinel/sentinel one is unconditionally
// true - see the header note on why comparing the sentinels would be wrong.
template<typename _Iter, typename _Sent>
bool operator==(const common_iterator<_Iter, _Sent>& a,
                const common_iterator<_Iter, _Sent>& b)
{
    if (a.holds_iterator() && b.holds_iterator())
    {
        return a.iter() == b.iter();
    }
    if (!a.holds_iterator() && !b.holds_iterator())
    {
        //   Both denote the end. S need not even be comparable with itself.
        return true;
    }
    return a.holds_iterator() ? (a.iter() == b.sent())
                              : (b.iter() == a.sent());
}

template<typename _Iter, typename _Sent>
bool operator!=(const common_iterator<_Iter, _Sent>& a,
                const common_iterator<_Iter, _Sent>& b)
{
    return !(a == b);
}

NS_END  // re_std
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_COMMON_ITERATOR_
