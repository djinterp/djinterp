/******************************************************************************
* djinterp [restd]                                          as_rvalue_view.hpp
*
* as_rvalue_view header:
*   Provides the C++23 rvalue-projection adaptor. as_rvalue_view<V>
* presents an underlying view V with each element exposed as an
* rvalue reference, so that iterating over it moves out of the
* underlying storage rather than copying. Equivalent to applying
* std::move to *it on every dereference.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - Does not depend on the (not-yet-shipped) ranges::iter_move
*     CPO — the rvalue reference is computed directly via
*     static_cast on the underlying iterator's reference. For an
*     underlying reference of T& this yields T&&; for T&& it remains
*     T&&; for a prvalue T it remains T (no meaningful move).
*   - enable_borrowed_range<as_rvalue_view<V>> inherits from
*     enable_borrowed_range<V> — the iterator carries no state
*     beyond the underlying iterator.
*
*   PIPE SYNTAX:
*   Colocates restd::views::as_rvalue as a range_adaptor_closure
* instance. Both forms work:
*       views::as_rvalue(vec)   // direct
*       vec | views::as_rvalue  // pipe
*
*
* path:      /inc/djinterp/restd/ranges/as_rvalue_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_AS_RVALUE_VIEW_
#define DJINTERP_RESTD_RANGES_AS_RVALUE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/iter_move.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   AS_RVALUE_VIEW
// ===========================================================================

// as_rvalue_view<_View>
//   class: lazy rvalue-projection of _View. Dereferencing an
// iterator yields an rvalue reference to the underlying element,
// suitable for use as a move source.
template<typename _View>
class as_rvalue_view : public view_interface<as_rvalue_view<_View> >
{
public:
    typedef _View   base_view;


private:
    _View  m_base;


    // ---- compute the rvalue reference type ----
    typedef typename iterator_traits<
                          iterator_t<_View>
                      >::reference                       underlying_reference;

    // rvalue_ref_t
    //   alias: the rvalue-reference projection of the underlying
    // iterator's reference type, routed through restd::iter_move
    // (Phase R22). This both honours user customisations of
    // iter_move via ADL on the iterator type and gives us a single
    // point of truth for the rvalue projection (the same one used
    // by ranges-aware algorithms).
    typedef iter_rvalue_reference_t<iterator_t<_View> >  rvalue_ref_t;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: wraps iterator_t<_View>. operator* applies static_cast
    // to rvalue_ref_t on the underlying dereference.
    class iterator
    {
    public:
        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::iterator_category   iterator_category;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::value_type          value_type;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::difference_type     difference_type;

        typedef rvalue_ref_t                     reference;

        typedef void                             pointer;


    private:
        iterator_t<_View>  m_it;


    public:
        D_CONSTEXPR
        iterator()
            : m_it()
        {}

        D_CONSTEXPR explicit
        iterator(
            iterator_t<_View>  _it
        )
            : m_it(_it)
        {}


        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_it;
        }


        // operator*
        //   function: routes through restd::iter_move so user
        // customisations are picked up via ADL.
        D_CONSTEXPR reference
        operator*() const
        {
            return restd::iter_move(m_it);
        }


        // operator++ (pre / post)
        D_CONSTEXPR_INLINE iterator&
        operator++()
        {
            ++m_it;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_it;
            return tmp;
        }


        // operator-- (pre / post) — bidirectional+, SFINAE-lazy
        D_CONSTEXPR_INLINE iterator&
        operator--()
        {
            --m_it;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_it;
            return tmp;
        }


        // random-access ops — SFINAE-lazy
        D_CONSTEXPR_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_it += _n;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_it -= _n;
            return *this;
        }

        D_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_it + _n);
        }

        D_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_it - _n);
        }

        D_CONSTEXPR
        auto
        operator-(
            iterator const& _rhs
        ) const
            -> decltype(m_it - _rhs.m_it)
        {
            return (m_it - _rhs.m_it);
        }

        D_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return static_cast<reference>(m_it[_n]);
        }


        // comparisons (delegate to underlying iterator)
        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_it == _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return (m_it != _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return (m_it < _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return (m_it <= _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return (m_it > _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator>=(
            iterator const& _rhs
        ) const
        {
            return (m_it >= _rhs.m_it);
        }
    };


    // =======================================================
    // I.B   NESTED SENTINEL
    // =======================================================

    class sentinel
    {
    private:
        sentinel_t<_View>  m_end;


    public:
        D_CONSTEXPR
        sentinel()
            : m_end()
        {}

        D_CONSTEXPR explicit
        sentinel(
            sentinel_t<_View>  _e
        )
            : m_end(_e)
        {}


        D_CONSTEXPR sentinel_t<_View>
        base() const
        {
            return m_end;
        }


        friend D_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return (_it.base() == _s.m_end);
        }

        friend D_CONSTEXPR bool
        operator!=(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return !(_it == _s);
        }

        friend D_CONSTEXPR bool
        operator==(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return (_it == _s);
        }

        friend D_CONSTEXPR bool
        operator!=(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return !(_it == _s);
        }
    };


public:
    // default ctor
    D_CONSTEXPR
    as_rvalue_view()
        : m_base()
    {}

    // value ctor
    D_CONSTEXPR
    as_rvalue_view(
        _View  _base
    )
        : m_base(static_cast<_View&&>(_base))
    {}


    // base
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }


    // begin / end
    D_CONSTEXPR iterator
    begin()
    {
        return iterator(restd::begin(m_base));
    }

    D_CONSTEXPR
    auto
    begin() const
        -> iterator
    {
        return iterator(restd::begin(m_base));
    }

    D_CONSTEXPR sentinel
    end()
    {
        return sentinel(restd::end(m_base));
    }

    D_CONSTEXPR
    auto
    end() const
        -> sentinel
    {
        return sentinel(restd::end(m_base));
    }


    // size — forwards to base when sized.
    D_CONSTEXPR
    auto
    size()
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }

    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<as_rvalue_view<V>>
//   trait: borrowed iff the underlying _View is itself borrowed.
// The iterator carries no extra state.
template<typename _View>
struct enable_borrowed_range<as_rvalue_view<_View> >
    : enable_borrowed_range<_View>
{};


// ===========================================================================
// III. VIEWS::AS_RVALUE (pipe-enabled closure-fn)
// ===========================================================================

namespace views
{
    // as_rvalue_fn
    //   class: closure-fn for as_rvalue. Parameter-less, so the
    // instance itself is the closure (rather than a function that
    // returns one).
    struct as_rvalue_fn : range_adaptor_closure<as_rvalue_fn>
    {
        template<typename _R>
        D_CONSTEXPR_INLINE
        as_rvalue_view<typename internal::all_dispatch<_R>::type>
        operator()(
            _R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<_R>::type  view_type;
            return as_rvalue_view<view_type>(
                internal::all_dispatch<_R>::call(static_cast<_R&&>(_r))
            );
        }
    };

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    inline D_CONSTEXPR as_rvalue_fn as_rvalue = as_rvalue_fn();
#else
    static D_CONSTEXPR as_rvalue_fn as_rvalue = as_rvalue_fn();
#endif
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_AS_RVALUE_VIEW_
