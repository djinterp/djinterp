/******************************************************************************
* djinterp [re_std]                                        lazy_split_view.hpp
*
* lazy_split_view header:
*   Provides the C++20 lazy_split_view adaptor. lazy_split_view<V, T>
* splits an underlying view V at occurrences of a delimiter value T,
* yielding inner views over the segments between delimiters.
*
*   DESIGN — INPUT-RANGE-FRIENDLY (vs split_view R16):
*   The shipped split_view (Phase R16) is forward-iterator-strength
* only — it pre-scans to locate each chunk's end and yields a
* subrange<iterator_t<V>>. That design requires the underlying
* iterator type to support multi-pass traversal.
*
*   lazy_split_view supports input-only ranges. The inner view yielded
* by *outer is NOT a subrange but a custom inner_view whose iterator
* holds a back-pointer to the outer iterator. Advancing the inner
* iterator advances the SAME underlying iterator that outer tracks.
* This means:
*   - inner_view iteration is fundamentally single-pass.
*   - Saving an inner_view and then ++outer invalidates the saved
*     inner_view (the underlying iterator has moved past the
*     delimiter).
*
*   This is the trade-off the C++20 lazy_split was designed around.
* Use lazy_split_view when you don't need to materialise chunks
* (just pipe each through algorithms in turn); use split_view (R16)
* when you do need backed-by-subrange chunks.
*
*   SCOPE — single-element delimiter only (matches split_view R16).
*   The C++23 sub-range-pattern form is deferred.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom outer/inner iterators.
*   - Forward-iterator-strength outer (single delimiter scan per ++).
*     Inner iterator inherits the underlying iterator category up
*     to forward (clamped — input iterators allowed too).
*
*   COLOCATED:
*   re_std::views::lazy_split(r, delim) — direct form.
*   re_std::views::lazy_split(delim)    — bound form for pipe syntax.
*
*
* path:      /inc/djinterp/re_std/ranges/lazy_split_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_LAZY_SPLIT_VIEW_
#define DJINTERP_RE_STD_RANGES_LAZY_SPLIT_VIEW_ 1

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
// I.   LAZY_SPLIT_VIEW
// ===========================================================================

// lazy_split_view<_View, _Delim>
//   class: input-range-friendly split. Each outer iteration yields
// an inner_view whose iterators share state with the outer.
template<typename _View,
         typename _Delim>
class lazy_split_view : public view_interface<lazy_split_view<_View, _Delim> >
{
public:
    typedef _View   base_view;
    typedef _Delim  delimiter_type;


private:
    _View   m_base;
    _Delim  m_delim;


public:
    class outer_iterator;
    class inner_view;
    class inner_iterator;
    class inner_sentinel;


    // =======================================================
    // I.A   OUTER_ITERATOR
    // =======================================================

    // outer_iterator
    //   class: tracks the "current position" in the underlying view.
    // Iterators returned by *outer share this state through a
    // pointer back to *this outer_iterator.
    class outer_iterator
    {
    public:
        typedef forward_iterator_tag                            iterator_category;
        typedef inner_view                                      value_type;
        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::difference_type                    difference_type;
        typedef inner_view                                      reference;
        typedef void                                            pointer;


    private:
        friend class inner_iterator;
        friend class inner_sentinel;
        friend class inner_view;

        iterator_t<_View>           m_current;
        sentinel_t<_View>           m_base_end;
        bool                        m_trailing;
        lazy_split_view const*      m_parent;


    public:
        D_CONSTEXPR
        outer_iterator()
            : m_current(),
              m_base_end(),
              m_trailing(false),
              m_parent(D_NULLPTR)
        {}

        outer_iterator(
            lazy_split_view const*  _parent,
            iterator_t<_View>       _begin,
            sentinel_t<_View>       _end
        )
            : m_current(_begin),
              m_base_end(_end),
              m_trailing(false),
              m_parent(_parent)
        {}


        D_CONSTEXPR bool
        trailing() const
        D_NOEXCEPT
        {
            return m_trailing;
        }


        // operator* — yields an inner_view sharing state with this
        // outer iterator (via pointer-back).
        inner_view
        operator*() const
        {
            // Cast away the constness of *this since inner_view
            // needs a mutable handle (it advances the underlying
            // iterator). Conceptually the iterator is mutable
            // (input-iteration); the C++20 spec also treats it
            // this way (inner_view sits on a const outer_iterator
            // pointer in some implementations, but the held
            // iterator_t<V> within is mutable).
            return inner_view(const_cast<outer_iterator*>(this));
        }


        // operator++ — scans the underlying iterator forward to
        // either the next delimiter (then steps past) or to base_end.
        // After ++, *outer yields the next chunk (which may be empty
        // if we landed exactly at base_end after a trailing delim).
        outer_iterator&
        operator++()
        {
            if (m_current == m_base_end)
            {
                // We already exhausted the underlying — the just-
                // yielded chunk was the (final, possibly empty)
                // chunk. Mark trailing so we compare equal to the
                // sentinel.
                m_trailing = true;
                return *this;
            }

            // Scan forward to the delimiter.
            while (m_current != m_base_end
                   && !(*m_current == m_parent->m_delim))
            {
                ++m_current;
            }

            // We're now at the delimiter OR at base_end.
            if (m_current == m_base_end)
            {
                // No delimiter encountered for this chunk and the
                // chunk's deref already walked us to the end. The
                // chunk we just yielded was the last one.
                m_trailing = true;
            }
            else
            {
                // At the delimiter; step past it. If we land at
                // base_end, the next yielded chunk is empty (the
                // trailing-empty case).
                ++m_current;
            }
            return *this;
        }

        outer_iterator
        operator++(int)
        {
            outer_iterator tmp = *this;
            ++(*this);
            return tmp;
        }


        D_CONSTEXPR bool
        operator==(
            outer_iterator const& _rhs
        ) const
        {
            return (m_trailing && _rhs.m_trailing)
                || (!m_trailing && !_rhs.m_trailing
                    && m_current == _rhs.m_current);
        }

        D_CONSTEXPR bool
        operator!=(
            outer_iterator const& _rhs
        ) const
        {
            return !(*this == _rhs);
        }
    };


    // =======================================================
    // I.B   OUTER_SENTINEL
    // =======================================================

    class outer_sentinel
    {
    public:
        D_CONSTEXPR
        outer_sentinel()
        {}


        friend D_CONSTEXPR bool
        operator==(
            outer_iterator const&  _it,
            outer_sentinel const&
        )
        {
            return _it.trailing();
        }

        friend D_CONSTEXPR bool
        operator!=(
            outer_iterator const&  _it,
            outer_sentinel const&  _s
        )
        {
            return !(_it == _s);
        }

        friend D_CONSTEXPR bool
        operator==(
            outer_sentinel const&  _s,
            outer_iterator const&  _it
        )
        {
            return (_it == _s);
        }

        friend D_CONSTEXPR bool
        operator!=(
            outer_sentinel const&  _s,
            outer_iterator const&  _it
        )
        {
            return !(_it == _s);
        }
    };


    // =======================================================
    // I.C   INNER_ITERATOR + INNER_SENTINEL
    // =======================================================

    // inner_iterator
    //   class: shares state with an outer_iterator via pointer-back.
    // operator* dereferences the outer's underlying iterator;
    // operator++ advances that underlying iterator.
    class inner_iterator
    {
    public:
        typedef forward_iterator_tag                            iterator_category;
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
        outer_iterator*  m_outer;


    public:
        D_CONSTEXPR
        inner_iterator()
            : m_outer(D_NULLPTR)
        {}

        D_CONSTEXPR explicit
        inner_iterator(
            outer_iterator*  _outer
        )
            : m_outer(_outer)
        {}


        // at_end
        //   function: tested by the sentinel. True when the outer's
        // underlying iterator has hit base_end OR is sitting on a
        // delimiter value (the end of the current chunk).
        bool
        at_end() const
        {
            return (m_outer->m_current == m_outer->m_base_end)
                || (*(m_outer->m_current) == m_outer->m_parent->m_delim);
        }


        D_CONSTEXPR reference
        operator*() const
        {
            return *(m_outer->m_current);
        }


        inner_iterator&
        operator++()
        {
            ++(m_outer->m_current);
            return *this;
        }

        inner_iterator
        operator++(int)
        {
            inner_iterator tmp = *this;
            ++(*this);
            return tmp;
        }


        D_CONSTEXPR bool
        operator==(
            inner_iterator const& _rhs
        ) const
        {
            return m_outer == _rhs.m_outer;
        }

        D_CONSTEXPR bool
        operator!=(
            inner_iterator const& _rhs
        ) const
        {
            return m_outer != _rhs.m_outer;
        }
    };


    class inner_sentinel
    {
    public:
        D_CONSTEXPR
        inner_sentinel()
        {}


        friend bool
        operator==(
            inner_iterator const&  _it,
            inner_sentinel const&
        )
        {
            return _it.at_end();
        }

        friend bool
        operator!=(
            inner_iterator const&  _it,
            inner_sentinel const&  _s
        )
        {
            return !(_it == _s);
        }

        friend bool
        operator==(
            inner_sentinel const&  _s,
            inner_iterator const&  _it
        )
        {
            return (_it == _s);
        }

        friend bool
        operator!=(
            inner_sentinel const&  _s,
            inner_iterator const&  _it
        )
        {
            return !(_it == _s);
        }
    };


    // =======================================================
    // I.D   INNER_VIEW
    // =======================================================

    // inner_view
    //   class: a single chunk. Iterators are pointer-coupled to the
    // outer_iterator that produced this inner_view. The inner_view
    // and its iterators MUST NOT outlive the outer_iterator they
    // share state with.
    class inner_view : public view_interface<inner_view>
    {
    private:
        outer_iterator*  m_outer;

    public:
        D_CONSTEXPR
        inner_view()
            : m_outer(D_NULLPTR)
        {}

        D_CONSTEXPR explicit
        inner_view(
            outer_iterator*  _outer
        )
            : m_outer(_outer)
        {}


        D_CONSTEXPR inner_iterator
        begin() const
        {
            return inner_iterator(m_outer);
        }

        D_CONSTEXPR inner_sentinel
        end() const
        {
            return inner_sentinel();
        }
    };


public:
    // default ctor
    D_CONSTEXPR
    lazy_split_view()
        : m_base(),
          m_delim()
    {}

    // value ctor
    D_CONSTEXPR
    lazy_split_view(
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


    // begin / end
    //   function: non-const accessors only — the view's
    // m_base needs to be stable for the inner state-sharing to
    // work; we cannot allow the underlying to be copied per begin().
    outer_iterator
    begin() const
    {
        return outer_iterator(
            this,
            re_std::begin(const_cast<_View&>(m_base)),
            re_std::end(const_cast<_View&>(m_base))
        );
    }

    D_CONSTEXPR outer_sentinel
    end() const
    {
        return outer_sentinel();
    }
};


// ===========================================================================
// II.  LAZY_SPLIT_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

template<typename _Delim>
struct lazy_split_closure : range_adaptor_closure<lazy_split_closure<_Delim> >
{
    _Delim delim;

    D_CONSTEXPR
    lazy_split_closure()
        : delim()
    {}

    D_CONSTEXPR explicit
    lazy_split_closure(
        _Delim _d
    )
        : delim(static_cast<_Delim&&>(_d))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    lazy_split_view<typename internal::all_dispatch<_R>::type, _Delim>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return lazy_split_view<view_type, _Delim>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            delim
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::LAZY_SPLIT
// ===========================================================================

namespace views
{
    // views::lazy_split(_r, _delim)  [direct form]
    template<typename _R,
             typename _Delim>
    D_CONSTEXPR_INLINE
    lazy_split_view<typename internal::all_dispatch<_R>::type,
                    typename decay<_Delim>::type>
    lazy_split(
        _R&&     _r,
        _Delim&& _delim
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Delim>::type               delim_type;
        return lazy_split_view<view_type, delim_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Delim&&>(_delim)
        );
    }

    // views::lazy_split(_delim)  [bound form]
    template<typename _Delim>
    D_CONSTEXPR_INLINE
    internal::lazy_split_closure<typename decay<_Delim>::type>
    lazy_split(
        _Delim&& _delim
    )
    {
        return internal::lazy_split_closure<typename decay<_Delim>::type>(
            static_cast<_Delim&&>(_delim)
        );
    }
}  // namespace views


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_LAZY_SPLIT_VIEW_
