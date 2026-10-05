/*******************************************************************************
* djinterp [re_std]                                              single_view.hpp
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
*     non-default-constructible objects; re_std's view trait already
*     simplifies the movable/default-init checks, so the direct-
*     storage form is consistent with the rest of the back-port.
*   - Does NOT specialise enable_borrowed_range — single_view owns
*     its element and dereferencing an iterator into a destroyed
*     single_view would be a use-after-free.
*
*   COLOCATED:
*   re_std::views::single(t) — function template returning
* single_view<decay_t<T>>{static_cast<T&&>(t)}. Mirrors C++20
* std::views::single as a constructor function.
*
*
* path:      /inc/re_std/ranges/single_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_SINGLE_VIEW_HPP
#define RE_STD_RANGES_SINGLE_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>

#include "../type_traits/type_traits.hpp"
#include "./view_interface.hpp"


namespace re_std
{


// ===========================================================================
// I.   SINGLE_VIEW
// ===========================================================================

// single_view<Type>
//   class: holds one Type by value and exposes the standard range
// interface over it. Type must be a non-reference, non-cv object
// type — same constraint as the C++20 std::ranges::single_view.
template<typename Type>
class single_view : public view_interface<single_view<Type> >
{
private:
    Type m_value;


public:
    // default ctor
    //   function: value-initialises the held Type. Requires Type
    // to be default-constructible.
    RE_STD_CONSTEXPR
    single_view()
        : m_value()
    {}

    // value ctor (const&)
    //   function: copies _t into the held value.
    RE_STD_CONSTEXPR
    single_view(
        Type const& _t
    )
        : m_value(_t)
    {}

    // value ctor (&&)
    //   function: moves _t into the held value.
#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    RE_STD_CONSTEXPR
    single_view(
        Type&& _t
    )
        : m_value(static_cast<Type&&>(_t))
    {}
#endif


    // begin / end / data — pointer pair to the held value.
    RE_STD_CONSTEXPR_CPP14 Type*
    begin()
    RE_STD_NOEXCEPT
    {
        return &m_value;
    }

    RE_STD_CONSTEXPR Type const*
    begin() const
    RE_STD_NOEXCEPT
    {
        return &m_value;
    }

    RE_STD_CONSTEXPR_CPP14 Type*
    end()
    RE_STD_NOEXCEPT
    {
        return (&m_value) + 1;
    }

    RE_STD_CONSTEXPR Type const*
    end() const
    RE_STD_NOEXCEPT
    {
        return (&m_value) + 1;
    }

    RE_STD_CONSTEXPR_CPP14 Type*
    data()
    RE_STD_NOEXCEPT
    {
        return &m_value;
    }

    RE_STD_CONSTEXPR Type const*
    data() const
    RE_STD_NOEXCEPT
    {
        return &m_value;
    }

    // size
    //   function: always 1.
    static RE_STD_CONSTEXPR std::size_t
    size()
    RE_STD_NOEXCEPT
    {
        return 1;
    }

    // empty
    //   function: always false. Shadows view_interface::empty.
    static RE_STD_CONSTEXPR bool
    empty()
    RE_STD_NOEXCEPT
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
    //   function: returns single_view<decay_t<T>> constructed from
    // _t. Decays array and function types per the C++20 contract,
    // and strips references / cv so the resulting view stores a
    // plain object.
    template<typename T>
    RE_STD_CONSTEXPR_INLINE
    single_view<typename decay<T>::type>
    single(
        T&& _t
    )
    {
        return single_view<typename decay<T>::type>(
            static_cast<T&&>(_t)
        );
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_SINGLE_VIEW_HPP
