/******************************************************************************
* djinterp [re_std]                                             split_view.hpp
*
* split_view header:
*   Provides the C++23 split adaptor (single-delimiter form).
* split_view<V, T> partitions an underlying view V at each
* occurrence of a delimiter element of type T, yielding subranges of
* the elements between delimiters. The delimiters themselves are
* excluded. Adjacent delimiters yield empty subranges.
*
*   SCOPE LIMITATION RELATIVE TO C++23:
*   The C++23 std::ranges::split_view accepts either a single value
* OR a sub-range pattern. Re_std ships only the single-value form —
* the pattern-range form requires a search-with-state machinery
* (essentially a multi-pass forward search inside operator++) that
* nearly doubles the code. The single-value form covers the dominant
* "split on '\n' / ',' / ' '" use cases.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator + sentinel.
*   - Forward-iterator-strength only.
*   - Yields subranges by value; not borrowed.
*
*   COLOCATED:
*   re_std::views::split(r, delim) — direct form.
*   re_std::views::split(delim)    — bound form for pipe syntax.
*
*
* path:      /inc/djinterp/re_std/ranges/split_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_SPLIT_VIEW_
#define DJINTERP_RE_STD_RANGES_SPLIT_VIEW_ 1

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
// I.   SPLIT_VIEW
// ===========================================================================

// split_view<_View, _Delim>
//   class: splits _View on occurrences of a single delimiter value.
template<typename _View,
         typename _Delim>
class split_view : public view_interface<split_view<_View, _Delim> >
{
public:
    typedef _View   base_view;
    typedef _Delim  delimiter_type;


private:
    _View   m_base;
    _Delim  m_delim;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

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
        iterator_t<_View>       m_chunk_start;
        iterator_t<_View>       m_chunk_end;
        iterator_t<_View>       m_base_begin;  // R28: enables operator-- termination
        bool                    m_at_end;
        split_view const*       m_parent;


        // find_chunk_end
        //   function: from m_chunk_start, scans forward to the next
        // delimiter or to the base end.
        void
        find_chunk_end()
        {
            iterator_t<_View> base_end = re_std::end(m_parent->m_base);
            m_chunk_end = m_chunk_start;
            while (m_chunk_end != base_end
                   && !(*m_chunk_end == m_parent->m_delim))
            {
                ++m_chunk_end;
            }
        }


    public:
        D_CONSTEXPR
        iterator()
            : m_chunk_start(),
              m_chunk_end(),
              m_base_begin(),
              m_at_end(true),
              m_parent(D_NULLPTR)
        {}

        iterator(
            split_view const*    _parent,
            iterator_t<_View>    _begin,
            iterator_t<_View>    _base_begin
        )
            : m_chunk_start(_begin),
              m_chunk_end(),
              m_base_begin(_base_begin),
              m_at_end(false),
              m_parent(_parent)
        {
            find_chunk_end();
        }


        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_chunk_start;
        }

        D_CONSTEXPR bool
        at_end() const
        D_NOEXCEPT
        {
            return m_at_end;
        }


        D_CONSTEXPR reference
        operator*() const
        {
            return reference(m_chunk_start, m_chunk_end);
        }


        // operator++ (pre)
        //   function: if chunk_end is at base_end, the just-yielded
        // chunk was the last one — mark exhausted. Otherwise advance
        // chunk_start past the delimiter and find the next chunk
        // end.
        iterator&
        operator++()
        {
            iterator_t<_View> base_end = re_std::end(m_parent->m_base);
            if (m_chunk_end == base_end)
            {
                m_at_end = true;
            }
            else
            {
                m_chunk_start = m_chunk_end;
                ++m_chunk_start;  // skip the delimiter element
                find_chunk_end();
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
        //   function: backs out of the at_end state by clearing the
        // flag (the cached chunk_start/chunk_end already describe
        // the last chunk). For a regular --: the previous chunk's
        // end is at m_chunk_start - 1 (the delimiter); its start is
        // found by walking back to either base_begin or the prior
        // delimiter.
        //
        //   Compiles only when iterator_t<_View> supports operator--.
        //   Undefined if invoked at the begin iterator of the
        // split_view (m_chunk_start == m_base_begin and not at_end).
        iterator&
        operator--()
        {
            if (m_at_end)
            {
                m_at_end = false;
                return *this;
            }

            // The previous chunk's end is one position before our
            // chunk_start (at the delimiter).
            iterator_t<_View> new_chunk_end = m_chunk_start;
            --new_chunk_end;

            // Walk back from the delimiter to find the previous
            // chunk's start: either at base_begin, or just past
            // a second delimiter.
            iterator_t<_View> walker = new_chunk_end;
            while (walker != m_base_begin)
            {
                iterator_t<_View> prev = walker;
                --prev;
                if (*prev == m_parent->m_delim)
                {
                    // walker is just past the delimiter — chunk start.
                    break;
                }
                walker = prev;
            }

            m_chunk_start = walker;
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
            return (m_at_end && _rhs.m_at_end)
                || (!m_at_end && !_rhs.m_at_end
                    && m_chunk_start == _rhs.m_chunk_start);
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
            return _it.at_end();
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
    D_CONSTEXPR
    split_view()
        : m_base(),
          m_delim()
    {}

    D_CONSTEXPR
    split_view(
        _View   _base,
        _Delim  _delim
    )
        : m_base(static_cast<_View&&>(_base)),
          m_delim(static_cast<_Delim&&>(_delim))
    {}


    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }

    D_CONSTEXPR _Delim const&
    delim() const
    D_NOEXCEPT
    {
        return m_delim;
    }


    iterator
    begin() const
    {
        return iterator(this, re_std::begin(m_base), re_std::begin(m_base));
    }

    D_CONSTEXPR sentinel
    end() const
    {
        return sentinel();
    }
};


// ===========================================================================
// II.  SPLIT_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _Delim>
struct split_closure : range_adaptor_closure<split_closure<_Delim> >
{
    _Delim delim;

    D_CONSTEXPR
    split_closure()
        : delim()
    {}

    D_CONSTEXPR explicit
    split_closure(
        _Delim _d
    )
        : delim(static_cast<_Delim&&>(_d))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    split_view<typename internal::all_dispatch<_R>::type, _Delim>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return split_view<view_type, _Delim>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            delim
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::SPLIT
// ===========================================================================

namespace views
{
    // views::split(_r, _delim)  [direct form]
    template<typename _R,
             typename _Delim>
    D_CONSTEXPR_INLINE
    split_view<typename internal::all_dispatch<_R>::type,
               typename decay<_Delim>::type>
    split(
        _R&&     _r,
        _Delim&& _delim
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Delim>::type               delim_type;
        return split_view<view_type, delim_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Delim&&>(_delim)
        );
    }

    // views::split(_delim)  [bound form]
    template<typename _Delim>
    D_CONSTEXPR_INLINE
    internal::split_closure<typename decay<_Delim>::type>
    split(
        _Delim&& _delim
    )
    {
        return internal::split_closure<typename decay<_Delim>::type>(
            static_cast<_Delim&&>(_delim)
        );
    }
}  // namespace views


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_SPLIT_VIEW_
