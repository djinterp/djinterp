/******************************************************************************
* djinterp [re_std]                                   basic_const_iterator.hpp
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
* path:      /inc/djinterp/re_std/ranges/basic_const_iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_BASIC_CONST_ITERATOR_
#define DJINTERP_RE_STD_ITERATOR_BASIC_CONST_ITERATOR_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./iterator_traits.hpp"


NS_RESTD


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
    template<typename _R>
    struct const_ref_projection
    {
    private:
        typedef typename remove_reference<_R>::type   referent;
        typedef typename add_const<referent>::type    const_referent;

    public:
        typedef typename conditional<
                              is_lvalue_reference<_R>::value,
                              typename add_lvalue_reference<const_referent>::type,
                              typename conditional<
                                          is_rvalue_reference<_R>::value,
                                          typename add_rvalue_reference<const_referent>::type,
                                          _R   // prvalue: keep as-is
                                      >::type
                          >::type type;
    };
}  // namespace internal


// iter_const_reference_t<_I>
//   alias: the const-projected reference type yielded by
// dereferencing a basic_const_iterator<_I>.
template<typename _I>
struct iter_const_reference
{
    typedef typename internal::const_ref_projection<
                          typename iterator_traits<_I>::reference
                      >::type type;
};

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
template<typename _I>
using iter_const_reference_t = typename iter_const_reference<_I>::type;
#endif


// ===========================================================================
// II.  BASIC_CONST_ITERATOR
// ===========================================================================

// basic_const_iterator<_I>
//   class: wraps _I and exposes its dereference as a const
// reference. All other operations delegate to the underlying _I.
template<typename _I>
class basic_const_iterator
{
public:
    typedef typename iterator_traits<_I>::iterator_category iterator_category;
    typedef typename iterator_traits<_I>::value_type        value_type;
    typedef typename iterator_traits<_I>::difference_type   difference_type;
    typedef iter_const_reference_t<_I>                      reference;
    typedef void                                            pointer;


private:
    _I  m_it;


public:
    // -------- ctors --------
    D_CONSTEXPR
    basic_const_iterator()
        : m_it()
    {}

    D_CONSTEXPR explicit
    basic_const_iterator(
        _I  _it
    )
        : m_it(_it)
    {}


    // -------- base accessor --------
    D_CONSTEXPR _I const&
    base() const D_NOEXCEPT
    {
        return m_it;
    }


    // -------- deref --------
    //   function: routes the underlying deref through the const
    // projection. For lvalue references, this is a const_cast-like
    // upgrade to const; for prvalues, this is a no-op cast.
    D_CONSTEXPR reference
    operator*() const
    {
        return static_cast<reference>(*m_it);
    }


    // -------- forward / bidirectional ops --------
    D_CONSTEXPR_INLINE basic_const_iterator&
    operator++()
    {
        ++m_it;
        return *this;
    }

    D_CONSTEXPR_INLINE basic_const_iterator
    operator++(int)
    {
        basic_const_iterator tmp = *this;
        ++m_it;
        return tmp;
    }

    D_CONSTEXPR_INLINE basic_const_iterator&
    operator--()
    {
        --m_it;
        return *this;
    }

    D_CONSTEXPR_INLINE basic_const_iterator
    operator--(int)
    {
        basic_const_iterator tmp = *this;
        --m_it;
        return tmp;
    }


    // -------- random-access ops --------
    D_CONSTEXPR_INLINE basic_const_iterator&
    operator+=(
        difference_type _n
    )
    {
        m_it += _n;
        return *this;
    }

    D_CONSTEXPR_INLINE basic_const_iterator&
    operator-=(
        difference_type _n
    )
    {
        m_it -= _n;
        return *this;
    }

    D_CONSTEXPR basic_const_iterator
    operator+(
        difference_type _n
    ) const
    {
        return basic_const_iterator(m_it + _n);
    }

    friend D_CONSTEXPR basic_const_iterator
    operator+(
        difference_type           _n,
        basic_const_iterator      _it
    )
    {
        return _it + _n;
    }

    D_CONSTEXPR basic_const_iterator
    operator-(
        difference_type _n
    ) const
    {
        return basic_const_iterator(m_it - _n);
    }

    D_CONSTEXPR
    auto
    operator-(
        basic_const_iterator const& _rhs
    ) const
        -> decltype(m_it - _rhs.m_it)
    {
        return m_it - _rhs.m_it;
    }

    D_CONSTEXPR reference
    operator[](
        difference_type _n
    ) const
    {
        return static_cast<reference>(m_it[_n]);
    }


    // -------- comparisons --------
    D_CONSTEXPR bool
    operator==(basic_const_iterator const& _r) const { return m_it == _r.m_it; }

    D_CONSTEXPR bool
    operator!=(basic_const_iterator const& _r) const { return m_it != _r.m_it; }

    D_CONSTEXPR bool
    operator<(basic_const_iterator const& _r)  const { return m_it < _r.m_it;  }

    D_CONSTEXPR bool
    operator<=(basic_const_iterator const& _r) const { return m_it <= _r.m_it; }

    D_CONSTEXPR bool
    operator>(basic_const_iterator const& _r)  const { return m_it > _r.m_it;  }

    D_CONSTEXPR bool
    operator>=(basic_const_iterator const& _r) const { return m_it >= _r.m_it; }


    // -------- cross-comparison with the underlying iterator --------
    //   function: lets basic_const_iterator compare against the
    // wrapped iterator type for sentinel comparisons in as_const_view.
    template<typename _S>
    D_CONSTEXPR bool
    operator==(_S const& _rhs) const
    {
        return m_it == _rhs;
    }

    template<typename _S>
    D_CONSTEXPR bool
    operator!=(_S const& _rhs) const
    {
        return m_it != _rhs;
    }

    template<typename _S>
    friend D_CONSTEXPR bool
    operator==(_S const& _lhs, basic_const_iterator const& _rhs)
    {
        return _lhs == _rhs.m_it;
    }

    template<typename _S>
    friend D_CONSTEXPR bool
    operator!=(_S const& _lhs, basic_const_iterator const& _rhs)
    {
        return _lhs != _rhs.m_it;
    }
};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_ITERATOR_BASIC_CONST_ITERATOR_
