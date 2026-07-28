/******************************************************************************
* djinterp [restd]                                          transform_view.hpp
*
* transform_view header:
*   Provides the C++20 lazy-projection adaptor. transform_view<V, F>
* presents an underlying view V with each element transformed by a
* function F. The function is applied lazily on dereference; no
* storage is allocated for the transformed elements.
*
*   PORTABILITY:
*   - C++11+; CRTP + view_interface + custom iterator class.
*   - The function F is stored by value (move-construction
*     required). The C++20 'movable-box' wrapper is omitted; F must
*     be at least movable. Default construction of transform_view
*     requires F to be default-constructible.
*   - The iterator holds a back-pointer to its parent transform_view
*     to access F at dereference time. As a consequence,
*     transform_view::iterator does not satisfy borrowed_range — an
*     iterator that outlives its parent is dangling. (No
*     enable_borrowed_range specialisation is provided.)
*
*   SIMPLIFICATION RELATIVE TO C++20:
*   The C++20 spec sets iterator_category to a strict combination
* of the underlying iterator's category AND whether F returns a
* reference. Restd retains the underlying iterator_category
* unchanged. For pure functions this is correct; for impure F that
* returns a prvalue, the resulting iterator may report a stronger
* category than it strictly models. Sufficient for SFINAE constraint
* use; not a strict iterator-concept binding.
*
*   COLOCATED:
*   restd::views::transform(r, f).
*
*
* path:      /inc/djinterp/restd/ranges/transform_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_TRANSFORM_VIEW_
#define DJINTERP_RESTD_RANGES_TRANSFORM_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


NS_RESTD


// ===========================================================================
// I.   TRANSFORM_VIEW
// ===========================================================================

// transform_view<_View, _Fn>
//   class: lazy projection of _View through _Fn. _Fn is invoked on
// dereference; no transformed elements are stored.
template<typename _View,
         typename _Fn>
class transform_view : public view_interface<transform_view<_View, _Fn> >
{
public:
    typedef _View   base_view;
    typedef _Fn     function_type;


private:
    _View                       m_base;
    internal::movable_box<_Fn>  m_fn;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: wraps iterator_t<_View> + a back-pointer to the
    // parent transform_view. Operator* applies the parent's stored
    // function to the underlying iterator's deref.
    class iterator
    {
    public:
        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::iterator_category   iterator_category;

        typedef typename iterator_traits<
                              iterator_t<_View>
                          >::difference_type     difference_type;

        // reference: result of m_fn(*m_it). Captured via decltype.
        // value_type strips refs/cv from reference.
        typedef decltype(
                    declval<_Fn const&>()(
                        *declval<iterator_t<_View>&>()
                    )
                )                                  reference;

        typedef typename decay<reference>::type    value_type;

        // pointer: void — the transformed reference may be a
        // prvalue, so there is no meaningful pointer-to-element.
        typedef void                               pointer;


    private:
        iterator_t<_View>          m_it;
        transform_view const*      m_parent;


    public:
        // default ctor
        D_CONSTEXPR
        iterator()
            : m_it(),
              m_parent(D_NULLPTR)
        {}

        // value ctor
        D_CONSTEXPR
        iterator(
            transform_view const*  _parent,
            iterator_t<_View>      _it
        )
            : m_it(_it),
              m_parent(_parent)
        {}


        // base — exposes the underlying iterator.
        D_CONSTEXPR iterator_t<_View>
        base() const
        {
            return m_it;
        }


        // operator*
        //   function: applies the parent's function to the
        // underlying iterator's dereference and returns the result.
        D_CONSTEXPR reference
        operator*() const
        {
            return (*(m_parent->m_fn))(*m_it);
        }


        // operator++ (pre / post)
        D_CONSTEXPR_INLINE iterator&
        operator++()
        {
            ++m_it;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_it;
            return tmp;
        }


        // operator-- (pre / post) -- only well-formed when
        // underlying is bidirectional. SFINAE'd via lazy
        // instantiation.
        D_CONSTEXPR_INLINE iterator&
        operator--()
        {
            --m_it;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_it;
            return tmp;
        }


        // random-access ops -- well-formed when underlying is RA.
        D_CONSTEXPR_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_it += _n;
            return *this;
        }

        D_CONSTEXPR_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_it -= _n;
            return *this;
        }

        D_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_parent, m_it + _n);
        }

        D_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_parent, m_it - _n);
        }

        D_CONSTEXPR
        auto
        operator-(
            iterator const& _rhs
        ) const
            -> decltype(m_it - _rhs.m_it)
        {
            return (m_it - _rhs.m_it);
        }

        D_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return (*(m_parent->m_fn))(m_it[_n]);
        }


        // comparisons (delegate to underlying iterator)
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

        D_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return (m_it < _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return (m_it <= _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return (m_it > _rhs.m_it);
        }

        D_CONSTEXPR bool
        operator>=(
            iterator const& _rhs
        ) const
        {
            return (m_it >= _rhs.m_it);
        }
    };


    // =======================================================
    // I.B   NESTED SENTINEL
    // =======================================================

    // sentinel
    //   class: thin wrapper over sentinel_t<_View>. Compares equal
    // to iterator when the underlying iterators compare equal.
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
    transform_view()
        : m_base(),
          m_fn()
    {}

    // value ctor
    //   function: takes the underlying view and the projection
    // function. Both are moved in.
    D_CONSTEXPR
    transform_view(
        _View  _base,
        _Fn    _fn
    )
        : m_base(static_cast<_View&&>(_base)),
          m_fn(static_cast<_Fn&&>(_fn))
    {}


    // base
    //   function: returns a copy of the underlying view.
    D_CONSTEXPR _View
    base() const
    {
        return m_base;
    }


    // begin / end
    D_CONSTEXPR iterator
    begin()
    {
        return iterator(this, restd::begin(m_base));
    }

    D_CONSTEXPR sentinel
    end()
    {
        return sentinel(restd::end(m_base));
    }


    // size — forwards to the underlying view's size when sized.
    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_base))
    {
        return restd::size(m_base);
    }
};


// ===========================================================================
// II.  TRANSFORM_CLOSURE (bound form for pipe syntax)
// ===========================================================================

NS_INTERNAL

// transform_closure
//   class: the bound form of views::transform. Holds a function
// and, when invoked with a range, constructs the transform_view
// directly.
template<typename _Fn>
struct transform_closure : range_adaptor_closure<transform_closure<_Fn> >
{
    _Fn fn;

    D_CONSTEXPR
    transform_closure()
        : fn()
    {}

    D_CONSTEXPR explicit
    transform_closure(
        _Fn _f
    )
        : fn(static_cast<_Fn&&>(_f))
    {}

    template<typename _R>
    D_CONSTEXPR_INLINE
    transform_view<typename internal::all_dispatch<_R>::type, _Fn>
    operator()(
        _R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<_R>::type view_type;
        return transform_view<view_type, _Fn>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            fn
        );
    }
};

NS_END  // internal


// ===========================================================================
// III. VIEWS::TRANSFORM
// ===========================================================================

namespace views
{
    // views::transform(_r, _fn)  [direct form]
    template<typename _R,
             typename _Fn>
    D_CONSTEXPR_INLINE
    transform_view<typename internal::all_dispatch<_R>::type,
                   typename decay<_Fn>::type>
    transform(
        _R&&  _r,
        _Fn&& _fn
    )
    {
        typedef typename internal::all_dispatch<_R>::type  view_type;
        typedef typename decay<_Fn>::type                  fn_type;
        return transform_view<view_type, fn_type>(
            internal::all_dispatch<_R>::call(static_cast<_R&&>(_r)),
            static_cast<_Fn&&>(_fn)
        );
    }

    // views::transform(_fn)  [bound form]
    //   function: returns a transform_closure for pipe composition.
    template<typename _Fn>
    D_CONSTEXPR_INLINE
    internal::transform_closure<typename decay<_Fn>::type>
    transform(
        _Fn&& _fn
    )
    {
        return internal::transform_closure<typename decay<_Fn>::type>(
            static_cast<_Fn&&>(_fn)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_TRANSFORM_VIEW_
