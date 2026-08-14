/******************************************************************************
* djinterp [restd]                                             stride_view.hpp
*
* stride_view header:
*   Provides the C++23 stride adaptor. stride_view<V> yields every
* Nth element of an underlying view V — V[0], V[N], V[2N], V[3N],
* ... — stopping at the first index that reaches or exceeds the
* underlying end.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - R28: iterator_category derived from the underlying. Forward
*     underlyings yield a forward stride iterator. Bidirectional
*     underlyings yield a bidirectional iterator — the m_missing
*     field tracks the short-of-step overshoot on the final ++ so a
*     subsequent -- lands on the last stride boundary that ++
*     yielded.
*   - R29: random-access underlyings now yield a random-access
*     stride iterator with O(1) operator+= / -= / +(n) / -(n) / [n]
*     and proper ordering. The += / -= operations compute the
*     "ideal" position as (real + missing) and recompute missing
*     after the move.
*
*   COLOCATED:
*   restd::views::stride(r, n) — direct form.
*   restd::views::stride(n)    — bound form for pipe syntax.
*
*
* path:      /inc/djinterp/re_std/ranges/stride_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_STRIDE_VIEW_
#define DJINTERP_RESTD_RANGES_STRIDE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   STRIDE_VIEW
// ===========================================================================

// stride_view<_View>
//   class: every Nth element of _View. The N value is set at
// construction; N must be at least 1.
template<typename _View>
class stride_view : public view_interface<stride_view<_View> >
{
public:
    typedef _View                                       base_view;
    typedef typename iterator_traits<
                          iterator_t<_View>
                      >::difference_type                difference_type;


private:
    _View            m_base;
    difference_type  m_step;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: wraps the underlying iterator. operator++ advances
    // by step, clamping at the underlying end. operator-- retreats
    // by step (compensating for any "missing" overshoot from a
    // previous clamped ++) when the underlying iterator is
    // bidirectional or stronger.
    class iterator
    {
    private:
        // _bidi_clamp
        //   trait: passthrough — R29 lifted the previous bidi
        //   clamp so RA underlyings yield an RA stride iterator.
        // Retained as a single seam should a future restd version
        // need to clamp again (e.g. for spec compliance on a
        // contiguous-only underlying).
        template<typename _Cat>
        struct _bidi_clamp
        {
            typedef _Cat type;
        };


    public:
        // iterator_category: promoted from the underlying when the
        // underlying is forward / bidirectional, clamped at bidi
        // when the underlying is RA (see _bidi_clamp).
        typedef typename _bidi_clamp<
                              typename iterator_traits<
                                            iterator_t<_View>
                                        >::iterator_category
                          >::type                               iterator_category;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::value_type                         value_type;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::difference_type                    difference_type;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::reference                          reference;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::pointer                            pointer;


    private:
        iterator_t<_View>           m_it;
        sentinel_t<_View>           m_end;
        difference_type             m_step;

        // m_missing
        //   field: the number of positions short of a full stride
        // the last ++ fell when it hit the underlying end. Zero
        // for every non-end iterator. When non-zero, operator--
        // retreats by (step - missing) to land on the actual last
        // stride boundary rather than (last_stride_boundary - 1).
        //
        //   Example: base = [0..7], step = 3, stride boundaries at
        // positions {0, 3, 6}. After yielding 6, ++ tries to advance
        // 3 more but the underlying has only 2 left, so the iterator
        // lands at end (position 8) with m_missing = 1. A subsequent
        // -- retreats 3 - 1 = 2 positions, landing back at position
        // 6 (the last valid stride boundary). Without m_missing the
        // retreat would be a full 3, landing at the wrong position.
        difference_type             m_missing;


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_it(),
              m_end(),
              m_step(1),
              m_missing(0)
        {}

        // value ctor
        D_CONSTEXPR
        iterator(
            iterator_t<_View>  _it,
            sentinel_t<_View>  _end,
            difference_type    _step
        )
            : m_it(_it),
              m_end(_end),
              m_step(_step),
              m_missing(0)
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
        //   function: advances by step, clamping at end. If the
        // advance is short of step (we hit end), records the
        // shortfall in m_missing so operator-- can recover the
        // correct last-stride-boundary position.
        iterator&
        operator++()
        {
            difference_type _n = m_step;
            while (_n > 0 && m_it != m_end)
            {
                ++m_it;
                --_n;
            }
            m_missing = _n;  // 0 normally; positive iff we hit end short
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
        //   function: retreats by (step - missing) and resets missing.
        // For a non-end iterator (missing == 0) this is a plain
        // step retreat; for an at-end iterator with missing > 0
        // this retreats the shorter distance needed to land on the
        // last stride boundary that ++ actually yielded.
        //
        //   Compiles only when iterator_t<_View> supports operator--
        // (i.e. the underlying is bidirectional or stronger).
        iterator&
        operator--()
        {
            difference_type _retreat = m_step - m_missing;
            while (_retreat > 0)
            {
                --m_it;
                --_retreat;
            }
            m_missing = 0;
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
        //   Compile only when iterator_t<_View> supports operator+=,
        // operator-, operator<, etc. SFINAE-lazy via template
        // instantiation — these member templates instantiate only
        // when used by the caller.

        // operator+=
        //   function: advances by _n strides. Computes the "ideal"
        // position as (m_it position + m_missing) and adds n*step,
        // clamping at end. m_missing is updated to reflect any
        // overshoot beyond the end.
        iterator&
        operator+=(
            difference_type _n
        )
        {
            difference_type _shift = _n * m_step - m_missing;
            if (_shift >= 0)
            {
                difference_type _available = m_end - m_it;
                if (_shift <= _available)
                {
                    m_it += _shift;
                    m_missing = 0;
                }
                else
                {
                    // Overshoot: land at end, record new missing.
                    m_it = m_end;
                    m_missing = _shift - _available;
                }
            }
            else
            {
                // Backward motion. _shift is negative.
                m_it += _shift;
                m_missing = 0;
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

        // operator-(it, it)
        //   function: stride distance between two iterators.
        // Uses (it_pos + missing) ideal positions to handle the
        // end-with-missing case correctly.
        D_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return ((m_it - _rhs.m_it) + (m_missing - _rhs.m_missing)) / m_step;
        }

        D_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return *(*this + _n);
        }


        // ordering — by ideal stride position.
        D_CONSTEXPR bool
        operator<(iterator const& _rhs) const
        {
            return (m_it - _rhs.m_it) + (m_missing - _rhs.m_missing) < 0;
        }

        D_CONSTEXPR bool
        operator<=(iterator const& _rhs) const
        {
            return !(_rhs < *this);
        }

        D_CONSTEXPR bool
        operator>(iterator const& _rhs) const
        {
            return _rhs < *this;
        }

        D_CONSTEXPR bool
        operator>=(iterator const& _rhs) const
        {
            return !(*this < _rhs);
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
    stride_view()
        : m_base(),
          m_step(1)
    {}

    // value ctor
    //   function: _step must be positive. Negative or zero steps
    // are undefined behaviour.
    D_CONSTEXPR
    stride_view(
        _View            _base,
        difference_type  _step
    )
        : m_base(static_cast<_View&&>(_base)),
          m_step(_step)
    {}


    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    // stride
    //   function: returns the step value. Non-standard accessor.
    D_CONSTEXPR difference_type
    stride() const
    D_NOEXCEPT
    {
        return m_step;
    }


    // begin / end
    D_CONSTEXPR iterator
    begin()
    {
        return iterator(restd::begin(m_base), restd::end(m_base), m_step);
    }

    D_CONSTEXPR sentinel
    end()
    {
        return sentinel(restd::end(m_base));
    }


    // size
    //   function: ceil(size(base) / step). Only well-formed when
    // the underlying view is sized.
    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        typedef decltype(restd::size(m_base)) size_type;
        size_type s    = restd::size(m_base);
        size_type step = static_cast<size_type>(m_step);
        return (s + step - 1) / step;  // ceiling division
    }
};


// ===========================================================================
// II.  STRIDE_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _N>
struct stride_closure : range_adaptor_closure<stride_closure<_N> >
{
    _N step;

    D_CONSTEXPR
    stride_closure()
        : step()
    {}

    D_CONSTEXPR explicit
    stride_closure(
        _N _n
    )
        : step(_n)
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    stride_view<typename internal::all_dispatch<_R>::type>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        typedef typename iterator_traits<
                              iterator_t<typename remove_reference<_R>::type>
                          >::difference_type             diff_type;
        return stride_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<diff_type>(step)
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::STRIDE
// ===========================================================================

namespace views
{
    // views::stride(_r, _n)  [direct form]
    template<typename _R>
    D_CONSTEXPR_INLINE
    stride_view<typename internal::all_dispatch<_R>::type>
    stride(
        _R&&                                                            _r,
        typename iterator_traits<
                     iterator_t<typename remove_reference<_R>::type>
                 >::difference_type                                     _n
    )
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return stride_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            _n
        );
    }

    // views::stride(_n)  [bound form]
    template<typename _N>
    D_CONSTEXPR_INLINE
    internal::stride_closure<typename decay<_N>::type>
    stride(
        _N&& _n
    )
    {
        return internal::stride_closure<typename decay<_N>::type>(
            static_cast<_N&&>(_n)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_STRIDE_VIEW_
