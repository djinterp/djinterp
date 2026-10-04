/******************************************************************************
* djinterp [re_std]                                              take_view.hpp
*
* take_view header:
*   Provides the C++20 prefix adaptor. take_view<V> presents the
* first N elements of an underlying view V, or all of V if it has
* fewer than N elements. Used as the slicing primitive for many
* range-based algorithms.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom internal sentinel.
*   - The C++20 standard formulates take_view's iterator as
*     iterator_t<V> when V is a sized random-access range, and
*     counted_iterator<iterator_t<V>> otherwise. re_std has not yet
*     shipped counted_iterator (deferred in <iterator> Phase 7c),
*     so we use an internal counter-style sentinel: begin() returns
*     iterator_t<V> as-is, end() returns a sentinel that compares
*     equal when either (a) the underlying iterator has reached the
*     underlying end, or (b) N elements have been visited.
*     The sentinel form is correct for every underlying category;
*     the C++20 spec's optimisation (use plain iterator_t<V> for
*     random-access) is not yet applied — every take_view::iterator
*     pays a 1-word count overhead.
*
*   COLOCATED:
*   re_std::views::take(r, n) — function template that constructs
* take_view over views::all(r).
*
*
* path:      /inc/djinterp/re_std/ranges/take_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_TAKE_VIEW_
#define DJINTERP_RE_STD_RANGES_TAKE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./range_difference_t.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   TAKE_VIEW
// ===========================================================================

// take_view<_View>
//   class: yields the first _n elements of _View. If _View has fewer
// than _n elements, yields all of them.
template<typename _View>
class take_view : public view_interface<take_view<_View> >
{
public:
    typedef _View                                  base_view;
    typedef typename iterator_traits<
                          iterator_t<_View>
                      >::difference_type           difference_type;


private:
    _View           m_base;
    difference_type m_count;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: pairs the underlying view's iterator with a remaining
    // count. Operator++ advances the underlying iterator and
    // decrements the count.
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
        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::pointer             pointer;
        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::reference           reference;

    private:
        iterator_t<_View>  m_it;
        difference_type    m_remaining;


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_it(),
              m_remaining(0)
        {}

        // value ctor
        D_CONSTEXPR
        iterator(
            iterator_t<_View>  _it,
            difference_type    _remaining
        )
            : m_it(_it),
              m_remaining(_remaining)
        {}


        // base
        //   function: returns the underlying iterator. Useful for
        // unwrapping after take.
        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_it;
        }

        // count
        //   function: returns the remaining count. Non-standard
        // accessor for the sentinel-comparison path.
        D_CONSTEXPR difference_type
        count() const
        D_NOEXCEPT
        {
            return m_remaining;
        }


        // operator*
        D_CONSTEXPR reference
        operator*() const
        {
            return *m_it;
        }


        // operator++ (pre / post)
        D_CONSTEXPR_INLINE iterator&
        operator++()
        {
            ++m_it;
            --m_remaining;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_it;
            --m_remaining;
            return tmp;
        }


        // ==, !=
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

    // sentinel
    //   class: holds the underlying view's end sentinel. Equality
    // with iterator triggers when EITHER the iterator has reached
    // the underlying end OR its remaining count has dropped to 0.
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

        // iterator-vs-sentinel comparison (both directions).
        // Friend functions so ADL picks them up.
        friend D_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return ( (_it.count() <= 0)
                  || (_it.base() == _s.m_end) );
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
    take_view()
        : m_base(),
          m_count(0)
    {}

    // value ctor
    D_CONSTEXPR
    take_view(
        _View            _base,
        difference_type  _n
    )
        : m_base(static_cast<_View&&>(_base)),
          m_count(_n)
    {}


    // base
    //   function: returns a copy of the underlying view. Required
    // by the C++20 contract for adaptor composition.
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }


    // begin
    //   function: iterator paired with the take count.
    D_CONSTEXPR iterator
    begin()
    {
        return iterator(re_std::begin(m_base), m_count);
    }

    // end
    //   function: sentinel wrapping the underlying end.
    D_CONSTEXPR sentinel
    end()
    {
        return sentinel(re_std::end(m_base));
    }
};


// ===========================================================================
// II.  TAKE_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

// take_closure
//   class: the bound form of views::take. Holds a count and, when
// invoked, constructs a take_view from views::all of the argument
// plus the stored count. Constructs the take_view directly rather
// than calling back through views::take to avoid the forward-
// reference at template parse time.
template<typename _N>
struct take_closure : range_adaptor_closure<take_closure<_N> >
{
    _N count;

    D_CONSTEXPR
    take_closure()
        : count()
    {}

    D_CONSTEXPR explicit
    take_closure(
        _N _n
    )
        : count(_n)
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    take_view<typename internal::all_dispatch<_R>::type>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        typedef typename iterator_traits<
                              iterator_t<typename remove_reference<_R>::type>
                          >::difference_type             diff_type;
        return take_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<diff_type>(count)
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::TAKE
// ===========================================================================

namespace views
{
    // views::take(_r, _n)  [direct form]
    //   function: wraps _r in a take_view, calling views::all to
    // coerce _r to a view first.
    template<typename _R>
    D_CONSTEXPR_INLINE
    take_view<typename internal::all_dispatch<_R>::type>
    take(
        _R&&                                                            _r,
        typename iterator_traits<
                     iterator_t<typename remove_reference<_R>::type>
                 >::difference_type                                     _n
    )
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return take_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            _n
        );
    }

    // views::take(_n)  [bound form]
    //   function: returns an internal::take_closure that, when
    // invoked with a range, calls views::take(r, _n). Enables
    // pipe syntax: r | views::take(10).
    template<typename _N>
    D_CONSTEXPR_INLINE
    internal::take_closure<typename decay<_N>::type>
    take(
        _N&& _n
    )
    {
        return internal::take_closure<typename decay<_N>::type>(
            static_cast<_N&&>(_n)
        );
    }
}  // namespace views


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_TAKE_VIEW_
