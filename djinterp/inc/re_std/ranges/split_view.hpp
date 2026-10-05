/*******************************************************************************
* djinterp [re_std]                                               split_view.hpp
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
* path:      /inc/re_std/ranges/split_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_SPLIT_VIEW_HPP
#define RE_STD_RANGES_SPLIT_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./subrange.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   SPLIT_VIEW
// ===========================================================================

// split_view<View, Delim>
//   class: splits View on occurrences of a single delimiter value.
template<typename View,
         typename Delim>
class split_view : public view_interface<split_view<View, Delim> >
{
public:
    typedef View   base_view;
    typedef Delim  delimiter_type;


private:
    View   m_base;
    Delim  m_delim;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    class iterator
    {
    private:
        // _bidi_clamp — clamps RA underlyings to bidi (R28 pattern).
        template<typename Cat>
        struct _bidi_clamp
        {
            typedef Cat type;
        };

    public:
        typedef typename _bidi_clamp<
                              typename iterator_traits<
                                            iterator_t<View>
                                        >::iterator_category
                          >::type                       iterator_category;
        typedef subrange<iterator_t<View>,
                         iterator_t<View> >            value_type;
        typedef typename iterator_traits<
                              iterator_t<View>
                          >::difference_type            difference_type;
        typedef value_type                              reference;
        typedef void                                    pointer;


    private:
        iterator_t<View>       m_chunk_start;
        iterator_t<View>       m_chunk_end;
        iterator_t<View>       m_base_begin;  // R28: enables operator-- termination
        bool                    m_at_end;
        split_view const*       m_parent;


        // find_chunk_end
        //   function: from m_chunk_start, scans forward to the next
        // delimiter or to the base end.
        void
        find_chunk_end()
        {
            iterator_t<View> base_end = re_std::end(m_parent->m_base);
            m_chunk_end = m_chunk_start;
            while (m_chunk_end != base_end
                   && !(*m_chunk_end == m_parent->m_delim))
            {
                ++m_chunk_end;
            }
        }


    public:
        RE_STD_CONSTEXPR
        iterator()
            : m_chunk_start(),
              m_chunk_end(),
              m_base_begin(),
              m_at_end(true),
              m_parent(RE_STD_NULLPTR)
        {}

        iterator(
            split_view const*    _parent,
            iterator_t<View>    _begin,
            iterator_t<View>    _base_begin
        )
            : m_chunk_start(_begin),
              m_chunk_end(),
              m_base_begin(_base_begin),
              m_at_end(false),
              m_parent(_parent)
        {
            find_chunk_end();
        }


        RE_STD_CONSTEXPR iterator_t<View>
        base() const
        {
            return m_chunk_start;
        }

        RE_STD_CONSTEXPR bool
        at_end() const
        RE_STD_NOEXCEPT
        {
            return m_at_end;
        }


        RE_STD_CONSTEXPR reference
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
            iterator_t<View> base_end = re_std::end(m_parent->m_base);
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
        //   Compiles only when iterator_t<View> supports operator--.
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
            iterator_t<View> new_chunk_end = m_chunk_start;
            --new_chunk_end;

            // Walk back from the delimiter to find the previous
            // chunk's start: either at base_begin, or just past
            // a second delimiter.
            iterator_t<View> walker = new_chunk_end;
            while (walker != m_base_begin)
            {
                iterator_t<View> prev = walker;
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


        RE_STD_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_at_end && _rhs.m_at_end)
                || (!m_at_end && !_rhs.m_at_end
                    && m_chunk_start == _rhs.m_chunk_start);
        }

        RE_STD_CONSTEXPR bool
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
        RE_STD_CONSTEXPR
        sentinel()
        {}


        friend RE_STD_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&
        )
        {
            return _it.at_end();
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
    RE_STD_CONSTEXPR
    split_view()
        : m_base(),
          m_delim()
    {}

    RE_STD_CONSTEXPR
    split_view(
        View   _base,
        Delim  _delim
    )
        : m_base(static_cast<View&&>(_base)),
          m_delim(static_cast<Delim&&>(_delim))
    {}


    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }

    RE_STD_CONSTEXPR Delim const&
    delim() const
    RE_STD_NOEXCEPT
    {
        return m_delim;
    }


    iterator
    begin() const
    {
        return iterator(this, re_std::begin(m_base), re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR sentinel
    end() const
    {
        return sentinel();
    }
};


// ===========================================================================
// II.  SPLIT_CLOSURE (bound form for pipe syntax)
// ===========================================================================

namespace internal
{

template<typename Delim>
struct split_closure : range_adaptor_closure<split_closure<Delim> >
{
    Delim delim;

    RE_STD_CONSTEXPR
    split_closure()
        : delim()
    {}

    RE_STD_CONSTEXPR explicit
    split_closure(
        Delim _d
    )
        : delim(static_cast<Delim&&>(_d))
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    split_view<typename internal::all_dispatch<R>::type, Delim>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return split_view<view_type, Delim>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            delim
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::SPLIT
// ===========================================================================

namespace views
{
    // views::split(_r, _delim)  [direct form]
    template<typename R,
             typename Delim>
    RE_STD_CONSTEXPR_INLINE
    split_view<typename internal::all_dispatch<R>::type,
               typename decay<Delim>::type>
    split(
        R&&     _r,
        Delim&& _delim
    )
    {
        typedef typename internal::all_dispatch<R>::type  view_type;
        typedef typename decay<Delim>::type               delim_type;
        return split_view<view_type, delim_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<Delim&&>(_delim)
        );
    }

    // views::split(_delim)  [bound form]
    template<typename Delim>
    RE_STD_CONSTEXPR_INLINE
    internal::split_closure<typename decay<Delim>::type>
    split(
        Delim&& _delim
    )
    {
        return internal::split_closure<typename decay<Delim>::type>(
            static_cast<Delim&&>(_delim)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_SPLIT_VIEW_HPP
