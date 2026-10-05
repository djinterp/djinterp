/*******************************************************************************
* djinterp [re_std]                                           as_rvalue_view.hpp
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
*   Colocates re_std::views::as_rvalue as a range_adaptor_closure
* instance. Both forms work:
*       views::as_rvalue(vec)   // direct
*       vec | views::as_rvalue  // pipe
*
*
* path:      /inc/re_std/ranges/as_rvalue_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_AS_RVALUE_VIEW_HPP
#define RE_STD_RANGES_AS_RVALUE_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "../iterator/iter_move.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   AS_RVALUE_VIEW
// ===========================================================================

// as_rvalue_view<View>
//   class: lazy rvalue-projection of View. Dereferencing an
// iterator yields an rvalue reference to the underlying element,
// suitable for use as a move source.
template<typename View>
class as_rvalue_view : public view_interface<as_rvalue_view<View> >
{
public:
    typedef View   base_view;


private:
    View  m_base;


    // ---- compute the rvalue reference type ----
    typedef typename iterator_traits<
                          iterator_t<View>
                      >::reference                       underlying_reference;

    // rvalue_ref_t
    //   alias: the rvalue-reference projection of the underlying
    // iterator's reference type, routed through re_std::iter_move
    // (Phase R22). This both honours user customisations of
    // iter_move via ADL on the iterator type and gives us a single
    // point of truth for the rvalue projection (the same one used
    // by ranges-aware algorithms).
    typedef iter_rvalue_reference_t<iterator_t<View> >  rvalue_ref_t;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: wraps iterator_t<View>. operator* applies static_cast
    // to rvalue_ref_t on the underlying dereference.
    class iterator
    {
    public:
        typedef typename iterator_traits<
                              iterator_t<View>
                          >::iterator_category   iterator_category;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::value_type          value_type;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::difference_type     difference_type;

        typedef rvalue_ref_t                     reference;

        typedef void                             pointer;


    private:
        iterator_t<View>  m_it;


    public:
        RE_STD_CONSTEXPR
        iterator()
            : m_it()
        {}

        RE_STD_CONSTEXPR explicit
        iterator(
            iterator_t<View>  _it
        )
            : m_it(_it)
        {}


        RE_STD_CONSTEXPR iterator_t<View>
        base() const
        {
            return m_it;
        }


        // operator*
        //   function: routes through re_std::iter_move so user
        // customisations are picked up via ADL.
        RE_STD_CONSTEXPR reference
        operator*() const
        {
            return re_std::iter_move(m_it);
        }


        // operator++ (pre / post)
        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator++()
        {
            ++m_it;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_it;
            return tmp;
        }


        // operator-- (pre / post) — bidirectional+, SFINAE-lazy
        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator--()
        {
            --m_it;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_it;
            return tmp;
        }


        // random-access ops — SFINAE-lazy
        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_it += _n;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_it -= _n;
            return *this;
        }

        RE_STD_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_it + _n);
        }

        RE_STD_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_it - _n);
        }

        RE_STD_CONSTEXPR
        auto
        operator-(
            iterator const& _rhs
        ) const
            -> decltype(m_it - _rhs.m_it)
        {
            return (m_it - _rhs.m_it);
        }

        RE_STD_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return static_cast<reference>(m_it[_n]);
        }


        // comparisons (delegate to underlying iterator)
        RE_STD_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_it == _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return (m_it != _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return (m_it < _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return (m_it <= _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return (m_it > _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
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
        sentinel_t<View>  m_end;


    public:
        RE_STD_CONSTEXPR
        sentinel()
            : m_end()
        {}

        RE_STD_CONSTEXPR explicit
        sentinel(
            sentinel_t<View>  _e
        )
            : m_end(_e)
        {}


        RE_STD_CONSTEXPR sentinel_t<View>
        base() const
        {
            return m_end;
        }


        friend RE_STD_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return (_it.base() == _s.m_end);
        }

        friend RE_STD_CONSTEXPR bool
        operator!=(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return !(_it == _s);
        }

        friend RE_STD_CONSTEXPR bool
        operator==(
            sentinel const&  _s,
            iterator const&  _it
        )
        {
            return (_it == _s);
        }

        friend RE_STD_CONSTEXPR bool
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
    RE_STD_CONSTEXPR
    as_rvalue_view()
        : m_base()
    {}

    // value ctor
    RE_STD_CONSTEXPR
    as_rvalue_view(
        View  _base
    )
        : m_base(static_cast<View&&>(_base))
    {}


    // base
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }


    // begin / end
    RE_STD_CONSTEXPR_CPP14 iterator
    begin()
    {
        return iterator(re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR
    auto
    begin() const
        -> iterator
    {
        return iterator(re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR_CPP14 sentinel
    end()
    {
        return sentinel(re_std::end(m_base));
    }

    RE_STD_CONSTEXPR
    auto
    end() const
        -> sentinel
    {
        return sentinel(re_std::end(m_base));
    }


    // size — forwards to base when sized.
    RE_STD_CONSTEXPR_CPP14
    auto
    size()
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }

    RE_STD_CONSTEXPR
    auto
    size() const
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<as_rvalue_view<V>>
//   trait: borrowed iff the underlying View is itself borrowed.
// The iterator carries no extra state.
template<typename View>
struct enable_borrowed_range<as_rvalue_view<View> >
    : enable_borrowed_range<View>
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
        template<typename R>
        RE_STD_CONSTEXPR_INLINE
        as_rvalue_view<typename internal::all_dispatch<R>::type>
        operator()(
            R&&  _r
        ) const
        {
            typedef typename internal::all_dispatch<R>::type  view_type;
            return as_rvalue_view<view_type>(
                internal::all_dispatch<R>::call(static_cast<R&&>(_r))
            );
        }
    };

#if RE_STD_LANG_IS_CPP17_OR_HIGHER
    inline RE_STD_CONSTEXPR as_rvalue_fn as_rvalue = as_rvalue_fn();
#else
    static RE_STD_CONSTEXPR as_rvalue_fn as_rvalue = as_rvalue_fn();
#endif
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_AS_RVALUE_VIEW_HPP
