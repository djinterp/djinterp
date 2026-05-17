/******************************************************************************
* djinterp [restd]                                              empty_view.hpp
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
*   restd::views::empty<T>() — function template that returns an
* empty_view<T>{}. The C++20 spelling is the variable template
* views::empty<T> (no parens); the function form is portable across
* C++11–17 and equally cheap (returns an empty class by value).
*
*
* path:      /inc/djinterp/restd/ranges/empty_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_EMPTY_VIEW_
#define DJINTERP_RESTD_RANGES_EMPTY_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <cstddef>  // size_t

#include "./view_interface.hpp"
#include "./enable_borrowed_range.hpp"


NS_RESTD


// ===========================================================================
// I.   EMPTY_VIEW
// ===========================================================================

// empty_view<_Type>
//   class: zero-element view of _Type. All accessors are static
// constexpr — no state is held.
template<typename _Type>
class empty_view : public view_interface<empty_view<_Type> >
{
public:
    // begin
    //   function: returns nullptr-cast-to-_Type*. Static — no
    // instance required.
    static D_CONSTEXPR _Type*
    begin()
    D_NOEXCEPT
    {
        return D_NULLPTR;
    }

    // end
    //   function: returns nullptr-cast-to-_Type*. begin() == end()
    // is the empty invariant.
    static D_CONSTEXPR _Type*
    end()
    D_NOEXCEPT
    {
        return D_NULLPTR;
    }

    // data
    //   function: returns nullptr. Defined so contiguous-range users
    // get a valid (if degenerate) pointer.
    static D_CONSTEXPR _Type*
    data()
    D_NOEXCEPT
    {
        return D_NULLPTR;
    }

    // size
    //   function: always 0. Type matches std::size_t for symmetry
    // with the C++20 contract.
    static D_CONSTEXPR std::size_t
    size()
    D_NOEXCEPT
    {
        return 0;
    }

    // empty
    //   function: always true. Shadows view_interface::empty for the
    // trivial answer.
    static D_CONSTEXPR bool
    empty()
    D_NOEXCEPT
    {
        return true;
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<empty_view<_Type>>
//   trait: empty_view is a borrowed_range. The (null) iterators
// remain valid past the empty_view's lifetime trivially.
template<typename _Type>
struct enable_borrowed_range<empty_view<_Type> >
    : true_type
{};


// ===========================================================================
// III. VIEWS::EMPTY (colocated CPO-like helper)
// ===========================================================================

namespace views
{
    // views::empty<_Type>()
    //   function: returns an empty_view<_Type> instance. Function-
    // template form for portability across C++11+; the C++20
    // variable-template spelling 'views::empty<int>' is not provided
    // here because variable templates are C++14+ AND because the
    // value-initialised return is identical in cost.
    template<typename _Type>
    D_CONSTEXPR_INLINE
    empty_view<_Type>
    empty()
    D_NOEXCEPT
    {
        return empty_view<_Type>();
    }
}  // namespace views


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_EMPTY_VIEW_
