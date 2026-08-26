/***********************************************************************
* re_std                                                     move_iterator.hpp
*
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
* path:      /inc/djinterp/re_std/iterator/move_iterator.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_MOVE_ITERATOR_
#define DJINTERP_RE_STD_ITERATOR_MOVE_ITERATOR_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/utility/move.hpp"


// D_CONSTEXPR_CPP14 — `constexpr` on C++14+, empty on C++11.
//   See reverse_iterator.hpp for the rationale; reproduced here so
//   each module is self-contained.
#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace re_std
{
namespace internal
{

    // ---- move_reference_of ----
    //   Computes move_iterator<Iter>::reference per the C++17 rule.
    //   When Iter::reference is a true reference type (T& or T const&),
    //   strip the reference and add &&. Otherwise pass the value type
    //   through unchanged (proxy iterator case).
    //
    //   Implementation note: we do this without re_std::is_reference or
    //   re_std::remove_reference traits, since this header tries to
    //   minimise its own dependency surface. The trick: a partial
    //   specialisation matches "T&"; the primary catches everything
    //   else (value types).

    template<typename _R>
    struct move_reference_of
    {
        // Primary: pass-through (proxy / by-value iterator).
        typedef _R type;
    };

    template<typename _T>
    struct move_reference_of<_T&>
    {
        // Reference case: produce T&& (an xvalue when returned).
        typedef _T&& type;
    };

}  // namespace internal


template<typename _Iter>
class move_iterator
{
public:
    typedef _Iter                                              iterator_type;
    typedef typename iterator_traits<_Iter>::iterator_category iterator_category;
    typedef typename iterator_traits<_Iter>::value_type        value_type;
    typedef typename iterator_traits<_Iter>::difference_type   difference_type;
    typedef _Iter                                              pointer;

    // The crucial part: reference is conditional on the wrapped
    // iterator's reference shape.
    typedef typename internal::move_reference_of
    <
        typename iterator_traits<_Iter>::reference
    >::type                                                    reference;

    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        // C++20: move_iterator carries an input_iterator_concept that
        // is always input_iterator_tag (move iterators don't qualify
        // as forward — moving consumes the source).
        // We expose iterator_concept conditionally.
        typedef input_iterator_tag                             iterator_concept;
    #endif

protected:
    _Iter current;

public:
    // ---- constructors ----

    D_CONSTEXPR move_iterator()
        : current() {}

    D_CONSTEXPR explicit move_iterator(iterator_type _x)
        : current(_x) {}

    template<typename _U>
    D_CONSTEXPR move_iterator(const move_iterator<_U>& _o)
        : current(_o.base()) {}

    template<typename _U>
    D_CONSTEXPR_CPP14 move_iterator&
    operator=(const move_iterator<_U>& _o)
    {
        current = _o.base();
        return *this;
    }

    // ---- access ----

    D_CONSTEXPR iterator_type base() const { return current; }

    D_CONSTEXPR reference operator*() const
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
    D_CONSTEXPR pointer operator->() const { return current; }

    D_CONSTEXPR reference operator[](difference_type _n) const
    {
        return static_cast<reference>(current[_n]);
    }

    // ---- arithmetic ----

    D_CONSTEXPR_CPP14 move_iterator& operator++()
    {
        ++current;
        return *this;
    }

    D_CONSTEXPR_CPP14 move_iterator operator++(int)
    {
        move_iterator _r = *this;
        ++current;
        return _r;
    }

    D_CONSTEXPR_CPP14 move_iterator& operator--()
    {
        --current;
        return *this;
    }

    D_CONSTEXPR_CPP14 move_iterator operator--(int)
    {
        move_iterator _r = *this;
        --current;
        return _r;
    }

    D_CONSTEXPR move_iterator
    operator+(difference_type _n) const
    {
        return move_iterator(current + _n);
    }

    D_CONSTEXPR_CPP14 move_iterator&
    operator+=(difference_type _n)
    {
        current += _n;
        return *this;
    }

    D_CONSTEXPR move_iterator
    operator-(difference_type _n) const
    {
        return move_iterator(current - _n);
    }

    D_CONSTEXPR_CPP14 move_iterator&
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

template<typename _A, typename _B>
D_CONSTEXPR bool operator==(const move_iterator<_A>& _x,
                            const move_iterator<_B>& _y)
{
    return _x.base() == _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator!=(const move_iterator<_A>& _x,
                            const move_iterator<_B>& _y)
{
    return _x.base() != _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator<(const move_iterator<_A>& _x,
                           const move_iterator<_B>& _y)
{
    return _x.base() < _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator>(const move_iterator<_A>& _x,
                           const move_iterator<_B>& _y)
{
    return _x.base() > _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator<=(const move_iterator<_A>& _x,
                            const move_iterator<_B>& _y)
{
    return _x.base() <= _y.base();
}

template<typename _A, typename _B>
D_CONSTEXPR bool operator>=(const move_iterator<_A>& _x,
                            const move_iterator<_B>& _y)
{
    return _x.base() >= _y.base();
}


// ---- non-member arithmetic ----

template<typename _Iter>
D_CONSTEXPR move_iterator<_Iter>
operator+(typename move_iterator<_Iter>::difference_type _n,
          const move_iterator<_Iter>& _r)
{
    return move_iterator<_Iter>(_r.base() + _n);
}

template<typename _A, typename _B>
D_CONSTEXPR auto operator-(const move_iterator<_A>& _x,
                           const move_iterator<_B>& _y)
    -> decltype(_x.base() - _y.base())
{
    return _x.base() - _y.base();
}


// ---- make_move_iterator ----

template<typename _Iter>
D_CONSTEXPR move_iterator<_Iter> make_move_iterator(_Iter _it)
{
    return move_iterator<_Iter>(_it);
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_MOVE_ITERATOR_
