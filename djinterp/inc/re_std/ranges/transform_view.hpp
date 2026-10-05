/*******************************************************************************
* djinterp [re_std]                                           transform_view.hpp
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
* reference. Re_std retains the underlying iterator_category
* unchanged. For pure functions this is correct; for impure F that
* returns a prvalue, the resulting iterator may report a stronger
* category than it strictly models. Sufficient for SFINAE constraint
* use; not a strict iterator-concept binding.
*
*   COLOCATED:
*   re_std::views::transform(r, f).
*
*
* path:      /inc/re_std/ranges/transform_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_TRANSFORM_VIEW_HPP
#define RE_STD_RANGES_TRANSFORM_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/iterator_traits.hpp"
#include "./view_interface.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"
#include "./movable_box.hpp"
#include "./all.hpp"
#include "./range_adaptor_closure.hpp"


namespace re_std
{


// ===========================================================================
// I.   TRANSFORM_VIEW
// ===========================================================================

// transform_view<View, Fn>
//   class: lazy projection of View through Fn. Fn is invoked on
// dereference; no transformed elements are stored.
template<typename View,
         typename Fn>
class transform_view : public view_interface<transform_view<View, Fn> >
{
public:
    typedef View   base_view;
    typedef Fn     function_type;


private:
    View                       m_base;
    internal::movable_box<Fn>  m_fn;


public:
    // =======================================================
    // I.A   NESTED ITERATOR
    // =======================================================

    // iterator
    //   class: wraps iterator_t<View> + a back-pointer to the
    // parent transform_view. Operator* applies the parent's stored
    // function to the underlying iterator's deref.
    class iterator
    {
    public:
        typedef typename iterator_traits<
                              iterator_t<View>
                          >::iterator_category   iterator_category;

        typedef typename iterator_traits<
                              iterator_t<View>
                          >::difference_type     difference_type;

        // reference: result of m_fn(*m_it). Captured via decltype.
        // value_type strips refs/cv from reference.
        typedef decltype(
                    declval<Fn const&>()(
                        *declval<iterator_t<View>&>()
                    )
                )                                  reference;

        typedef typename decay<reference>::type    value_type;

        // pointer: void — the transformed reference may be a
        // prvalue, so there is no meaningful pointer-to-element.
        typedef void                               pointer;


    private:
        iterator_t<View>          m_it;
        transform_view const*      m_parent;


    public:
        // default ctor
        RE_STD_CONSTEXPR
        iterator()
            : m_it(),
              m_parent(RE_STD_NULLPTR)
        {}

        // value ctor
        RE_STD_CONSTEXPR
        iterator(
            transform_view const*  _parent,
            iterator_t<View>      _it
        )
            : m_it(_it),
              m_parent(_parent)
        {}


        // base — exposes the underlying iterator.
        RE_STD_CONSTEXPR iterator_t<View>
        base() const
        {
            return m_it;
        }


        // operator*
        //   function: applies the parent's function to the
        // underlying iterator's dereference and returns the result.
        RE_STD_CONSTEXPR reference
        operator*() const
        {
            return (*(m_parent->m_fn))(*m_it);
        }


        // operator++ (pre / post)
        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator++()
        {
            ++m_it;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator
        operator++(int)
        {
            iterator tmp = *this;
            ++m_it;
            return tmp;
        }


        // operator-- (pre / post) -- only well-formed when
        // underlying is bidirectional. SFINAE'd via lazy
        // instantiation.
        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator--()
        {
            --m_it;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator
        operator--(int)
        {
            iterator tmp = *this;
            --m_it;
            return tmp;
        }


        // random-access ops -- well-formed when underlying is RA.
        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator+=(
            difference_type _n
        )
        {
            m_it += _n;
            return *this;
        }

        RE_STD_CONSTEXPR_CPP14 RE_STD_INLINE iterator&
        operator-=(
            difference_type _n
        )
        {
            m_it -= _n;
            return *this;
        }

        RE_STD_CONSTEXPR iterator
        operator+(
            difference_type _n
        ) const
        {
            return iterator(m_parent, m_it + _n);
        }

        RE_STD_CONSTEXPR iterator
        operator-(
            difference_type _n
        ) const
        {
            return iterator(m_parent, m_it - _n);
        }

        RE_STD_CONSTEXPR
        auto
        operator-(
            iterator const& _rhs
        ) const
            -> decltype(m_it - _rhs.m_it)
        {
            return (m_it - _rhs.m_it);
        }

        RE_STD_CONSTEXPR reference
        operator[](
            difference_type _n
        ) const
        {
            return (*(m_parent->m_fn))(m_it[_n]);
        }


        // comparisons (delegate to underlying iterator)
        RE_STD_CONSTEXPR bool
        operator==(
            iterator const& _rhs
        ) const
        {
            return (m_it == _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator!=(
            iterator const& _rhs
        ) const
        {
            return (m_it != _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator<(
            iterator const& _rhs
        ) const
        {
            return (m_it < _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator<=(
            iterator const& _rhs
        ) const
        {
            return (m_it <= _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
        operator>(
            iterator const& _rhs
        ) const
        {
            return (m_it > _rhs.m_it);
        }

        RE_STD_CONSTEXPR bool
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
    //   class: thin wrapper over sentinel_t<View>. Compares equal
    // to iterator when the underlying iterators compare equal.
    class sentinel
    {
    private:
        sentinel_t<View>  m_end;


    public:
        RE_STD_CONSTEXPR
        sentinel()
            : m_end()
        {}

        RE_STD_CONSTEXPR explicit
        sentinel(
            sentinel_t<View>  _e
        )
            : m_end(_e)
        {}


        RE_STD_CONSTEXPR sentinel_t<View>
        base() const
        {
            return m_end;
        }


        friend RE_STD_CONSTEXPR bool
        operator==(
            iterator const&  _it,
            sentinel const&  _s
        )
        {
            return (_it.base() == _s.m_end);
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
    // default ctor
    RE_STD_CONSTEXPR
    transform_view()
        : m_base(),
          m_fn()
    {}

    // value ctor
    //   function: takes the underlying view and the projection
    // function. Both are moved in.
    RE_STD_CONSTEXPR
    transform_view(
        View  _base,
        Fn    _fn
    )
        : m_base(static_cast<View&&>(_base)),
          m_fn(static_cast<Fn&&>(_fn))
    {}


    // base
    //   function: returns a copy of the underlying view.
    RE_STD_CONSTEXPR View
    base() const
    {
        return m_base;
    }


    // begin / end
    RE_STD_CONSTEXPR iterator
    begin()
    {
        return iterator(this, re_std::begin(m_base));
    }

    RE_STD_CONSTEXPR sentinel
    end()
    {
        return sentinel(re_std::end(m_base));
    }


    // size — forwards to the underlying view's size when sized.
    RE_STD_CONSTEXPR
    auto
    size() const
        -> decltype(re_std::size(m_base))
    {
        return re_std::size(m_base);
    }
};


// ===========================================================================
// II.  TRANSFORM_CLOSURE (bound form for pipe syntax)
// ===========================================================================

namespace internal
{

// transform_closure
//   class: the bound form of views::transform. Holds a function
// and, when invoked with a range, constructs the transform_view
// directly.
template<typename Fn>
struct transform_closure : range_adaptor_closure<transform_closure<Fn> >
{
    Fn fn;

    RE_STD_CONSTEXPR
    transform_closure()
        : fn()
    {}

    RE_STD_CONSTEXPR explicit
    transform_closure(
        Fn _f
    )
        : fn(static_cast<Fn&&>(_f))
    {}

    template<typename R>
    RE_STD_CONSTEXPR_INLINE
    transform_view<typename internal::all_dispatch<R>::type, Fn>
    operator()(
        R&&  _r
    ) const
    {
        typedef typename internal::all_dispatch<R>::type view_type;
        return transform_view<view_type, Fn>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            fn
        );
    }
};

}  // internal


// ===========================================================================
// III. VIEWS::TRANSFORM
// ===========================================================================

namespace views
{
    // views::transform(_r, _fn)  [direct form]
    template<typename R,
             typename Fn>
    RE_STD_CONSTEXPR_INLINE
    transform_view<typename internal::all_dispatch<R>::type,
                   typename decay<Fn>::type>
    transform(
        R&&  _r,
        Fn&& _fn
    )
    {
        typedef typename internal::all_dispatch<R>::type  view_type;
        typedef typename decay<Fn>::type                  fn_type;
        return transform_view<view_type, fn_type>(
            internal::all_dispatch<R>::call(static_cast<R&&>(_r)),
            static_cast<Fn&&>(_fn)
        );
    }

    // views::transform(_fn)  [bound form]
    //   function: returns a transform_closure for pipe composition.
    template<typename Fn>
    RE_STD_CONSTEXPR_INLINE
    internal::transform_closure<typename decay<Fn>::type>
    transform(
        Fn&& _fn
    )
    {
        return internal::transform_closure<typename decay<Fn>::type>(
            static_cast<Fn&&>(_fn)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_TRANSFORM_VIEW_HPP
