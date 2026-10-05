/*******************************************************************************
* djinterp [re_std]                                         reverse_iterator.hpp
*
* reverse_iterator class header:
* iterator adaptor that wraps a bidirectional (or random-access)
* iterator and presents the inverse traversal: ++r is conceptually
* --base, *r dereferences the element BEFORE the wrapped iterator's
* current position.
*
* the off-by-one rule:
*
*   reverse_iterator's stored base() points one PAST the logical
*   element it dereferences. So:
*
*       r.base() == it          implies *r == *(it - 1)
*
*   This is why rbegin() = reverse_iterator(end()) and rend() =
*   reverse_iterator(begin()): the past-the-end iterator becomes the
*   first reverse element, and begin becomes the past-the-end-of-
*   reverse position.
*
* surface:
*   - default ctor, value ctor, converting copy ctor from
*     reverse_iterator<U>
*   - operator*, operator->, operator[]
*   - operator++, --, +=, -=, +, -
*   - base()
*   - the standard six relational operators
*
* constexpr availability:
*   - non-mutating ops (default ctor, value ctor, base, etc.) are
*     `constexpr` on every tier from C++11+.
*   - mutating ops (op++, op--, op+=, op-=, even op*, op->, op[] —
*     because they internally mutate a local copy of base) are only
*     `constexpr` on C++14+. C++11 forbids constexpr non-static
*     non-const member functions, so they are unqualified there.
*     Local RE_STD_CONSTEXPR_CPP14 macro below; gated on
*     RE_STD_LANG_IS_CPP14_OR_HIGHER.
*
*
* path:      /inc/re_std/iterator/reverse_iterator.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_REVERSE_ITERATOR_HPP
#define RE_STD_ITERATOR_REVERSE_ITERATOR_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "re_std/iterator/iterator_traits.hpp"


namespace re_std
{

template<typename Iter>
class reverse_iterator
{
public:
    typedef Iter                                              iterator_type;
    typedef typename iterator_traits<Iter>::iterator_category iterator_category;
    typedef typename iterator_traits<Iter>::value_type        value_type;
    typedef typename iterator_traits<Iter>::difference_type   difference_type;
    typedef typename iterator_traits<Iter>::pointer           pointer;
    typedef typename iterator_traits<Iter>::reference         reference;

protected:
    Iter current;

public:
    // ---- constructors ----

    RE_STD_CONSTEXPR reverse_iterator()
        : current() {}

    RE_STD_CONSTEXPR explicit reverse_iterator(iterator_type _x)
        : current(_x) {}

    template<typename U>
    RE_STD_CONSTEXPR reverse_iterator(const reverse_iterator<U>& _o)
        : current(_o.base()) {}

    template<typename U>
    RE_STD_CONSTEXPR_CPP14 reverse_iterator&
    operator=(const reverse_iterator<U>& _o)
    {
        current = _o.base();
        return *this;
    }

    // ---- access ----

    RE_STD_CONSTEXPR iterator_type base() const { return current; }

    RE_STD_CONSTEXPR_CPP14 reference operator*() const
    {
        // Off-by-one: r.current points one past the logical element.
        Iter _tmp = current;
        return *--_tmp;
    }

    RE_STD_CONSTEXPR_CPP14 pointer operator->() const
    {
        Iter _tmp = current;
        --_tmp;
        return to_pointer(_tmp);
    }

    RE_STD_CONSTEXPR_CPP14 reference operator[](difference_type _n) const
    {
        return *(*this + _n);
    }

    // ---- arithmetic ----

    RE_STD_CONSTEXPR_CPP14 reverse_iterator& operator++()
    {
        --current;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator operator++(int)
    {
        reverse_iterator _r = *this;
        --current;
        return _r;
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator& operator--()
    {
        ++current;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator operator--(int)
    {
        reverse_iterator _r = *this;
        ++current;
        return _r;
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator
    operator+(difference_type _n) const
    {
        return reverse_iterator(current - _n);
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator&
    operator+=(difference_type _n)
    {
        current -= _n;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator
    operator-(difference_type _n) const
    {
        return reverse_iterator(current + _n);
    }

    RE_STD_CONSTEXPR_CPP14 reverse_iterator&
    operator-=(difference_type _n)
    {
        current += _n;
        return *this;
    }

private:
    // operator-> helper: raw pointer pass-through, class iterator
    // dispatch via member operator->.
    template<typename T>
    static RE_STD_CONSTEXPR T* to_pointer(T* _p) { return _p; }

    template<typename It>
    static RE_STD_CONSTEXPR_CPP14 pointer
    to_pointer(It _it) { return _it.operator->(); }
};


// ---- non-member relational ----

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator==(const reverse_iterator<A>& _x,
                            const reverse_iterator<B>& _y)
{
    return _x.base() == _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator!=(const reverse_iterator<A>& _x,
                            const reverse_iterator<B>& _y)
{
    return _x.base() != _y.base();
}

// FLIPPED ordering: r1 < r2 iff base(r1) > base(r2). The reverse
// iterator at the larger base is the "earlier" one in reverse order.
template<typename A, typename B>
RE_STD_CONSTEXPR bool operator<(const reverse_iterator<A>& _x,
                           const reverse_iterator<B>& _y)
{
    return _x.base() > _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator>(const reverse_iterator<A>& _x,
                           const reverse_iterator<B>& _y)
{
    return _x.base() < _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator<=(const reverse_iterator<A>& _x,
                            const reverse_iterator<B>& _y)
{
    return _x.base() >= _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator>=(const reverse_iterator<A>& _x,
                            const reverse_iterator<B>& _y)
{
    return _x.base() <= _y.base();
}


// ---- non-member arithmetic ----

template<typename Iter>
RE_STD_CONSTEXPR reverse_iterator<Iter>
operator+(typename reverse_iterator<Iter>::difference_type _n,
          const reverse_iterator<Iter>& _r)
{
    return reverse_iterator<Iter>(_r.base() - _n);
}

template<typename A, typename B>
RE_STD_CONSTEXPR auto operator-(const reverse_iterator<A>& _x,
                           const reverse_iterator<B>& _y)
    -> decltype(_y.base() - _x.base())
{
    return _y.base() - _x.base();
}


// ---- make_reverse_iterator ----
// Added in C++14 std; provided unconditionally on C++11+.

template<typename Iter>
RE_STD_CONSTEXPR reverse_iterator<Iter> make_reverse_iterator(Iter _it)
{
    return reverse_iterator<Iter>(_it);
}


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ITERATOR_REVERSE_ITERATOR_HPP
