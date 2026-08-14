/***********************************************************************
* restd                                                   reverse_iterator.hpp
*
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
*     Local D_CONSTEXPR_CPP14 macro below; gated on
*     D_ENV_LANG_IS_CPP14_OR_HIGHER.
*
*
* path:      /inc/djinterp/re_std/iterator/reverse_iterator.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_REVERSE_ITERATOR_
#define RESTD_ITERATOR_REVERSE_ITERATOR_ 1

#include "djinterp.hpp"

#include "restd/iterator/iterator_traits.hpp"


// D_CONSTEXPR_CPP14 — `constexpr` on C++14+, empty on C++11.
//   Used for the mutating-then-returning-this ops in reverse_iterator.
//   Local definition pending an entry in the global qualifier macro
//   table (RESTD_AGENT_README.md). Guarded so this file compiles
//   whether or not the global macro lands.
#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{

template<typename _Iter>
class reverse_iterator
{
public:
    typedef _Iter                                              iterator_type;
    typedef typename iterator_traits<_Iter>::iterator_category iterator_category;
    typedef typename iterator_traits<_Iter>::value_type        value_type;
    typedef typename iterator_traits<_Iter>::difference_type   difference_type;
    typedef typename iterator_traits<_Iter>::pointer           pointer;
    typedef typename iterator_traits<_Iter>::reference         reference;

protected:
    _Iter current;

public:
    // ---- constructors ----

    D_CONSTEXPR reverse_iterator()
        : current() {}

    D_CONSTEXPR explicit reverse_iterator(iterator_type _x)
        : current(_x) {}

    template<typename _U>
    D_CONSTEXPR reverse_iterator(const reverse_iterator<_U>& _o)
        : current(_o.base()) {}

    template<typename _U>
    D_CONSTEXPR_CPP14 reverse_iterator&
    operator=(const reverse_iterator<_U>& _o)
    {
        current = _o.base();
        return *this;
    }

    // ---- access ----

    D_CONSTEXPR iterator_type base() const { return current; }

    D_CONSTEXPR_CPP14 reference operator*() const
    {
        // Off-by-one: r.current points one past the logical element.
        _Iter _tmp = current;
        return *--_tmp;
    }

    D_CONSTEXPR_CPP14 pointer operator->() const
    {
        _Iter _tmp = current;
        --_tmp;
        return to_pointer(_tmp);
    }

    D_CONSTEXPR_CPP14 reference operator[](difference_type _n) const
    {
        return *(*this + _n);
    }

    // ---- arithmetic ----

    D_CONSTEXPR_CPP14 reverse_iterator& operator++()
    {
        --current;
        return *this;
    }

    D_CONSTEXPR_CPP14 reverse_iterator operator++(int)
    {
        reverse_iterator _r = *this;
        --current;
        return _r;
    }

    D_CONSTEXPR_CPP14 reverse_iterator& operator--()
    {
        ++current;
        return *this;
    }

    D_CONSTEXPR_CPP14 reverse_iterator operator--(int)
    {
        reverse_iterator _r = *this;
        ++current;
        return _r;
    }

    D_CONSTEXPR_CPP14 reverse_iterator
    operator+(difference_type _n) const
    {
        return reverse_iterator(current - _n);
    }

    D_CONSTEXPR_CPP14 reverse_iterator&
    operator+=(difference_type _n)
    {
        current -= _n;
        return *this;
    }

    D_CONSTEXPR_CPP14 reverse_iterator
    operator-(difference_type _n) const
    {
        return reverse_iterator(current + _n);
    }

    D_CONSTEXPR_CPP14 reverse_iterator&
    operator-=(difference_type _n)
    {
        current += _n;
        return *this;
    }

private:
    // operator-> helper: raw pointer pass-through, class iterator
    // dispatch via member operator->.
    template<typename _T>
    static D_CONSTEXPR _T* to_pointer(_T* _p) { return _p; }

    template<typename _It>
    static D_CONSTEXPR_CPP14 pointer
    to_pointer(_It _it) { return _it.operator->(); }
};


// ---- non-member relational ----

template<typename _A, typename _B>
D_CONSTEXPR bool operator==(const reverse_iterator<_A>& _x,
                            const reverse_iterator<_B>& _y)
{
    return _x.base() == _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator!=(const reverse_iterator<_A>& _x,
                            const reverse_iterator<_B>& _y)
{
    return _x.base() != _y.base();
}

// FLIPPED ordering: r1 < r2 iff base(r1) > base(r2). The reverse
// iterator at the larger base is the "earlier" one in reverse order.
template<typename _A, typename _B>
D_CONSTEXPR bool operator<(const reverse_iterator<_A>& _x,
                           const reverse_iterator<_B>& _y)
{
    return _x.base() > _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator>(const reverse_iterator<_A>& _x,
                           const reverse_iterator<_B>& _y)
{
    return _x.base() < _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator<=(const reverse_iterator<_A>& _x,
                            const reverse_iterator<_B>& _y)
{
    return _x.base() >= _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator>=(const reverse_iterator<_A>& _x,
                            const reverse_iterator<_B>& _y)
{
    return _x.base() <= _y.base();
}


// ---- non-member arithmetic ----

template<typename _Iter>
D_CONSTEXPR reverse_iterator<_Iter>
operator+(typename reverse_iterator<_Iter>::difference_type _n,
          const reverse_iterator<_Iter>& _r)
{
    return reverse_iterator<_Iter>(_r.base() - _n);
}

template<typename _A, typename _B>
D_CONSTEXPR auto operator-(const reverse_iterator<_A>& _x,
                           const reverse_iterator<_B>& _y)
    -> decltype(_y.base() - _x.base())
{
    return _y.base() - _x.base();
}


// ---- make_reverse_iterator ----
// Added in C++14 std; provided unconditionally on C++11+.

template<typename _Iter>
D_CONSTEXPR reverse_iterator<_Iter> make_reverse_iterator(_Iter _it)
{
    return reverse_iterator<_Iter>(_it);
}


}  // namespace restd

#endif  // RESTD_ITERATOR_REVERSE_ITERATOR_
