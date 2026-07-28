/******************************************************************************
* djinterp [restd]                                              chunk_view.hpp
*
* chunk_view header:
*   Provides the C++23 chunk adaptor. chunk_view<V> partitions an
* underlying view V into non-overlapping subranges of N consecutive
* elements; the last subrange may be shorter if size(V) is not a
* multiple of N. Each element of the outer view is itself a
* subrange<iterator_t<V>>.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - R28: iterator_category derived from underlying. Bidi+
*     underlyings yield a bidi chunk iterator; operator-- retreats
*     m_n positions and sets m_chunk_end = previous m_start.
*   - R29: RA underlyings yield an RA chunk iterator with O(1)
*     advance: operator+= n is m_start += n*m_n (clamped). The
*     chunk_end is recomputed to span the new chunk (may be shorter
*     than m_n for the last chunk).
*   - The chunk end is cached on the iterator so operator* is O(1).
*   - Iterator yields subranges by value (subrange is movable and
*     cheap — two iterators).
*   - enable_borrowed_range NOT specialised — chunk_view's iterators
*     are valid only while the chunk_view (and its m_end sentinel)
*     is alive.
*
*   COLOCATED:
*   restd::views::chunk(r, n) — direct form.
*   restd::views::chunk(n)    — bound form for pipe syntax.
*
*
* path:      /inc/djinterp/restd/ranges/chunk_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_CHUNK_VIEW_
#define DJINTERP_RESTD_RANGES_CHUNK_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./subrange.hpp"
#include "./subrange_kind.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   CHUNK_VIEW
// ===========================================================================

// chunk_view<_View>
//   class: partitions _View into N-element subranges. Chunks of N
// elements at all positions except possibly the last, which is
// short if size(V) % N != 0.
template<typename _View>
class chunk_view : public view_interface<chunk_view<_View> >
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
    //   class: holds (chunk_start, chunk_end, base_end, n). On
    // operator*, returns subrange(chunk_start, chunk_end) — O(1).
    // On operator++, advances chunk_start to chunk_end and computes
    // a new chunk_end by advancing n times (clamped) — O(N).
    class iterator
    {
    private:
        // _bidi_clamp — passthrough; R28 promoted from forward-only,
        // R29 lifted the clamp to allow RA underlyings to yield an
        // RA chunk iterator (O(1) advance to chunk K via m_start +=
        // K * m_n).
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
        iterator_t<_View>   m_chunk_end;
        sentinel_t<_View>   m_base_end;
        difference_type     m_n;


        // compute_chunk_end
        //   function: advances m_chunk_end from m_start by up to
        // m_n positions, stopping at m_base_end.
        void
        compute_chunk_end()
        {
            m_chunk_end = m_start;
            for (difference_type _k = m_n;
                 _k > 0 && m_chunk_end != m_base_end;
                 --_k)
            {
                ++m_chunk_end;
            }
        }


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_start(),
              m_chunk_end(),
              m_base_end(),
              m_n(1)
        {}

        // value ctor
        iterator(
            iterator_t<_View>   _start,
            sentinel_t<_View>   _base_end,
            difference_type     _n
        )
            : m_start(_start),
              m_chunk_end(),
              m_base_end(_base_end),
              m_n(_n)
        {
            compute_chunk_end();
        }


        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_start;
        }


        // operator*
        //   function: yields the current chunk as a subrange. O(1)
        // — uses the cached chunk_end.
        D_CONSTEXPR reference
        operator*() const
        {
            return reference(m_start, m_chunk_end);
        }


        // operator++ (pre)
        //   function: starts the next chunk at the previous chunk's
        // end and computes the new chunk_end (O(N) advance).
        iterator&
        operator++()
        {
            m_start = m_chunk_end;
            compute_chunk_end();
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
        //   function: retreats by n positions on the underlying
        // iterator. The current m_start becomes the previous
        // chunk's m_chunk_end; the new m_start is computed by
        // retreating m_n positions. Compiles only when the
        // underlying iterator supports operator--.
        //
        //   Note: undefined if invoked at the begin iterator of the
        // chunk_view (no "previous chunk" exists). The caller is
        // responsible for not retreating past the start.
        iterator&
        operator--()
        {
            m_chunk_end = m_start;
            for (difference_type _k = m_n; _k > 0; --_k)
            {
                --m_start;
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
        //   For RA underlyings, advancing K chunks is O(1) via
        // m_start += K*m_n with clamping at m_base_end. The
        // chunk_end is recomputed to span the new chunk (may be
        // shorter than m_n if the last chunk is short).
        //
        //   Compiles only when iterator_t<_View> supports operator+=
        // and operator- (i.e. is random-access).

        iterator&
        operator+=(
            difference_type _n
        )
        {
            difference_type _shift = _n * m_n;
            if (_shift >= 0)
            {
                difference_type _available = m_base_end - m_start;
                if (_shift <= _available)
                {
                    m_start += _shift;
                }
                else
                {
                    m_start = m_base_end;
                }
            }
            else
            {
                // Backward — assume the caller doesn't retreat past
                // the chunk_view's begin.
                m_start += _shift;
            }
            // Recompute chunk_end for the new start.
            compute_chunk_end();
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

        // operator-(it, it) — chunk distance, in chunks.
        //   Ceiling division so the past-last-short-chunk position
        // (m_start == m_base_end) counts as one more than the last
        // valid chunk index.
        D_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return (m_start - _rhs.m_start + m_n - 1) / m_n;
        }

        D_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return *(*this + _n);
        }


        // ordering by m_start.
        D_CONSTEXPR bool operator<(iterator const& _r)  const { return m_start <  _r.m_start; }
        D_CONSTEXPR bool operator<=(iterator const& _r) const { return m_start <= _r.m_start; }
        D_CONSTEXPR bool operator>(iterator const& _r)  const { return m_start >  _r.m_start; }
        D_CONSTEXPR bool operator>=(iterator const& _r) const { return m_start >= _r.m_start; }


        // == / !=
        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_start == _rhs.m_start);
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return (m_start != _rhs.m_start);
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
    chunk_view()
        : m_base(),
          m_n(1)
    {}

    // value ctor
    //   function: _n must be positive. Zero or negative N is
    // undefined behaviour.
    D_CONSTEXPR
    chunk_view(
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

    // chunk_size
    //   function: returns the N value. Non-standard accessor.
    D_CONSTEXPR difference_type
    chunk_size() const
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
        return sentinel(restd::end(m_base));
    }


    // size
    //   function: ceil(size(base) / n). Only well-formed when the
    // underlying view is sized.
    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        typedef decltype(restd::size(m_base)) size_type;
        size_type s = restd::size(m_base);
        size_type n = static_cast<size_type>(m_n);
        return (s + n - 1) / n;
    }
};


// ===========================================================================
// II.  CHUNK_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _N>
struct chunk_closure : range_adaptor_closure<chunk_closure<_N> >
{
    _N count;

    D_CONSTEXPR
    chunk_closure()
        : count()
    {}

    D_CONSTEXPR explicit
    chunk_closure(
        _N _n
    )
        : count(_n)
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    chunk_view<typename internal::all_dispatch<_R>::type>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        typedef typename iterator_traits<
                              iterator_t<typename remove_reference<_R>::type>
                          >::difference_type             diff_type;
        return chunk_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<diff_type>(count)
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::CHUNK
// ===========================================================================

namespace views
{
    // views::chunk(_r, _n)  [direct form]
    template<typename _R>
    D_CONSTEXPR_INLINE
    chunk_view<typename internal::all_dispatch<_R>::type>
    chunk(
        _R&&                                                            _r,
        typename iterator_traits<
                     iterator_t<typename remove_reference<_R>::type>
                 >::difference_type                                     _n
    )
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return chunk_view<view_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            _n
        );
    }

    // views::chunk(_n)  [bound form]
    template<typename _N>
    D_CONSTEXPR_INLINE
    internal::chunk_closure<typename decay<_N>::type>
    chunk(
        _N&& _n
    )
    {
        return internal::chunk_closure<typename decay<_N>::type>(
            static_cast<_N&&>(_n)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_CHUNK_VIEW_
