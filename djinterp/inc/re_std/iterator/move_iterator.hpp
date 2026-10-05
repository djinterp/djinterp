/*******************************************************************************
* djinterp [re_std]                                            move_iterator.hpp
*
* move_iterator class header:
* iterator adaptor that wraps a base iterator and modifies dereference
* to yield rvalue references (xvalues) instead of lvalue references,
* so that algorithms operating on the adapted range will move-construct
* destination elements rather than copy.
*
* THE TWO DEREFERENCE PATHS:
*   move_iterator's reference type depends on the wrapped iterator's
*   reference type:
*     - if Iter::reference is a real reference type (T& or T const&),
*       move_iterator::reference = remove_reference<...>::type&&
*       (an xvalue when dereferenced).
*     - if Iter::reference is a value type (e.g. proxy iterator),
*       move_iterator::reference = Iter::reference UNCHANGED.
*       Casting a temporary to && would be undefined.
*
*   This matches the C++17 std::move_iterator behaviour exactly. Pre-
*   C++17 std unconditionally applied the && cast and broke proxies;
*   re_std does not reproduce that footgun.
*
* surface:
*   - default ctor, value ctor, converting copy ctor from
*     move_iterator<U>
*   - operator*, operator->, operator[]
*   - operator++, --, +=, -=, +, -
*   - base()
*   - the standard six relational operators
*
* iterator_category is preserved from the wrapped iterator (so a
* move_iterator over a random-access iterator is itself random-access).
* Only the dereference semantics change.
*
*
* path:      /inc/re_std/iterator/move_iterator.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_MOVE_ITERATOR_HPP
#define RE_STD_ITERATOR_MOVE_ITERATOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/utility/move.hpp"


namespace re_std
{
namespace internal
{

    // move_reference_of
    //   trait: computes move_iterator<Iter>::reference per the C++17 rule.
    //   When Iter::reference is a true reference type (T& or T const&),
    //   strip the reference and add &&. Otherwise pass the value type
    //   through unchanged (proxy iterator case).
    //
    //   Implementation note: we do this without re_std::is_reference or
    //   re_std::remove_reference traits, since this header tries to
    //   minimise its own dependency surface. The trick: a partial
    //   specialisation matches "T&"; the primary catches everything
    //   else (value types).

    template<typename R>
    struct move_reference_of
    {
        // Primary: pass-through (proxy / by-value iterator).
        typedef R type;
    };

    template<typename T>
    struct move_reference_of<T&>
    {
        // Reference case: produce T&& (an xvalue when returned).
        typedef T&& type;
    };

}  // internal
template<typename Iter>
class move_iterator
{
public:
    typedef Iter                                              iterator_type;
    typedef typename iterator_traits<Iter>::iterator_category iterator_category;
    typedef typename iterator_traits<Iter>::value_type        value_type;
    typedef typename iterator_traits<Iter>::difference_type   difference_type;
    typedef Iter                                              pointer;

    // The crucial part: reference is conditional on the wrapped
    // iterator's reference shape.
    typedef typename internal::move_reference_of
    <
        typename iterator_traits<Iter>::reference
    >::type                                                    reference;

    #if RE_STD_LANG_IS_CPP20_OR_HIGHER
        // C++20: move_iterator carries an input_iterator_concept that
        // is always input_iterator_tag (move iterators don't qualify
        // as forward — moving consumes the source).
        // We expose iterator_concept conditionally.
        typedef input_iterator_tag                             iterator_concept;
    #endif

protected:
    Iter current;

public:
    // ---- constructors ----

    RE_STD_CONSTEXPR move_iterator()
        : current() {}

    RE_STD_CONSTEXPR explicit move_iterator(iterator_type _x)
        : current(_x) {}

    template<typename U>
    RE_STD_CONSTEXPR move_iterator(const move_iterator<U>& _o)
        : current(_o.base()) {}

    template<typename U>
    RE_STD_CONSTEXPR_CPP14 move_iterator&
    operator=(const move_iterator<U>& _o)
    {
        current = _o.base();
        return *this;
    }

    // ---- access ----

    RE_STD_CONSTEXPR iterator_type base() const { return current; }

    RE_STD_CONSTEXPR reference operator*() const
    {
        // The static_cast<reference> handles both branches of
        // move_reference_of: for the reference branch it casts T& to
        // T&& (xvalue), for the value branch it's an identity cast.
        return static_cast<reference>(*current);
    }

    // operator-> returns the underlying iterator. The standard
    // describes this as "deprecated in C++20" and removed in C++23,
    // because *m + arrow doesn't compose meaningfully on a move
    // iterator (you'd be calling -> on an xvalue). We keep it for
    // C++11..C++20 compatibility.
    RE_STD_CONSTEXPR pointer operator->() const { return current; }

    RE_STD_CONSTEXPR reference operator[](difference_type _n) const
    {
        return static_cast<reference>(current[_n]);
    }

    // ---- arithmetic ----

    RE_STD_CONSTEXPR_CPP14 move_iterator& operator++()
    {
        ++current;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 move_iterator operator++(int)
    {
        move_iterator _r = *this;
        ++current;
        return _r;
    }

    RE_STD_CONSTEXPR_CPP14 move_iterator& operator--()
    {
        --current;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 move_iterator operator--(int)
    {
        move_iterator _r = *this;
        --current;
        return _r;
    }

    RE_STD_CONSTEXPR move_iterator
    operator+(difference_type _n) const
    {
        return move_iterator(current + _n);
    }

    RE_STD_CONSTEXPR_CPP14 move_iterator&
    operator+=(difference_type _n)
    {
        current += _n;
        return *this;
    }

    RE_STD_CONSTEXPR move_iterator
    operator-(difference_type _n) const
    {
        return move_iterator(current - _n);
    }

    RE_STD_CONSTEXPR_CPP14 move_iterator&
    operator-=(difference_type _n)
    {
        current -= _n;
        return *this;
    }
};


// ---- non-member relational ----
//
// move_iterator's relational ops do NOT flip ordering (unlike
// reverse_iterator). They forward to the base iterators directly.

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator==(const move_iterator<A>& _x,
                            const move_iterator<B>& _y)
{
    return _x.base() == _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator!=(const move_iterator<A>& _x,
                            const move_iterator<B>& _y)
{
    return _x.base() != _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator<(const move_iterator<A>& _x,
                           const move_iterator<B>& _y)
{
    return _x.base() < _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator>(const move_iterator<A>& _x,
                           const move_iterator<B>& _y)
{
    return _x.base() > _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator<=(const move_iterator<A>& _x,
                            const move_iterator<B>& _y)
{
    return _x.base() <= _y.base();
}

template<typename A, typename B>
RE_STD_CONSTEXPR bool operator>=(const move_iterator<A>& _x,
                            const move_iterator<B>& _y)
{
    return _x.base() >= _y.base();
}


// ---- non-member arithmetic ----

template<typename Iter>
RE_STD_CONSTEXPR move_iterator<Iter>
operator+(typename move_iterator<Iter>::difference_type _n,
          const move_iterator<Iter>& _r)
{
    return move_iterator<Iter>(_r.base() + _n);
}

template<typename A, typename B>
RE_STD_CONSTEXPR auto operator-(const move_iterator<A>& _x,
                           const move_iterator<B>& _y)
    -> decltype(_x.base() - _y.base())
{
    return _x.base() - _y.base();
}


// ---- make_move_iterator ----

template<typename Iter>
RE_STD_CONSTEXPR move_iterator<Iter> make_move_iterator(Iter _it)
{
    return move_iterator<Iter>(_it);
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_MOVE_ITERATOR_HPP
