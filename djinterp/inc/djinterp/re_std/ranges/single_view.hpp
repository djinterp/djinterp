/******************************************************************************
* djinterp [restd]                                             single_view.hpp
*
* single_view header:
*   Provides the C++20 single-element view. single_view<T> wraps one
* value of T and presents it as a range of size 1. Useful for
* injecting a scalar into a range pipeline, or as a unit element in
* concatenations.
*
*   PORTABILITY:
*   - Requires CRTP + view_interface, available C++11+.
*   - Stores the wrapped value by direct member rather than the C++20
*     'movable-box' wrapper. The movable-box exists to satisfy the
*     C++20 'movable' requirement on view types that may contain
*     non-default-constructible objects; restd's view trait already
*     simplifies the movable/default-init checks, so the direct-
*     storage form is consistent with the rest of the back-port.
*   - Does NOT specialise enable_borrowed_range — single_view owns
*     its element and dereferencing an iterator into a destroyed
*     single_view would be a use-after-free.
*
*   COLOCATED:
*   restd::views::single(t) — function template returning
* single_view<decay_t<T>>{static_cast<T&&>(t)}. Mirrors C++20
* std::views::single as a constructor function.
*
*
* path:      /inc/djinterp/re_std/ranges/single_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_SINGLE_VIEW_
#define DJINTERP_RESTD_RANGES_SINGLE_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>

#include "../type_traits/type_traits.hpp"
#include "./view_interface.hpp"


NS_RESTD


// ===========================================================================
// I.   SINGLE_VIEW
// ===========================================================================

// single_view<_Type>
//   class: holds one _Type by value and exposes the standard range
// interface over it. _Type must be a non-reference, non-cv object
// type — same constraint as the C++20 std::ranges::single_view.
template<typename _Type>
class single_view : public view_interface<single_view<_Type> >
{
private:
    _Type m_value;


public:
    // default ctor
    //   function: value-initialises the held _Type. Requires _Type
    // to be default-constructible.
    D_CONSTEXPR
    single_view()
        : m_value()
    {}

    // value ctor (const&)
    //   function: copies _t into the held value.
    D_CONSTEXPR
    single_view(
        _Type const& _t
    )
        : m_value(_t)
    {}

    // value ctor (&&)
    //   function: moves _t into the held value.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    D_CONSTEXPR
    single_view(
        _Type&& _t
    )
        : m_value(static_cast<_Type&&>(_t))
    {}
#endif


    // begin / end / data — pointer pair to the held value.
    D_CONSTEXPR _Type*
    begin()
    D_NOEXCEPT
    {
        return &m_value;
    }

    D_CONSTEXPR _Type const*
    begin() const
    D_NOEXCEPT
    {
        return &m_value;
    }

    D_CONSTEXPR _Type*
    end()
    D_NOEXCEPT
    {
        return (&m_value) + 1;
    }

    D_CONSTEXPR _Type const*
    end() const
    D_NOEXCEPT
    {
        return (&m_value) + 1;
    }

    D_CONSTEXPR _Type*
    data()
    D_NOEXCEPT
    {
        return &m_value;
    }

    D_CONSTEXPR _Type const*
    data() const
    D_NOEXCEPT
    {
        return &m_value;
    }

    // size
    //   function: always 1.
    static D_CONSTEXPR std::size_t
    size()
    D_NOEXCEPT
    {
        return 1;
    }

    // empty
    //   function: always false. Shadows view_interface::empty.
    static D_CONSTEXPR bool
    empty()
    D_NOEXCEPT
    {
        return false;
    }
};


// ===========================================================================
// II.  VIEWS::SINGLE (colocated CPO-like helper)
// ===========================================================================

namespace views
{
    // views::single(_t)
    //   function: returns single_view<decay_t<_T>> constructed from
    // _t. Decays array and function types per the C++20 contract,
    // and strips references / cv so the resulting view stores a
    // plain object.
    template<typename _T>
    D_CONSTEXPR_INLINE
    single_view<typename decay<_T>::type>
    single(
        _T&& _t
    )
    {
        return single_view<typename decay<_T>::type>(
            static_cast<_T&&>(_t)
        );
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_SINGLE_VIEW_
