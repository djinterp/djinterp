/******************************************************************************
* djinterp [restd]                                               iota_view.hpp
*
* iota_view header:
*   Provides the C++20 arithmetic-sequence view. iota_view<W, W>
* generates the half-open interval [start, bound) by repeated
* operator++ on a stored W value. The most common use is iterating
* an integer range without an explicit container — restd::views::
* iota(0, 10) yields the values 0..9 lazily.
*
*   PORTABILITY:
*   - Requires CRTP + view_interface + nested-iterator templates,
*     available C++11+.
*   - Specialises enable_borrowed_range<iota_view<W, B>> to true —
*     the iterators hold the current value by copy, not a reference
*     to the iota_view itself, so they remain valid past the view's
*     lifetime.
*
*   FORMS SHIPPED:
*   - Bounded:   iota_view<W, W>{start, bound} — half-open
*                interval [start, bound). common_range when W == W.
*   - Unbounded: iota_view<W, unreachable_sentinel_t>{start} —
*                infinite view starting at start. Pair with
*                take_view / take_while_view / similar to terminate.
*
*   COLOCATED:
*   restd::views::iota(start, bound) — bounded form.
*   restd::views::iota(start)        — unbounded form.
*
*   DIFFERENCE TYPE:
*   The nested iterator's difference_type is std::ptrdiff_t rather
* than the C++20 iota-diff-t machinery. For users iterating with a
* size_t W on a machine where ptrdiff_t is narrower than size_t the
* full-range difference may not be representable; that boundary case
* is documented in coverage_data.py and not enforced here.
*
*
* path:      /inc/djinterp/restd/ranges/iota_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_IOTA_VIEW_
#define DJINTERP_RESTD_RANGES_IOTA_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // ptrdiff_t

#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./enable_borrowed_range.hpp"
#include "./unreachable_sentinel_t.hpp"


NS_RESTD


// ===========================================================================
// I.   IOTA_VIEW
// ===========================================================================

// iota_view<_W, _Bound>
//   class: half-open interval [start, bound) over _W. Only the
// common form (_Bound == _W) is supplied here.
template<typename _W,
         typename _Bound = _W>
class iota_view : public view_interface<iota_view<_W, _Bound> >
{
public:
    // ===========================================================
    // I.A   NESTED ITERATOR
    // ===========================================================

    // iterator
    //   class: counts _W via operator++ / --. random-access by
    // default — the underlying _W is assumed to support +, -, <
    // arithmetic.
    class iterator
    {
    public:
        typedef random_access_iterator_tag  iterator_category;
        typedef _W                          value_type;
        typedef std::ptrdiff_t              difference_type;
        typedef _W const*                   pointer;
        typedef _W                          reference;

    private:
        _W m_value;


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_value()
        {}

        // value ctor
        D_CONSTEXPR explicit
        iterator(
            _W _v
        )
            : m_value(_v)
        {}


        // operator* — yields a copy of the current value (proxy
        // reference; iota_view's reference type is _W, not _W&).
        D_CONSTEXPR _W
        operator*() const
        {
            return m_value;
        }


        // operator++ (pre / post)
        D_CONSTEXPR_INLINE iterator&
        operator++()
        {
            ++m_value;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_value;
            return tmp;
        }

        // operator-- (pre / post)
        D_CONSTEXPR_INLINE iterator&
        operator--()
        {
            --m_value;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_value;
            return tmp;
        }

        // operator+= / -= (random-access)
        D_CONSTEXPR_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_value = static_cast<_W>(m_value + static_cast<_W>(_n));
            return *this;
        }

        D_CONSTEXPR_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_value = static_cast<_W>(m_value - static_cast<_W>(_n));
            return *this;
        }

        // operator+ (iter, n) / (n, iter)
        D_CONSTEXPR_INLINE iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(static_cast<_W>(m_value + static_cast<_W>(_n)));
        }

        friend D_CONSTEXPR_INLINE iterator
        operator+(
            difference_type _n,
            iterator        _it
        )
        {
            return _it + _n;
        }

        // operator- (iter, n)
        D_CONSTEXPR_INLINE iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(static_cast<_W>(m_value - static_cast<_W>(_n)));
        }

        // operator- (iter, iter)  --  distance
        D_CONSTEXPR_INLINE difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return static_cast<difference_type>(m_value)
                 - static_cast<difference_type>(_rhs.m_value);
        }

        // operator[]
        D_CONSTEXPR _W
        operator[](
            difference_type _n
        ) const
        {
            return static_cast<_W>(m_value + static_cast<_W>(_n));
        }

        // comparisons (==, !=, <, <=, >, >=)
        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return m_value == _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return m_value != _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return m_value < _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return m_value <= _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return m_value > _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator>=(
            iterator const& _rhs
        ) const
        {
            return m_value >= _rhs.m_value;
        }
    };


private:
    _W      m_start;
    _Bound  m_bound;


public:
    // default ctor
    D_CONSTEXPR
    iota_view()
        : m_start(),
          m_bound()
    {}

    // value ctor
    D_CONSTEXPR
    iota_view(
        _W      _start,
        _Bound  _bound
    )
        : m_start(_start),
          m_bound(_bound)
    {}


    // begin / end
    D_CONSTEXPR iterator
    begin() const
    {
        return iterator(m_start);
    }

    D_CONSTEXPR iterator
    end() const
    {
        return iterator(static_cast<_W>(m_bound));
    }

    // size
    //   function: bound - start as a std::size_t. Defined when the
    // arithmetic is well-formed on _W.
    D_CONSTEXPR std::size_t
    size() const
    {
        return static_cast<std::size_t>(
            static_cast<_W>(m_bound) - m_start
        );
    }

    // empty
    //   function: start == bound. Shadows view_interface::empty.
    D_CONSTEXPR bool
    empty() const
    {
        return (m_start == static_cast<_W>(m_bound));
    }
};


// ===========================================================================
// II.  IOTA_VIEW<_W, unreachable_sentinel_t>  (unbounded specialisation)
// ===========================================================================

// iota_view<_W, unreachable_sentinel_t>
//   class: unbounded arithmetic-sequence view. Counts from _start
// upward without an upper bound; end() returns
// unreachable_sentinel_t which never compares equal to the iterator.
// Iteration terminates only when the caller stops calling operator++.
// note: shares the iterator class structure of the primary template
// for consistency, defined inline here to keep file-per-symbol
// granularity (no shared internal helper file).
template<typename _W>
class iota_view<_W, unreachable_sentinel_t>
    : public view_interface<iota_view<_W, unreachable_sentinel_t> >
{
public:
    // iterator
    //   class: identical surface to the bounded form's iterator.
    class iterator
    {
    public:
        typedef random_access_iterator_tag  iterator_category;
        typedef _W                          value_type;
        typedef std::ptrdiff_t              difference_type;
        typedef _W const*                   pointer;
        typedef _W                          reference;

    private:
        _W m_value;

    public:
        D_CONSTEXPR
        iterator()
            : m_value()
        {}

        D_CONSTEXPR explicit
        iterator(
            _W _v
        )
            : m_value(_v)
        {}


        D_CONSTEXPR _W
        operator*() const
        {
            return m_value;
        }


        D_CONSTEXPR_INLINE iterator&
        operator++()
        {
            ++m_value;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_value;
            return tmp;
        }

        D_CONSTEXPR_INLINE iterator&
        operator--()
        {
            --m_value;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_value;
            return tmp;
        }

        D_CONSTEXPR_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_value = static_cast<_W>(m_value + static_cast<_W>(_n));
            return *this;
        }

        D_CONSTEXPR_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_value = static_cast<_W>(m_value - static_cast<_W>(_n));
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(static_cast<_W>(m_value + static_cast<_W>(_n)));
        }

        friend D_CONSTEXPR_INLINE iterator
        operator+(
            difference_type _n,
            iterator        _it
        )
        {
            return _it + _n;
        }

        D_CONSTEXPR_INLINE iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(static_cast<_W>(m_value - static_cast<_W>(_n)));
        }

        D_CONSTEXPR_INLINE difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return static_cast<difference_type>(m_value)
                 - static_cast<difference_type>(_rhs.m_value);
        }

        D_CONSTEXPR _W
        operator[](
            difference_type _n
        ) const
        {
            return static_cast<_W>(m_value + static_cast<_W>(_n));
        }

        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return m_value == _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return m_value != _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return m_value < _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return m_value <= _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return m_value > _rhs.m_value;
        }

        D_CONSTEXPR bool
        operator>=(
            iterator const& _rhs
        ) const
        {
            return m_value >= _rhs.m_value;
        }
    };


private:
    _W m_start;


public:
    // default ctor
    D_CONSTEXPR
    iota_view()
        : m_start()
    {}

    // value ctor (single argument: start; no bound).
    D_CONSTEXPR explicit
    iota_view(
        _W _start
    )
        : m_start(_start)
    {}


    D_CONSTEXPR iterator
    begin() const
    {
        return iterator(m_start);
    }

    // end
    //   function: returns the never-equal sentinel. The view is
    // infinite — pair it with take_view, take_while_view, or
    // similar to obtain a finite range.
    D_CONSTEXPR unreachable_sentinel_t
    end() const
    D_NOEXCEPT
    {
        return unreachable_sentinel_t();
    }

    // size and empty are deliberately omitted — the view has no
    // finite size and is never empty (provided _W is incrementable).
};


// ===========================================================================
// III. ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<iota_view<W, B>>
//   trait: iota_view's iterators hold their position by value, so
// they remain valid past the iota_view's destruction.
template<typename _W,
         typename _Bound>
struct enable_borrowed_range<iota_view<_W, _Bound> >
    : true_type
{};


// ===========================================================================
// IV.  VIEWS::IOTA (colocated CPO-like helper)
// ===========================================================================

namespace views
{
    // views::iota(_start, _bound)
    //   function: returns iota_view<_W, _W> over [_start, _bound).
    template<typename _W>
    D_CONSTEXPR_INLINE
    iota_view<_W, _W>
    iota(
        _W _start,
        _W _bound
    )
    {
        return iota_view<_W, _W>(_start, _bound);
    }

    // views::iota(_start)
    //   function: returns the unbounded iota_view<_W, unreachable_sentinel_t>
    // starting at _start. The result is an infinite view; pair with
    // views::take, views::take_while, or similar to terminate
    // iteration.
    template<typename _W>
    D_CONSTEXPR_INLINE
    iota_view<_W, unreachable_sentinel_t>
    iota(
        _W _start
    )
    {
        return iota_view<_W, unreachable_sentinel_t>(_start);
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_IOTA_VIEW_
