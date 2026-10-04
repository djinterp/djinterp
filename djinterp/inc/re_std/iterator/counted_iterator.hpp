/******************************************************************************
* re_std [iterator]                                        counted_iterator.hpp
*
*   counted_iterator - an iterator that carries its own remaining length.
*
*   WHY THIS EXISTS.
*   It turns "the first n elements" into a RANGE without needing a second
* iterator.  Advancing an ordinary iterator n times to find the end is O(n)
* for a non-random-access iterator and impossible for an input iterator that
* cannot be traversed twice.  counted_iterator carries the count instead and
* reports exhaustion by comparing equal to default_sentinel.
*
*   THE COUNT RUNS DOWN, NOT UP.  count() is the number of steps REMAINING, so
* it decrements on ++ and increments on --.  Reaching zero is what makes the
* iterator equal to default_sentinel.
*
*   operator- BETWEEN TWO counted_iteratorS IS REVERSED, and this is the one
* piece of the interface that reliably surprises people:
*
*       a - b   ==   b.count() - a.count()
*
*   That is not a typo in the standard.  Counts run DOWN, so the iterator that
* has advanced further has the SMALLER count; subtracting the counts in the
* obvious order would give the distance with the wrong sign.  There is a test
* for this precisely because it looks like a bug.
*
*   PRECONDITION: never advance past the count.  std leaves that undefined and
* re_std does the same rather than paying for a check on every increment; the
* whole point of the adaptor is that the bound is known, so overshooting it is
* a caller error, not a case to handle.
*
*   STD IS C++20; re_std IS C++11 - a nine-year back-port.  std specifies it in
* terms of the C++20 iterator concepts, but the adaptor itself needs only
* iterator_traits, which re_std has from C++11.
*
*
* path:      /inc/djinterp/re_std/iterator/counted_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_COUNTED_ITERATOR_
#define DJINTERP_RE_STD_ITERATOR_COUNTED_ITERATOR_ 1

// re_std
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../utility/utility.hpp"
#include "../memory/addressof.hpp"
#include "./iterator_traits.hpp"
#include "./default_sentinel.hpp"

NS_RESTD

// counted_iterator
//   class: adapts _Iter, carrying the number of steps remaining.
template<typename _Iter>
class counted_iterator
{
    typedef iterator_traits<_Iter> _Traits;

    _Iter                            m_current;
    typename _Traits::difference_type m_length;

public:
    typedef _Iter                                iterator_type;
    typedef typename _Traits::value_type         value_type;
    typedef typename _Traits::difference_type    difference_type;
    typedef typename _Traits::reference          reference;
    typedef typename _Traits::pointer            pointer;
    typedef typename _Traits::iterator_category  iterator_category;

    D_CONSTEXPR counted_iterator() : m_current(), m_length(0) {}

    D_CONSTEXPR counted_iterator(_Iter it, difference_type n)
        : m_current(it), m_length(n)
    {}

    D_CONSTEXPR const _Iter& base() const { return m_current; }

    //   Steps REMAINING, not steps taken.
    D_CONSTEXPR difference_type count() const D_NOEXCEPT { return m_length; }

    //   The non-const overload is D_CONSTEXPR_CPP14, not D_CONSTEXPR, and
    // that is not a stylistic choice: in C++11 `constexpr` on a member
    // function IMPLIES const, so marking both overloads D_CONSTEXPR makes
    // them the same signature and the class fails to compile - at C++11 only.
    // C++14 dropped the implication (N3598), which is why the pair is legal
    // from there.  Caught by the tier matrix; it compiles cleanly at every
    // other tier.
    D_CONSTEXPR_CPP14 reference operator*()       { return *m_current; }
    D_CONSTEXPR       reference operator*() const { return *m_current; }
    D_CONSTEXPR       pointer   operator->() const { return m_current; }

    D_CONSTEXPR_CPP14 counted_iterator& operator++()
    {
        ++m_current;
        --m_length;
        return *this;
    }

    D_CONSTEXPR_CPP14 counted_iterator operator++(int)
    {
        counted_iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    D_CONSTEXPR_CPP14 counted_iterator& operator--()
    {
        --m_current;
        ++m_length;
        return *this;
    }

    D_CONSTEXPR_CPP14 counted_iterator operator--(int)
    {
        counted_iterator tmp = *this;
        --(*this);
        return tmp;
    }

    D_CONSTEXPR_CPP14 counted_iterator& operator+=(difference_type n)
    {
        m_current += n;
        m_length  -= n;
        return *this;
    }

    D_CONSTEXPR_CPP14 counted_iterator& operator-=(difference_type n)
    {
        m_current -= n;
        m_length  += n;
        return *this;
    }

    D_CONSTEXPR counted_iterator operator+(difference_type n) const
    { return counted_iterator(m_current + n, m_length - n); }

    D_CONSTEXPR counted_iterator operator-(difference_type n) const
    { return counted_iterator(m_current - n, m_length + n); }

    D_CONSTEXPR reference operator[](difference_type n) const
    { return m_current[n]; }
};

// operator- (counted_iterator, counted_iterator)
//   function: distance between two counted iterators.  REVERSED on purpose -
// counts run down, so the further-advanced iterator has the smaller count.
template<typename _Iter1, typename _Iter2>
D_CONSTEXPR typename iterator_traits<_Iter2>::difference_type
operator-(const counted_iterator<_Iter1>& a, const counted_iterator<_Iter2>& b)
{
    return b.count() - a.count();
}

// operator- (counted_iterator, default_sentinel_t)
//   function: negative distance to the end - std specifies this sign.
template<typename _Iter>
D_CONSTEXPR typename iterator_traits<_Iter>::difference_type
operator-(const counted_iterator<_Iter>& a, default_sentinel_t)
{
    return -a.count();
}

template<typename _Iter>
D_CONSTEXPR typename iterator_traits<_Iter>::difference_type
operator-(default_sentinel_t, const counted_iterator<_Iter>& b)
{
    return b.count();
}

template<typename _Iter>
D_CONSTEXPR counted_iterator<_Iter>
operator+(typename iterator_traits<_Iter>::difference_type n,
          const counted_iterator<_Iter>& it)
{ return it + n; }

// ---- comparisons ---------------------------------------------------------
//   Two counted_iterators compare by COUNT, not by the underlying iterator:
// that is what makes the comparison valid for input iterators, whose
// underlying == may not be meaningful across copies.

template<typename _Iter1, typename _Iter2>
D_CONSTEXPR bool operator==(const counted_iterator<_Iter1>& a,
                            const counted_iterator<_Iter2>& b)
{ return a.count() == b.count(); }

template<typename _Iter1, typename _Iter2>
D_CONSTEXPR bool operator!=(const counted_iterator<_Iter1>& a,
                            const counted_iterator<_Iter2>& b)
{ return a.count() != b.count(); }

template<typename _Iter1, typename _Iter2>
D_CONSTEXPR bool operator<(const counted_iterator<_Iter1>& a,
                           const counted_iterator<_Iter2>& b)
{ return b.count() < a.count(); }

template<typename _Iter1, typename _Iter2>
D_CONSTEXPR bool operator>(const counted_iterator<_Iter1>& a,
                           const counted_iterator<_Iter2>& b)
{ return b.count() > a.count(); }

template<typename _Iter1, typename _Iter2>
D_CONSTEXPR bool operator<=(const counted_iterator<_Iter1>& a,
                            const counted_iterator<_Iter2>& b)
{ return b.count() <= a.count(); }

template<typename _Iter1, typename _Iter2>
D_CONSTEXPR bool operator>=(const counted_iterator<_Iter1>& a,
                            const counted_iterator<_Iter2>& b)
{ return b.count() >= a.count(); }

//   Exhaustion.
template<typename _Iter>
D_CONSTEXPR bool operator==(const counted_iterator<_Iter>& a, default_sentinel_t)
{ return a.count() == 0; }

template<typename _Iter>
D_CONSTEXPR bool operator==(default_sentinel_t, const counted_iterator<_Iter>& a)
{ return a.count() == 0; }

template<typename _Iter>
D_CONSTEXPR bool operator!=(const counted_iterator<_Iter>& a, default_sentinel_t)
{ return a.count() != 0; }

template<typename _Iter>
D_CONSTEXPR bool operator!=(default_sentinel_t, const counted_iterator<_Iter>& a)
{ return a.count() != 0; }

NS_END  // re_std
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_COUNTED_ITERATOR_
