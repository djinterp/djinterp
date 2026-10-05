/*******************************************************************************
* djinterp [re_std]                                              filter_view.hpp
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
*     re_std::begin(base) until Pred returns true, and subsequent
*     calls return the cached iterator. The cache is mutable so
*     begin() can be const-callable.
*   - Iterator category is clamped to at-most bidirectional. C++20
*     allows random_access_iterator_tag on the underlying iterator
*     but filter_view's iterator can NEVER be random-access because
*     +n and -n cannot skip through unknown-count failed predicate
*     evaluations in O(1).
*
*   COLOCATED:
*   re_std::views::filter(r, pred).
*
*
* path:      /inc/re_std/ranges/filter_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_FILTER_VIEW_HPP
#define RE_STD_RANGES_FILTER_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL: ITERATOR-CATEGORY CLAMP
// ===========================================================================

namespace internal
{

// filter_iter_cat
//   trait: clamps the underlying iterator_category to at-most
// bidirectional. filter_view's iterator can be bidirectional when
// the underlying iterator is — operator-- scans backward past
// failing elements until one passes — but it can never be
// random-access regardless of the underlying.
template<typename UnderlyingCat>
struct filter_iter_cat
{
    // Forward / input / output unchanged; bidi or stronger clamps to
    // bidirectional_iterator_tag.
    typedef typename conditional<
                         is_base_of<bidirectional_iterator_tag,
                                    UnderlyingCat>::value,
                         bidirectional_iterator_tag,
                         UnderlyingCat
                     >::type type;
};

}  // internal


// ===========================================================================
// I.   FILTER_VIEW
// ===========================================================================

// filter_view<View, Pred>
//   class: lazy filter of View by predicate Pred. Only elements
// satisfying Pred(*it) appear in the resulting view.
template<typename View,
         typename Pred>
class filter_view : public view_interface<filter_view<View, Pred> >
{
public:
    typedef View   base_view;
    typedef Pred   predicate_type;


private:
    View                           m_base;
    internal::movable_box<Pred>    m_pred;
    mutable bool                m_cache_init;
    mutable iterator_t<View>   m_cache;


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
        iterator_t<View> it = re_std::begin(m_base);
        sentinel_t<View> e  = re_std::end(m_base);
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
    //   class: wraps iterator_t<View> + parent back-pointer.
    // operator++ scans forward past failing elements; operator--
    // scans backward past failing elements (bidi only).
    class iterator
    {
    public:
        typedef typename internal::filter_iter_cat<
                    typename iterator_traits<
                                  iterator_t<View>
                              >::iterator_category
                >::type                            iterator_category;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::value_type            value_type;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::difference_type       difference_type;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::pointer               pointer;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::reference             reference;


    private:
        iterator_t<View>           m_it;
        filter_view const*          m_parent;


    public:
        // default ctor
        RE_STD_CONSTEXPR
        iterator()
            : m_it(),
              m_parent(RE_STD_NULLPTR)
        {}

        // value ctor
        RE_STD_CONSTEXPR
        iterator(
            filter_view const*  _parent,
            iterator_t<View>   _it
        )
            : m_it(_it),
              m_parent(_parent)
        {}


        RE_STD_CONSTEXPR iterator_t<View>
        base() const
        {
            return m_it;
        }


        RE_STD_CONSTEXPR reference
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
            sentinel_t<View> e = re_std::end(m_parent->m_base);
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
    filter_view()
        : m_base(),
          m_pred(),
          m_cache_init(false),
          m_cache()
    {}

    // value ctor
    RE_STD_CONSTEXPR
    filter_view(
        View  _base,
        Pred  _pred
    )
        : m_base(static_cast<View&&>(_base)),
          m_pred(static_cast<Pred&&>(_pred)),
          m_cache_init(false),
          m_cache()
    {}


    // base
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }

    // pred
    //   function: returns a const reference to the stored predicate.
    // Non-standard accessor; useful for diagnostic / introspective
    // code.
    RE_STD_CONSTEXPR Pred const&
    pred() const
    RE_STD_NOEXCEPT
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
        return sentinel(re_std::end(m_base));
    }
};


// ===========================================================================
// II.  FILTER_CLOSURE (bound form for pipe syntax)
// ===========================================================================

namespace internal
{

// filter_closure
//   class: the bound form of views::filter. Holds a predicate
// and, when invoked, constructs a filter_view directly.
template<typename Pred>
struct filter_closure : range_adaptor_closure<filter_closure<Pred> >
{
    Pred pred;

    RE_STD_CONSTEXPR
    filter_closure()
        : pred()
    {}

    RE_STD_CONSTEXPR explicit
    filter_closure(
        Pred _p
    )
        : pred(static_cast<Pred&&>(_p))
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    filter_view<typename internal::all_dispatch<R>::type, Pred>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return filter_view<view_type, Pred>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            pred
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::FILTER
// ===========================================================================

namespace views
{
    // views::filter(_r, _pred)  [direct form]
    template<typename R,
             typename Pred>
    RE_STD_CONSTEXPR_INLINE
    filter_view<typename internal::all_dispatch<R>::type,
                typename decay<Pred>::type>
    filter(
        R&&    _r,
        Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<R>::type  view_type;
        typedef typename decay<Pred>::type                pred_type;
        return filter_view<view_type, pred_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<Pred&&>(_pred)
        );
    }

    // views::filter(_pred)  [bound form]
    template<typename Pred>
    RE_STD_CONSTEXPR_INLINE
    internal::filter_closure<typename decay<Pred>::type>
    filter(
        Pred&& _pred
    )
    {
        return internal::filter_closure<typename decay<Pred>::type>(
            static_cast<Pred&&>(_pred)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_FILTER_VIEW_HPP
