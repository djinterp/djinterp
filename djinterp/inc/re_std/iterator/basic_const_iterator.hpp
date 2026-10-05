/*******************************************************************************
* djinterp [re_std]                                     basic_const_iterator.hpp
*
* basic_const_iterator header:
*   Provides the C++23 re_std::basic_const_iterator<I> class. Wraps
* an underlying iterator I and presents its dereference as a
* const reference. Used internally by ranges::as_const_view and by
* the constant_range concept; usable directly by code that wants a
* const projection over an iterator.
*
*   REFERENCE-TYPE MAPPING:
*   - Underlying *it is T&        -> basic_const_iterator's *it is T const&
*   - Underlying *it is T&&       -> basic_const_iterator's *it is T const&&
*   - Underlying *it is T (prvalue, e.g. proxy iterator)
*                                 -> basic_const_iterator's *it is T
*     (prvalues are already immutable; the wrapper is a no-op cast)
*
*   COLOCATED:
*   - iter_const_reference_t<I> alias for the projected reference.
*
*   PORTABILITY:
*   - C++11+; depends on type_traits + iterator_traits only.
*   - Inherits underlying iterator_category. RA operations are
*     present unconditionally and lazy-instantiated; they compile
*     only when the underlying iterator supports them.
*
*
* path:      /inc/re_std/iterator/basic_const_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_BASIC_CONST_ITERATOR_HPP
#define RE_STD_ITERATOR_BASIC_CONST_ITERATOR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./iterator_traits.hpp"


namespace re_std
{
// ===========================================================================
// I.   ITER_CONST_REFERENCE_T HELPER
// ===========================================================================

namespace internal
{

    // const_ref_projection<R>
    //   trait: maps the underlying iterator's reference type R to
    // its const-projected analogue.
    //   - R = T&     -> T const&
    //   - R = T&&    -> T const&&
    //   - R = T      -> T  (prvalues remain prvalues)
    template<typename R>
    struct const_ref_projection
    {
    private:
        typedef typename remove_reference<R>::type   referent;
        typedef typename add_const<referent>::type    const_referent;

    public:
        typedef typename conditional<
                              is_lvalue_reference<R>::value,
                              typename add_lvalue_reference<const_referent>::type,
                              typename conditional<
                                          is_rvalue_reference<R>::value,
                                          typename add_rvalue_reference<const_referent>::type,
                                          R   // prvalue: keep as-is
                                      >::type
                          >::type type;
    };
}  // internal
// iter_const_reference_t<I>
//   alias: the const-projected reference type yielded by
// dereferencing a basic_const_iterator<I>.
template<typename I>
struct iter_const_reference
{
    typedef typename internal::const_ref_projection<
                          typename iterator_traits<I>::reference
                      >::type type;
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER
template<typename I>
using iter_const_reference_t = typename iter_const_reference<I>::type;
#endif


// ===========================================================================
// II.  BASIC_CONST_ITERATOR
// ===========================================================================

// basic_const_iterator<I>
//   class: wraps I and exposes its dereference as a const
// reference. All other operations delegate to the underlying I.
template<typename I>
class basic_const_iterator
{
public:
    typedef typename iterator_traits<I>::iterator_category iterator_category;
    typedef typename iterator_traits<I>::value_type        value_type;
    typedef typename iterator_traits<I>::difference_type   difference_type;
    typedef iter_const_reference_t<I>                      reference;
    typedef void                                            pointer;


private:
    I  m_it;


public:
    // -------- ctors --------
    RE_STD_CONSTEXPR
    basic_const_iterator()
        : m_it()
    {}

    RE_STD_CONSTEXPR explicit
    basic_const_iterator(
        I  _it
    )
        : m_it(_it)
    {}


    // -------- base accessor --------
    RE_STD_CONSTEXPR I const&
    base() const RE_STD_NOEXCEPT
    {
        return m_it;
    }


    // -------- deref --------
    //   function: routes the underlying deref through the const
    // projection. For lvalue references, this is a const_cast-like
    // upgrade to const; for prvalues, this is a no-op cast.
    RE_STD_CONSTEXPR reference
    operator*() const
    {
        return static_cast<reference>(*m_it);
    }


    // -------- forward / bidirectional ops --------
    RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE basic_const_iterator&
    operator++()
    {
        ++m_it;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE basic_const_iterator
    operator++(int)
    {
        basic_const_iterator tmp = *this;
        ++m_it;
        return tmp;
    }

    RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE basic_const_iterator&
    operator--()
    {
        --m_it;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE basic_const_iterator
    operator--(int)
    {
        basic_const_iterator tmp = *this;
        --m_it;
        return tmp;
    }


    // -------- random-access ops --------
    RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE basic_const_iterator&
    operator+=(
        difference_type _n
    )
    {
        m_it += _n;
        return *this;
    }

    RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE basic_const_iterator&
    operator-=(
        difference_type _n
    )
    {
        m_it -= _n;
        return *this;
    }

    RE_STD_CONSTEXPR basic_const_iterator
    operator+(
        difference_type _n
    ) const
    {
        return basic_const_iterator(m_it + _n);
    }

    friend RE_STD_CONSTEXPR basic_const_iterator
    operator+(
        difference_type           _n,
        basic_const_iterator      _it
    )
    {
        return _it + _n;
    }

    RE_STD_CONSTEXPR basic_const_iterator
    operator-(
        difference_type _n
    ) const
    {
        return basic_const_iterator(m_it - _n);
    }

    RE_STD_CONSTEXPR
    auto
    operator-(
        basic_const_iterator const& _rhs
    ) const
        -> decltype(m_it - _rhs.m_it)
    {
        return m_it - _rhs.m_it;
    }

    RE_STD_CONSTEXPR reference
    operator[](
        difference_type _n
    ) const
    {
        return static_cast<reference>(m_it[_n]);
    }


    // -------- comparisons --------
    RE_STD_CONSTEXPR bool
    operator==(basic_const_iterator const& _r) const { return m_it == _r.m_it; }

    RE_STD_CONSTEXPR bool
    operator!=(basic_const_iterator const& _r) const { return m_it != _r.m_it; }

    RE_STD_CONSTEXPR bool
    operator<(basic_const_iterator const& _r)  const { return m_it < _r.m_it;  }

    RE_STD_CONSTEXPR bool
    operator<=(basic_const_iterator const& _r) const { return m_it <= _r.m_it; }

    RE_STD_CONSTEXPR bool
    operator>(basic_const_iterator const& _r)  const { return m_it > _r.m_it;  }

    RE_STD_CONSTEXPR bool
    operator>=(basic_const_iterator const& _r) const { return m_it >= _r.m_it; }


    // -------- cross-comparison with the underlying iterator --------
    //   function: lets basic_const_iterator compare against the
    // wrapped iterator type for sentinel comparisons in as_const_view.
    template<typename S>
    RE_STD_CONSTEXPR bool
    operator==(S const& _rhs) const
    {
        return m_it == _rhs;
    }

    template<typename S>
    RE_STD_CONSTEXPR bool
    operator!=(S const& _rhs) const
    {
        return m_it != _rhs;
    }

    template<typename S>
    friend RE_STD_CONSTEXPR bool
    operator==(S const& _lhs, basic_const_iterator const& _rhs)
    {
        return _lhs == _rhs.m_it;
    }

    template<typename S>
    friend RE_STD_CONSTEXPR bool
    operator!=(S const& _lhs, basic_const_iterator const& _rhs)
    {
        return _lhs != _rhs.m_it;
    }
};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_ITERATOR_BASIC_CONST_ITERATOR_HPP
