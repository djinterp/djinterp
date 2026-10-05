/*******************************************************************************
* djinterp [re_std]                                               empty_view.hpp
*
* empty_view header:
*   Provides the C++20 zero-element view. empty_view<T> models a
* range of zero T values — useful as a neutral element in range
* algorithms, as a placeholder for conditional branches that
* return a view of T, and in template metaprogramming on view types.
*
*   PORTABILITY:
*   - Requires CRTP + view_interface, available C++11+.
*   - Begin/end/data are static constexpr member functions, matching
*     the C++20 contract (callable without an instance).
*   - Specialises enable_borrowed_range<empty_view<T>> to true:
*     there is no underlying storage to invalidate, so the
*     null pointers it yields are trivially borrowed.
*
*   COLOCATED:
*   re_std::views::empty<T>() — function template that returns an
* empty_view<T>{}. The C++20 spelling is the variable template
* views::empty<T> (no parens); the function form is portable across
* C++11–17 and equally cheap (returns an empty class by value).
*
*
* path:      /inc/re_std/ranges/empty_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_EMPTY_VIEW_HPP
#define RE_STD_RANGES_EMPTY_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>  // size_t

#include "./view_interface.hpp"
#include "./enable_borrowed_range.hpp"


namespace re_std
{


// ===========================================================================
// I.   EMPTY_VIEW
// ===========================================================================

// empty_view<Type>
//   class: zero-element view of Type. All accessors are static
// constexpr — no state is held.
template<typename Type>
class empty_view : public view_interface<empty_view<Type> >
{
public:
    // begin
    //   function: returns nullptr-cast-to-Type*. Static — no
    // instance required.
    static RE_STD_CONSTEXPR Type*
    begin()
    RE_STD_NOEXCEPT
    {
        return RE_STD_NULLPTR;
    }

    // end
    //   function: returns nullptr-cast-to-Type*. begin() == end()
    // is the empty invariant.
    static RE_STD_CONSTEXPR Type*
    end()
    RE_STD_NOEXCEPT
    {
        return RE_STD_NULLPTR;
    }

    // data
    //   function: returns nullptr. Defined so contiguous-range users
    // get a valid (if degenerate) pointer.
    static RE_STD_CONSTEXPR Type*
    data()
    RE_STD_NOEXCEPT
    {
        return RE_STD_NULLPTR;
    }

    // size
    //   function: always 0. Type matches std::size_t for symmetry
    // with the C++20 contract.
    static RE_STD_CONSTEXPR std::size_t
    size()
    RE_STD_NOEXCEPT
    {
        return 0;
    }

    // empty
    //   function: always true. Shadows view_interface::empty for the
    // trivial answer.
    static RE_STD_CONSTEXPR bool
    empty()
    RE_STD_NOEXCEPT
    {
        return true;
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<empty_view<Type>>
//   trait: empty_view is a borrowed_range. The (null) iterators
// remain valid past the empty_view's lifetime trivially.
template<typename Type>
struct enable_borrowed_range<empty_view<Type> >
    : true_type
{};


// ===========================================================================
// III. VIEWS::EMPTY (colocated CPO-like helper)
// ===========================================================================

namespace views
{
    // views::empty<Type>()
    //   function: returns an empty_view<Type> instance. Function-
    // template form for portability across C++11+; the C++20
    // variable-template spelling 'views::empty<int>' is not provided
    // here because variable templates are C++14+ AND because the
    // value-initialised return is identical in cost.
    template<typename Type>
    RE_STD_CONSTEXPR_INLINE
    empty_view<Type>
    empty()
    RE_STD_NOEXCEPT
    {
        return empty_view<Type>();
    }
}  // namespace views


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_EMPTY_VIEW_HPP
