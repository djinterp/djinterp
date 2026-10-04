/******************************************************************************
* djinterp [re_std]                                            repeat_view.hpp
*
* repeat_view header:
*   Provides the C++23 repeat adaptor. repeat_view<T, Bound> yields
* the same value Bound times (bounded form, with a count) or
* forever (unbounded form, with Bound = unreachable_sentinel_t).
* The stored value is shared across all dereferences.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + nested iterator + sentinel.
*   - Two forms: repeat_view<T, ptrdiff_t> (bounded) and
*     repeat_view<T, unreachable_sentinel_t> (unbounded).
*   - R26: T is now stored inside internal::movable_box<T>. The
*     view's default ctor is well-formed for ANY T — non-default-
*     constructible T leaves the box empty (dereferencing iterators
*     on such a view is UB until the value ctor populates the box).
*     The C++23 spec achieves the same with its exposition-only
*     movable-box; re_std's is the version shipped in R22.
*   - Specialises enable_borrowed_range to true on both forms —
*     the iterators carry their position by value (and a pointer to
*     the shared T inside the view); they remain valid past the
*     view's destruction so long as T's storage outlives the
*     dereferences (the iterator stores a pointer to the view's
*     internal T, so this only holds when the view itself is still
*     alive at deref time; users should not extract iterators past
*     view lifetime).
*
*   COLOCATED:
*   re_std::views::repeat(value, n) — bounded.
*   re_std::views::repeat(value)    — unbounded.
*
*
* path:      /inc/djinterp/re_std/ranges/repeat_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_RANGES_REPEAT_VIEW_
#define DJINTERP_RE_STD_RANGES_REPEAT_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // ptrdiff_t

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./unreachable_sentinel_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./movable_box.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   REPEAT_VIEW (primary template — bounded form)
// ===========================================================================

// repeat_view<_T, _Bound>
//   class: the bounded form. _Bound defaults to std::ptrdiff_t.
template<typename _T,
         typename _Bound = std::ptrdiff_t>
class repeat_view : public view_interface<repeat_view<_T, _Bound> >
{
public:
    typedef _T      value_type;
    typedef _Bound  bound_type;


private:
    internal::movable_box<_T>   m_value;
    _Bound                      m_bound;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    class iterator
    {
    public:
        typedef random_access_iterator_tag                  iterator_category;
        typedef _T                                          value_type;
        typedef _Bound                                      difference_type;
        typedef _T const*                                   pointer;
        typedef _T const&                                   reference;


    private:
        _T const*       m_value;
        _Bound          m_pos;


    public:
        D_CONSTEXPR
        iterator()
            : m_value(D_NULLPTR),
              m_pos(0)
        {}

        D_CONSTEXPR
        iterator(
            _T const*   _v,
            _Bound      _p
        )
            : m_value(_v),
              m_pos(_p)
        {}


        D_CONSTEXPR reference
        operator*() const
        {
            return *m_value;
        }


        D_CONSTEXPR_INLINE iterator&
        operator++()
        {
            ++m_pos;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_pos;
            return tmp;
        }

        D_CONSTEXPR_INLINE iterator&
        operator--()
        {
            --m_pos;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_pos;
            return tmp;
        }


        D_CONSTEXPR_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_pos += _n;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_pos -= _n;
            return *this;
        }

        D_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos + _n);
        }

        friend D_CONSTEXPR iterator
        operator+(
            difference_type _n,
            iterator        _it
        )
        {
            return _it + _n;
        }

        D_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos - _n);
        }

        D_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return m_pos - _rhs.m_pos;
        }

        D_CONSTEXPR reference
        operator[](
            difference_type
        ) const
        {
            return *m_value;
        }


        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return m_pos == _rhs.m_pos;
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return m_pos != _rhs.m_pos;
        }

        D_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return m_pos < _rhs.m_pos;
        }

        D_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return m_pos <= _rhs.m_pos;
        }

        D_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return m_pos > _rhs.m_pos;
        }

        D_CONSTEXPR bool
        operator>=(
            iterator const& _rhs
        ) const
        {
            return m_pos >= _rhs.m_pos;
        }
    };


public:
    D_CONSTEXPR
    repeat_view()
        : m_value(),
          m_bound(0)
    {}

    D_CONSTEXPR
    repeat_view(
        _T      _value,
        _Bound  _bound
    )
        : m_value(static_cast<_T&&>(_value)),
          m_bound(_bound)
    {}


    D_CONSTEXPR iterator
    begin() const
    {
        return iterator(&(*m_value), _Bound(0));
    }

    D_CONSTEXPR iterator
    end() const
    {
        return iterator(&(*m_value), m_bound);
    }

    D_CONSTEXPR _Bound
    size() const
    D_NOEXCEPT
    {
        return m_bound;
    }
};


// ===========================================================================
// II.  REPEAT_VIEW<_T, unreachable_sentinel_t>  (unbounded specialisation)
// ===========================================================================

template<typename _T>
class repeat_view<_T, unreachable_sentinel_t>
    : public view_interface<repeat_view<_T, unreachable_sentinel_t> >
{
public:
    typedef _T  value_type;


private:
    internal::movable_box<_T>   m_value;


public:
    // iterator
    //   class: identical surface to the bounded form's iterator,
    // but with a std::ptrdiff_t position (since no bound limits the
    // value's range).
    class iterator
    {
    public:
        typedef random_access_iterator_tag                  iterator_category;
        typedef _T                                          value_type;
        typedef std::ptrdiff_t                              difference_type;
        typedef _T const*                                   pointer;
        typedef _T const&                                   reference;

    private:
        _T const*               m_value;
        std::ptrdiff_t          m_pos;

    public:
        D_CONSTEXPR
        iterator()
            : m_value(D_NULLPTR),
              m_pos(0)
        {}

        D_CONSTEXPR
        iterator(
            _T const*       _v,
            std::ptrdiff_t  _p
        )
            : m_value(_v),
              m_pos(_p)
        {}

        D_CONSTEXPR reference
        operator*() const
        {
            return *m_value;
        }

        D_CONSTEXPR_CPP14 inline iterator&
        operator++()
        {
            ++m_pos;
            return *this;
        }

        D_CONSTEXPR_CPP14 inline iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_pos;
            return tmp;
        }

        D_CONSTEXPR_CPP14 inline iterator&
        operator--()
        {
            --m_pos;
            return *this;
        }

        D_CONSTEXPR_CPP14 inline iterator&
        operator+=(
            difference_type _n
        )
        {
            m_pos += _n;
            return *this;
        }

        D_CONSTEXPR_CPP14 inline iterator&
        operator-=(
            difference_type _n
        )
        {
            m_pos -= _n;
            return *this;
        }

        D_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos + _n);
        }

        D_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos - _n);
        }

        D_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return m_pos - _rhs.m_pos;
        }

        D_CONSTEXPR reference
        operator[](
            difference_type
        ) const
        {
            return *m_value;
        }


        D_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return m_pos == _rhs.m_pos;
        }

        D_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return m_pos != _rhs.m_pos;
        }
    };


    D_CONSTEXPR
    repeat_view()
        : m_value()
    {}

    D_CONSTEXPR explicit
    repeat_view(
        _T _value
    )
        : m_value(static_cast<_T&&>(_value))
    {}


    D_CONSTEXPR iterator
    begin() const
    {
        return iterator(&(*m_value), 0);
    }

    D_CONSTEXPR unreachable_sentinel_t
    end() const
    D_NOEXCEPT
    {
        return unreachable_sentinel_t();
    }

    // size and empty deliberately omitted — infinite.
};


// ===========================================================================
// III. ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// repeat_view is technically NOT borrowed in the std sense — the
// iterators hold a pointer to the view's stored T. We do NOT
// specialise enable_borrowed_range so the default (false) applies.
// Users who need iterators that survive the view must materialise.


// ===========================================================================
// IV.  VIEWS::REPEAT
// ===========================================================================

namespace views
{
    // views::repeat(_value, _bound)  [bounded form]
    template<typename _T>
    D_CONSTEXPR_INLINE
    repeat_view<typename decay<_T>::type, std::ptrdiff_t>
    repeat(
        _T&&            _value,
        std::ptrdiff_t  _bound
    )
    {
        return repeat_view<typename decay<_T>::type, std::ptrdiff_t>(
            static_cast<_T&&>(_value),
            _bound
        );
    }

    // views::repeat(_value)  [unbounded form]
    template<typename _T>
    D_CONSTEXPR_INLINE
    repeat_view<typename decay<_T>::type, unreachable_sentinel_t>
    repeat(
        _T&& _value
    )
    {
        return repeat_view<typename decay<_T>::type, unreachable_sentinel_t>(
            static_cast<_T&&>(_value)
        );
    }
}  // namespace views


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RANGES_REPEAT_VIEW_
