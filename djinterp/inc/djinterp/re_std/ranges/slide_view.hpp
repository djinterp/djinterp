/******************************************************************************
* djinterp [restd]                                              slide_view.hpp
*
* slide_view header:
*   Provides the C++23 sliding-window adaptor. slide_view<V> yields
* all subranges of N consecutive elements of V — overlapping windows
* of fixed size. For V = {a, b, c, d, e} and N = 3 the result is
* {[a,b,c], [b,c,d], [c,d,e]}. Unlike chunk_view, slide_view never
* yields a short window: if size(V) < N, the result is empty.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - R28: iterator_category derived from underlying. Bidi+
*     underlyings yield a bidi slide iterator with O(1) operator--.
*     The exhausted-state has special handling: a -- from exhausted
*     re-enters the last valid window (whose positions are still
*     cached from when ++ set the flag).
*   - R29: RA underlyings yield an RA slide iterator with O(1)
*     operator+=. Sliding by N shifts both window endpoints; if the
*     new m_window_end would exceed m_base_end the iterator
*     transitions to exhausted (and shifts both endpoints partially
*     to land at the limit).
*   - The window end is cached so operator* is O(1); operator++
*     advances both ends by 1 (O(1) per ++ once the iterator is
*     initialised).
*   - The iterator carries an 'exhausted' flag set when a full
*     window no longer fits. Sentinel comparison just checks this
*     flag.
*
*   COLOCATED:
*   restd::views::slide(r, n) — direct form.
*   restd::views::slide(n)    — bound form for pipe syntax.
*
*
* path:      /inc/djinterp/re_std/ranges/slide_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_SLIDE_VIEW_
#define DJINTERP_RESTD_RANGES_SLIDE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./subrange.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   SLIDE_VIEW
// ===========================================================================

// slide_view<_View>
//   class: yields all length-N windows of _View. Never yields a
// short window — when size(_View) < N, the slide is empty.
template<typename _View>
class slide_view : public view_interface<slide_view<_View> >
{
public:
    typedef _View                                       base_view;
    typedef typename iterator_traits<
                          iterator_t<_View>
                      >::difference_type                difference_type;


private:
    _View            m_base;
    difference_type  m_n;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: holds (window_start, window_end, base_end, exhausted).
    // operator* returns subrange(window_start, window_end);
    // operator++ slides both ends by 1; iteration stops when the
    // previous-yielded window was the last fittable one.
    class iterator
    {
    private:
        // _bidi_clamp — passthrough; R28 promoted from forward-only,
        // R29 lifted the clamp to allow RA underlyings to yield an
        // RA slide iterator (O(1) window shift).
        template<typename _Cat>
        struct _bidi_clamp
        {
            typedef _Cat type;
        };

    public:
        typedef typename _bidi_clamp<
                              typename iterator_traits<
                                            iterator_t<_View>
                                        >::iterator_category
                          >::type                       iterator_category;
        typedef subrange<iterator_t<_View>,
                         iterator_t<_View> >            value_type;
        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::difference_type            difference_type;
        typedef value_type                              reference;
        typedef void                                    pointer;


    private:
        iterator_t<_View>   m_start;
        iterator_t<_View>   m_window_end;
        sentinel_t<_View>   m_base_end;
        bool                m_exhausted;


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_start(),
              m_window_end(),
              m_base_end(),
              m_exhausted(true)
        {}

        // value ctor
        //   function: initialises the first window. Advances
        // window_end up to _n positions; if the advance can't fit a
        // full window (fewer than _n elements remaining), sets
        // m_exhausted = true immediately.
        iterator(
            iterator_t<_View>   _begin,
            sentinel_t<_View>   _base_end,
            difference_type     _n
        )
            : m_start(_begin),
              m_window_end(_begin),
              m_base_end(_base_end),
              m_exhausted(false)
        {
            difference_type _advanced = 0;
            while (_advanced < _n && m_window_end != m_base_end)
            {
                ++m_window_end;
                ++_advanced;
            }
            if (_advanced < _n)
            {
                m_exhausted = true;
            }
        }


        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_start;
        }

        // exhausted
        //   function: introspection — true when no further window
        // fits. The sentinel comparison reads this.
        D_CONSTEXPR bool
        exhausted() const
        D_NOEXCEPT
        {
            return m_exhausted;
        }


        D_CONSTEXPR reference
        operator*() const
        {
            return reference(m_start, m_window_end);
        }


        // operator++ (pre)
        //   function: if the previous window's end was at base_end,
        // the just-yielded window was the last possible — set
        // exhausted. Otherwise slide both ends by one.
        iterator&
        operator++()
        {
            if (m_exhausted)
            {
                return *this;
            }
            if (m_window_end == m_base_end)
            {
                m_exhausted = true;
            }
            else
            {
                ++m_start;
                ++m_window_end;
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


        // operator-- (pre, bidirectional path)
        //   function: slides both window ends backward by one.
        // Special handling for the exhausted state: returning from
        // exhausted means recovering the last valid window. When ++
        // entered exhausted (m_window_end was at base_end), m_start
        // and m_window_end were left at the last valid window's
        // positions, so we simply clear the exhausted flag.
        //
        //   Compiles only when iterator_t<_View> supports operator--.
        iterator&
        operator--()
        {
            if (m_exhausted)
            {
                // The last valid window's (m_start, m_window_end)
                // is still cached; just clear the flag.
                m_exhausted = false;
            }
            else
            {
                --m_start;
                --m_window_end;
            }
            return *this;
        }

        iterator
        operator--(int)
        {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }


        // R29 — random-access ops.
        //   For RA underlyings, sliding by N is O(1): both
        // m_start and m_window_end shift by N. If the new
        // m_window_end would exceed m_base_end the iterator becomes
        // exhausted.
        //
        //   Compiles only when iterator_t<_View> is random-access.

        iterator&
        operator+=(
            difference_type _n
        )
        {
            if (_n == 0)
            {
                return *this;
            }
            if (m_exhausted)
            {
                // Currently past the last valid window. Operating on
                // exhausted iterators is well-defined only for ==/!=
                // and negative motion back to a valid window.
                if (_n < 0)
                {
                    // Re-enter the last valid window then shift the
                    // remaining n+1 places.
                    m_exhausted = false;
                    _n += 1;
                }
                else
                {
                    return *this;  // can't go further forward
                }
            }
            if (_n > 0)
            {
                difference_type _available = m_base_end - m_window_end;
                if (_n <= _available)
                {
                    m_start += _n;
                    m_window_end += _n;
                }
                else
                {
                    // Past last valid window — exhaust.
                    m_start += _available;
                    m_window_end = m_base_end;
                    m_exhausted = true;
                }
            }
            else  // _n < 0
            {
                m_start += _n;
                m_window_end += _n;
            }
            return *this;
        }

        iterator&
        operator-=(
            difference_type _n
        )
        {
            return *this += (-_n);
        }

        D_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            iterator tmp = *this;
            tmp += _n;
            return tmp;
        }

        friend D_CONSTEXPR iterator
        operator+(
            difference_type     _n,
            iterator            _it
        )
        {
            return _it + _n;
        }

        D_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            iterator tmp = *this;
            tmp -= _n;
            return tmp;
        }

        // operator-(it, it) — window distance.
        //   For non-exhausted iterators: simple position subtraction.
        //   For one-side exhausted: treat exhausted as position
        // (last_valid_start + 1).
        D_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            difference_type _lhs_offset = m_exhausted ? 1 : 0;
            difference_type _rhs_offset = _rhs.m_exhausted ? 1 : 0;
            return (m_start - _rhs.m_start) + _lhs_offset - _rhs_offset;
        }

        D_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return *(*this + _n);
        }


        // ordering.
        D_CONSTEXPR bool
        operator<(iterator const& _r) const
        {
            return ((*this) - _r) < 0;
        }

        D_CONSTEXPR bool
        operator<=(iterator const& _r) const
        {
            return ((*this) - _r) <= 0;
        }

        D_CONSTEXPR bool
        operator>(iterator const& _r) const
        {
            return ((*this) - _r) > 0;
        }

        D_CONSTEXPR bool
        operator>=(iterator const& _r) const
        {
            return ((*this) - _r) >= 0;
        }


        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            // Two iterators are equal when both are exhausted, OR
            // both point at the same start and neither is exhausted.
            return (m_exhausted && _rhs.m_exhausted)
                || (!m_exhausted && !_rhs.m_exhausted
                    && m_start == _rhs.m_start);
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return !(*this == _rhs);
        }
    };


    // =======================================================
    // I.B   NESTED SENTINEL
    // =======================================================

    // sentinel
    //   class: empty tag. Iterator-vs-sentinel comparison reads the
    // iterator's exhausted flag.
    class sentinel
    {
    public:
        D_CONSTEXPR
        sentinel()
        {}


        friend D_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&
        )
        {
            return _it.exhausted();
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
    slide_view()
        : m_base(),
          m_n(1)
    {}

    // value ctor
    D_CONSTEXPR
    slide_view(
        _View            _base,
        difference_type  _n
    )
        : m_base(static_cast<_View&&>(_base)),
          m_n(_n)
    {}


    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    D_CONSTEXPR difference_type
    window_size() const
    D_NOEXCEPT
    {
        return m_n;
    }


    // begin / end
    iterator
    begin()
    {
        return iterator(restd::begin(m_base), restd::end(m_base), m_n);
    }

    D_CONSTEXPR sentinel
    end()
    {
        return sentinel();
    }


    // size
    //   function: max(0, size(base) - N + 1). Only well-formed
    // when the underlying view is sized.
    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        typedef decltype(restd::size(m_base)) size_type;
        size_type s = restd::size(m_base);
        size_type n = static_cast<size_type>(m_n);
        return (s >= n) ? (s - n + 1) : static_cast<size_type>(0);
    }
};


// ===========================================================================
// II.  SLIDE_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _N>
struct slide_closure : range_adaptor_closure<slide_closure<_N> >
{
    _N count;

    D_CONSTEXPR
    slide_closure()
        : count()
    {}

    D_CONSTEXPR explicit
    slide_closure(
        _N _n
    )
        : count(_n)
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    slide_view<typename internal::all_dispatch<_R>::type>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        typedef typename iterator_traits<
                              iterator_t<typename remove_reference<_R>::type>
                          >::difference_type             diff_type;
        return slide_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<diff_type>(count)
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::SLIDE
// ===========================================================================

namespace views
{
    // views::slide(_r, _n)  [direct form]
    template<typename _R>
    D_CONSTEXPR_INLINE
    slide_view<typename internal::all_dispatch<_R>::type>
    slide(
        _R&&                                                            _r,
        typename iterator_traits<
                     iterator_t<typename remove_reference<_R>::type>
                 >::difference_type                                     _n
    )
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return slide_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            _n
        );
    }

    // views::slide(_n)  [bound form]
    template<typename _N>
    D_CONSTEXPR_INLINE
    internal::slide_closure<typename decay<_N>::type>
    slide(
        _N&& _n
    )
    {
        return internal::slide_closure<typename decay<_N>::type>(
            static_cast<_N&&>(_n)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_SLIDE_VIEW_
