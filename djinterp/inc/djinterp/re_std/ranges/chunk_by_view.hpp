/******************************************************************************
* djinterp [restd]                                           chunk_by_view.hpp
*
* chunk_by_view header:
*   Provides the C++23 predicate-driven chunking adaptor.
* chunk_by_view<V, Pred> partitions an underlying view V into
* subranges of consecutive elements such that within each subrange
* Pred(prev, cur) holds for every adjacent pair. A chunk boundary
* falls between consecutive elements (prev, cur) for which
* Pred(prev, cur) returns false. The last chunk may be of any size
* including 1.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - Forward-iterator-strength only. The chunk end is cached on ++
*     so operator* is O(1); chunk-end finding pays O(chunk-length)
*     per ++.
*   - Iterator carries a back-pointer to the parent to access the
*     predicate during chunk-end finding; not borrowed.
*   - Yields subranges by value (subrange is cheap — two iterators).
*
*   COLOCATED:
*   restd::views::chunk_by(r, pred) — direct form.
*   restd::views::chunk_by(pred)    — bound form for pipe syntax.
*
*
* path:      /inc/djinterp/re_std/ranges/chunk_by_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_CHUNK_BY_VIEW_
#define DJINTERP_RESTD_RANGES_CHUNK_BY_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./subrange.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   CHUNK_BY_VIEW
// ===========================================================================

// chunk_by_view<_View, _Pred>
//   class: groups adjacent elements of _View whose pairwise
// predicate holds.
template<typename _View,
         typename _Pred>
class chunk_by_view : public view_interface<chunk_by_view<_View, _Pred> >
{
public:
    typedef _View   base_view;
    typedef _Pred   predicate_type;


private:
    _View                           m_base;
    internal::movable_box<_Pred>    m_pred;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: holds (chunk_start, chunk_end, parent). Each chunk
    // is the maximal prefix from chunk_start where every adjacent
    // pair satisfies the predicate.
    class iterator
    {
    private:
        // _bidi_clamp — clamps RA underlyings to bidi (R28 pattern).
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
        iterator_t<_View>           m_start;
        iterator_t<_View>           m_chunk_end;
        iterator_t<_View>           m_base_begin;  // R28: enables operator-- to stop at the very first chunk
        chunk_by_view const*        m_parent;


        // find_chunk_end
        //   function: from m_start, scan forward looking for the
        // first adjacent pair (prev, cur) where the predicate
        // returns false; m_chunk_end becomes the position of cur.
        // If the scan reaches the end, m_chunk_end becomes the
        // underlying end.
        void
        find_chunk_end()
        {
            sentinel_t<_View> base_end = restd::end(m_parent->m_base);
            if (m_start == base_end)
            {
                m_chunk_end = m_start;
                return;
            }
            iterator_t<_View> prev = m_start;
            iterator_t<_View> cur  = m_start;
            ++cur;
            while (cur != base_end)
            {
                if (!(*(m_parent->m_pred))(*prev, *cur))
                {
                    m_chunk_end = cur;
                    return;
                }
                prev = cur;
                ++cur;
            }
            m_chunk_end = cur;  // reached base_end
        }


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_start(),
              m_chunk_end(),
              m_base_begin(),
              m_parent(D_NULLPTR)
        {}

        // value ctor
        iterator(
            chunk_by_view const*  _parent,
            iterator_t<_View>     _start,
            iterator_t<_View>     _base_begin
        )
            : m_start(_start),
              m_chunk_end(),
              m_base_begin(_base_begin),
              m_parent(_parent)
        {
            find_chunk_end();
        }


        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_start;
        }


        D_CONSTEXPR reference
        operator*() const
        {
            return reference(m_start, m_chunk_end);
        }


        // operator++ (pre)
        iterator&
        operator++()
        {
            m_start = m_chunk_end;
            find_chunk_end();
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
        //   function: walks backward from m_start past elements
        // where the adjacent-pair predicate holds, stopping at the
        // chunk boundary (first false predicate going back) or at
        // m_base_begin. The previous chunk's end becomes the
        // current m_start.
        //
        //   Compiles only when iterator_t<_View> supports operator--.
        //   Undefined if invoked at the begin iterator.
        iterator&
        operator--()
        {
            // The previous chunk's end is exactly the current
            // m_start (since chunk boundaries are between adjacent
            // pairs where the predicate is false).
            iterator_t<_View> new_chunk_end = m_start;

            // Step back into the previous chunk's last element.
            --m_start;

            // Walk back through the previous chunk: as long as
            // pred(*prev, *m_start) holds, prev is in the same chunk.
            while (m_start != m_base_begin)
            {
                iterator_t<_View> prev = m_start;
                --prev;
                if (!(*(m_parent->m_pred))(*prev, *m_start))
                {
                    // Boundary between prev and m_start — m_start
                    // is the chunk start.
                    break;
                }
                m_start = prev;
            }

            m_chunk_end = new_chunk_end;
            return *this;
        }

        iterator
        operator--(int)
        {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }


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
    chunk_by_view()
        : m_base(),
          m_pred()
    {}

    // value ctor
    D_CONSTEXPR
    chunk_by_view(
        _View  _base,
        _Pred  _pred
    )
        : m_base(static_cast<_View&&>(_base)),
          m_pred(static_cast<_Pred&&>(_pred))
    {}


    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    D_CONSTEXPR _Pred const&
    pred() const
    D_NOEXCEPT
    {
        return *m_pred;
    }


    // begin / end
    iterator
    begin() const
    {
        return iterator(this, restd::begin(m_base), restd::begin(m_base));
    }

    D_CONSTEXPR sentinel
    end() const
    {
        return sentinel(restd::end(m_base));
    }
};


// ===========================================================================
// II.  CHUNK_BY_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _Pred>
struct chunk_by_closure : range_adaptor_closure<chunk_by_closure<_Pred> >
{
    _Pred pred;

    D_CONSTEXPR
    chunk_by_closure()
        : pred()
    {}

    D_CONSTEXPR explicit
    chunk_by_closure(
        _Pred _p
    )
        : pred(static_cast<_Pred&&>(_p))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    chunk_by_view<typename internal::all_dispatch<_R>::type, _Pred>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return chunk_by_view<view_type, _Pred>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            pred
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::CHUNK_BY
// ===========================================================================

namespace views
{
    // views::chunk_by(_r, _pred)  [direct form]
    template<typename _R,
             typename _Pred>
    D_CONSTEXPR_INLINE
    chunk_by_view<typename internal::all_dispatch<_R>::type,
                  typename decay<_Pred>::type>
    chunk_by(
        _R&&    _r,
        _Pred&& _pred
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Pred>::type                pred_type;
        return chunk_by_view<view_type, pred_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Pred&&>(_pred)
        );
    }

    // views::chunk_by(_pred)  [bound form]
    template<typename _Pred>
    D_CONSTEXPR_INLINE
    internal::chunk_by_closure<typename decay<_Pred>::type>
    chunk_by(
        _Pred&& _pred
    )
    {
        return internal::chunk_by_closure<typename decay<_Pred>::type>(
            static_cast<_Pred&&>(_pred)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_CHUNK_BY_VIEW_
