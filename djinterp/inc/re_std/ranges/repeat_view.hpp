/*******************************************************************************
* djinterp [re_std]                                              repeat_view.hpp
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
* path:      /inc/re_std/ranges/repeat_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_REPEAT_VIEW_HPP
#define RE_STD_RANGES_REPEAT_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // ptrdiff_t

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./unreachable_sentinel_t.hpp"
#include "./enable_borrowed_range.hpp"
#include "./movable_box.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   REPEAT_VIEW (primary template — bounded form)
// ===========================================================================

// repeat_view<T, Bound>
//   class: the bounded form. Bound defaults to std::ptrdiff_t.
template<typename T,
         typename Bound = std::ptrdiff_t>
class repeat_view : public view_interface<repeat_view<T, Bound> >
{
public:
    typedef T      value_type;
    typedef Bound  bound_type;


private:
    internal::movable_box<T>   m_value;
    Bound                      m_bound;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    class iterator
    {
    public:
        typedef random_access_iterator_tag                  iterator_category;
        typedef T                                          value_type;
        typedef Bound                                      difference_type;
        typedef T const*                                   pointer;
        typedef T const&                                   reference;


    private:
        T const*       m_value;
        Bound          m_pos;


    public:
        RE_STD_CONSTEXPR
        iterator()
            : m_value(RE_STD_NULLPTR),
              m_pos(0)
        {}

        RE_STD_CONSTEXPR
        iterator(
            T const*   _v,
            Bound      _p
        )
            : m_value(_v),
              m_pos(_p)
        {}


        RE_STD_CONSTEXPR reference
        operator*() const
        {
            return *m_value;
        }


        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator++()
        {
            ++m_pos;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_pos;
            return tmp;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator--()
        {
            --m_pos;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_pos;
            return tmp;
        }


        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_pos += _n;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_pos -= _n;
            return *this;
        }

        RE_STD_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos + _n);
        }

        friend RE_STD_CONSTEXPR iterator
        operator+(
            difference_type _n,
            iterator        _it
        )
        {
            return _it + _n;
        }

        RE_STD_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos - _n);
        }

        RE_STD_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return m_pos - _rhs.m_pos;
        }

        RE_STD_CONSTEXPR reference
        operator[](
            difference_type
        ) const
        {
            return *m_value;
        }


        RE_STD_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return m_pos == _rhs.m_pos;
        }

        RE_STD_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return m_pos != _rhs.m_pos;
        }

        RE_STD_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return m_pos < _rhs.m_pos;
        }

        RE_STD_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return m_pos <= _rhs.m_pos;
        }

        RE_STD_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return m_pos > _rhs.m_pos;
        }

        RE_STD_CONSTEXPR bool
        operator>=(
            iterator const& _rhs
        ) const
        {
            return m_pos >= _rhs.m_pos;
        }
    };


public:
    RE_STD_CONSTEXPR
    repeat_view()
        : m_value(),
          m_bound(0)
    {}

    RE_STD_CONSTEXPR
    repeat_view(
        T      _value,
        Bound  _bound
    )
        : m_value(static_cast<T&&>(_value)),
          m_bound(_bound)
    {}


    RE_STD_CONSTEXPR iterator
    begin() const
    {
        return iterator(&(*m_value), Bound(0));
    }

    RE_STD_CONSTEXPR iterator
    end() const
    {
        return iterator(&(*m_value), m_bound);
    }

    RE_STD_CONSTEXPR Bound
    size() const
    RE_STD_NOEXCEPT
    {
        return m_bound;
    }
};


// ===========================================================================
// II.  REPEAT_VIEW<T, unreachable_sentinel_t>  (unbounded specialisation)
// ===========================================================================

template<typename T>
class repeat_view<T, unreachable_sentinel_t>
    : public view_interface<repeat_view<T, unreachable_sentinel_t> >
{
public:
    typedef T  value_type;


private:
    internal::movable_box<T>   m_value;


public:
    // iterator
    //   class: identical surface to the bounded form's iterator,
    // but with a std::ptrdiff_t position (since no bound limits the
    // value's range).
    class iterator
    {
    public:
        typedef random_access_iterator_tag                  iterator_category;
        typedef T                                          value_type;
        typedef std::ptrdiff_t                              difference_type;
        typedef T const*                                   pointer;
        typedef T const&                                   reference;

    private:
        T const*               m_value;
        std::ptrdiff_t          m_pos;

    public:
        RE_STD_CONSTEXPR
        iterator()
            : m_value(RE_STD_NULLPTR),
              m_pos(0)
        {}

        RE_STD_CONSTEXPR
        iterator(
            T const*       _v,
            std::ptrdiff_t  _p
        )
            : m_value(_v),
              m_pos(_p)
        {}

        RE_STD_CONSTEXPR reference
        operator*() const
        {
            return *m_value;
        }

        RE_STD_CONSTEXPR_CPP14 inline iterator&
        operator++()
        {
            ++m_pos;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 inline iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_pos;
            return tmp;
        }

        RE_STD_CONSTEXPR_CPP14 inline iterator&
        operator--()
        {
            --m_pos;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 inline iterator&
        operator+=(
            difference_type _n
        )
        {
            m_pos += _n;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 inline iterator&
        operator-=(
            difference_type _n
        )
        {
            m_pos -= _n;
            return *this;
        }

        RE_STD_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos + _n);
        }

        RE_STD_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_value, m_pos - _n);
        }

        RE_STD_CONSTEXPR difference_type
        operator-(
            iterator const& _rhs
        ) const
        {
            return m_pos - _rhs.m_pos;
        }

        RE_STD_CONSTEXPR reference
        operator[](
            difference_type
        ) const
        {
            return *m_value;
        }


        RE_STD_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return m_pos == _rhs.m_pos;
        }

        RE_STD_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return m_pos != _rhs.m_pos;
        }
    };


    RE_STD_CONSTEXPR
    repeat_view()
        : m_value()
    {}

    RE_STD_CONSTEXPR explicit
    repeat_view(
        T _value
    )
        : m_value(static_cast<T&&>(_value))
    {}


    RE_STD_CONSTEXPR iterator
    begin() const
    {
        return iterator(&(*m_value), 0);
    }

    RE_STD_CONSTEXPR unreachable_sentinel_t
    end() const
    RE_STD_NOEXCEPT
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
    template<typename T>
    RE_STD_CONSTEXPR_INLINE
    repeat_view<typename decay<T>::type, std::ptrdiff_t>
    repeat(
        T&&            _value,
        std::ptrdiff_t  _bound
    )
    {
        return repeat_view<typename decay<T>::type, std::ptrdiff_t>(
            static_cast<T&&>(_value),
            _bound
        );
    }

    // views::repeat(_value)  [unbounded form]
    template<typename T>
    RE_STD_CONSTEXPR_INLINE
    repeat_view<typename decay<T>::type, unreachable_sentinel_t>
    repeat(
        T&& _value
    )
    {
        return repeat_view<typename decay<T>::type, unreachable_sentinel_t>(
            static_cast<T&&>(_value)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_REPEAT_VIEW_HPP
