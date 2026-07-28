/******************************************************************************
* djinterp [restd]                                             filter_view.hpp
*
* filter_view header:
*   Provides the C++20 lazy-filtering adaptor. filter_view<V, Pred>
* presents the elements of an underlying view V for which Pred
* returns true, lazily — Pred is invoked during iteration, not at
* view construction.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator class.
*   - Pred is stored by value (move-construction required), same
*     simplification as transform_view's function storage.
*   - The iterator holds a back-pointer to its parent filter_view
*     so dereferencing can re-invoke Pred during traversal. As a
*     consequence filter_view::iterator does not satisfy
*     borrowed_range — outliving the parent dangles.
*   - begin() is lazily cached: the first call scans forward from
*     restd::begin(base) until Pred returns true, and subsequent
*     calls return the cached iterator. The cache is mutable so
*     begin() can be const-callable.
*   - Iterator category is clamped to at-most bidirectional. C++20
*     allows random_access_iterator_tag on the underlying iterator
*     but filter_view's iterator can NEVER be random-access because
*     +n and -n cannot skip through unknown-count failed predicate
*     evaluations in O(1).
*
*   COLOCATED:
*   restd::views::filter(r, pred).
*
*
* path:      /inc/djinterp/restd/ranges/filter_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_FILTER_VIEW_
#define DJINTERP_RESTD_RANGES_FILTER_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL: ITERATOR-CATEGORY CLAMP
// ===========================================================================

NS_INTERNAL

// filter_iter_cat
//   trait: clamps the underlying iterator_category to at-most
// bidirectional. filter_view's iterator can be bidirectional when
// the underlying iterator is — operator-- scans backward past
// failing elements until one passes — but it can never be
// random-access regardless of the underlying.
template<typename _UnderlyingCat>
struct filter_iter_cat
{
    // Forward / input / output unchanged; bidi or stronger clamps to
    // bidirectional_iterator_tag.
    typedef typename conditional<
                         is_base_of<bidirectional_iterator_tag,
                                    _UnderlyingCat>::value,
                         bidirectional_iterator_tag,
                         _UnderlyingCat
                     >::type type;
};

NS_END  // internal


// ===========================================================================
// I.   FILTER_VIEW
// ===========================================================================

// filter_view<_View, _Pred>
//   class: lazy filter of _View by predicate _Pred. Only elements
// satisfying _Pred(*it) appear in the resulting view.
template<typename _View,
         typename _Pred>
class filter_view : public view_interface<filter_view<_View, _Pred> >
{
public:
    typedef _View   base_view;
    typedef _Pred   predicate_type;


private:
    _View                           m_base;
    internal::movable_box<_Pred>    m_pred;
    mutable bool                m_cache_init;
    mutable iterator_t<_View>   m_cache;


    // find_first
    //   function: scans forward from begin(base) for the first
    // element satisfying the predicate. Populates the cache.
    void
    find_first() const
    {
        if (m_cache_init)
        {
            return;
        }
        iterator_t<_View> it = restd::begin(m_base);
        sentinel_t<_View> e  = restd::end(m_base);
        while (it != e && !(*m_pred)(*it))
        {
            ++it;
        }
        m_cache      = it;
        m_cache_init = true;
    }


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: wraps iterator_t<_View> + parent back-pointer.
    // operator++ scans forward past failing elements; operator--
    // scans backward past failing elements (bidi only).
    class iterator
    {
    public:
        typedef typename internal::filter_iter_cat<
                    typename iterator_traits<
                                  iterator_t<_View>
                              >::iterator_category
                >::type                            iterator_category;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::value_type            value_type;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::difference_type       difference_type;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::pointer               pointer;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::reference             reference;


    private:
        iterator_t<_View>           m_it;
        filter_view const*          m_parent;


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_it(),
              m_parent(D_NULLPTR)
        {}

        // value ctor
        D_CONSTEXPR
        iterator(
            filter_view const*  _parent,
            iterator_t<_View>   _it
        )
            : m_it(_it),
              m_parent(_parent)
        {}


        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_it;
        }


        D_CONSTEXPR reference
        operator*() const
        {
            return *m_it;
        }


        // operator++ (pre)
        //   function: advances past one element, then scans
        // forward until the predicate accepts. Stops at the
        // underlying end.
        iterator&
        operator++()
        {
            sentinel_t<_View> e = restd::end(m_parent->m_base);
            ++m_it;
            while (m_it != e && !(*(m_parent->m_pred))(*m_it))
            {
                ++m_it;
            }
            return *this;
        }

        iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }


        // operator-- (pre, bidirectional only)
        //   function: scans backward until the predicate accepts.
        // The base of the underlying range is treated as a hard
        // wall — calling operator-- when *no* earlier element
        // satisfies the predicate is undefined behaviour (matches
        // the C++20 contract).
        iterator&
        operator--()
        {
            do {
                --m_it;
            } while (!(*(m_parent->m_pred))(*m_it));
            return *this;
        }

        iterator
        operator--(int)
        {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }


        // == / !=
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
    filter_view()
        : m_base(),
          m_pred(),
          m_cache_init(false),
          m_cache()
    {}

    // value ctor
    D_CONSTEXPR
    filter_view(
        _View  _base,
        _Pred  _pred
    )
        : m_base(static_cast<_View&&>(_base)),
          m_pred(static_cast<_Pred&&>(_pred)),
          m_cache_init(false),
          m_cache()
    {}


    // base
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    // pred
    //   function: returns a const reference to the stored predicate.
    // Non-standard accessor; useful for diagnostic / introspective
    // code.
    D_CONSTEXPR _Pred const&
    pred() const
    D_NOEXCEPT
    {
        return *m_pred;
    }


    // begin
    //   function: returns the cached "first accepted element"
    // iterator, populating the cache on the first call.
    iterator
    begin() const
    {
        find_first();
        return iterator(this, m_cache);
    }

    // end
    sentinel
    end() const
    {
        return sentinel(restd::end(m_base));
    }
};


// ===========================================================================
// II.  FILTER_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

// filter_closure
//   class: the bound form of views::filter. Holds a predicate
// and, when invoked, constructs a filter_view directly.
template<typename _Pred>
struct filter_closure : range_adaptor_closure<filter_closure<_Pred> >
{
    _Pred pred;

    D_CONSTEXPR
    filter_closure()
        : pred()
    {}

    D_CONSTEXPR explicit
    filter_closure(
        _Pred _p
    )
        : pred(static_cast<_Pred&&>(_p))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    filter_view<typename internal::all_dispatch<_R>::type, _Pred>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return filter_view<view_type, _Pred>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            pred
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::FILTER
// ===========================================================================

namespace views
{
    // views::filter(_r, _pred)  [direct form]
    template<typename _R,
             typename _Pred>
    D_CONSTEXPR_INLINE
    filter_view<typename internal::all_dispatch<_R>::type,
                typename decay<_Pred>::type>
    filter(
        _R&&    _r,
        _Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Pred>::type                pred_type;
        return filter_view<view_type, pred_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Pred&&>(_pred)
        );
    }

    // views::filter(_pred)  [bound form]
    template<typename _Pred>
    D_CONSTEXPR_INLINE
    internal::filter_closure<typename decay<_Pred>::type>
    filter(
        _Pred&& _pred
    )
    {
        return internal::filter_closure<typename decay<_Pred>::type>(
            static_cast<_Pred&&>(_pred)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_FILTER_VIEW_
