/******************************************************************************
* djinterp [restd]                                             owning_view.hpp
*
* owning_view header:
*   Provides the C++20 ownership-wrapping range adaptor. owning_view<R>
* holds a moved-in _Range by value and forwards begin / end / size /
* empty / data to it, presenting a view over a range whose storage it
* owns. Used by views::all when the source range is a movable rvalue
* non-view.
*
*   PORTABILITY:
*   - Requires CRTP + view_interface + rvalue references + ref-
*     qualified member functions, available C++11+.
*   - Move-only by design — the copy ctor and copy assignment
*     operator are deleted. The C++20 contract makes copyability
*     conditional on whether _Range is copyable; restd takes the
*     conservative deletion to avoid surprising silent copies of
*     potentially expensive ranges.
*   - enable_borrowed_range<owning_view<R>> inherits from
*     enable_borrowed_range<R>: owning_view is borrowed only when
*     the underlying _Range is itself a borrowed_range.
*
*
* path:      /inc/djinterp/restd/ranges/owning_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_RANGES_OWNING_VIEW_
#define DJINTERP_RESTD_RANGES_OWNING_VIEW_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/begin.hpp"
#include "../iterator/end.hpp"
#include "../iterator/size.hpp"
#include "../iterator/empty.hpp"
#include "../iterator/data.hpp"
#include "./view_interface.hpp"
#include "./enable_borrowed_range.hpp"


NS_RESTD


// ===========================================================================
// I.   OWNING_VIEW
// ===========================================================================

// owning_view<_Range>
//   class: view that owns the underlying _Range by value. Moved-in
// at construction. Move-only.
template<typename _Range>
class owning_view : public view_interface<owning_view<_Range> >
{
private:
    _Range m_range;


public:
    // default ctor
    //   function: requires _Range to be default-constructible.
    D_CONSTEXPR
    owning_view()
        : m_range()
    {}

    // value ctor (move)
    //   function: takes ownership of _r by moving it in. There is
    // no lvalue overload — owning_view is the destination for
    // ranges that need ownership transferred.
    D_CONSTEXPR
    owning_view(
        _Range&& _r
    )
        : m_range(static_cast<_Range&&>(_r))
    {}

    // move ctor / move assign
    //   function: defaulted; transfers ownership of the held range.
    owning_view(owning_view&&) = default;
    owning_view& operator=(owning_view&&) = default;

    // copy ctor / copy assign — deleted (move-only).
    owning_view(owning_view const&) = delete;
    owning_view& operator=(owning_view const&) = delete;


    // base (mutable lvalue)
    //   function: returns a reference to the held range. ref-
    // qualified to provide accurate value categories.
    D_CONSTEXPR _Range&
    base() &
    D_NOEXCEPT
    {
        return m_range;
    }

    // base (const lvalue)
    D_CONSTEXPR _Range const&
    base() const&
    D_NOEXCEPT
    {
        return m_range;
    }

    // base (rvalue)
    //   function: returns an rvalue reference suitable for moving
    // the underlying range out of an expiring owning_view.
    D_CONSTEXPR _Range&&
    base() &&
    D_NOEXCEPT
    {
        return static_cast<_Range&&>(m_range);
    }

    // base (const rvalue) — kept for completeness; rarely useful.
    D_CONSTEXPR _Range const&&
    base() const&&
    D_NOEXCEPT
    {
        return static_cast<_Range const&&>(m_range);
    }


    // begin / end — forward to the underlying range. Both lvalue
    // and const-lvalue overloads provided.
    D_CONSTEXPR
    auto
    begin()
        -> decltype(restd::begin(m_range))
    {
        return restd::begin(m_range);
    }

    D_CONSTEXPR
    auto
    begin() const
        -> decltype(restd::begin(m_range))
    {
        return restd::begin(m_range);
    }

    D_CONSTEXPR
    auto
    end()
        -> decltype(restd::end(m_range))
    {
        return restd::end(m_range);
    }

    D_CONSTEXPR
    auto
    end() const
        -> decltype(restd::end(m_range))
    {
        return restd::end(m_range);
    }


    // empty / size / data — forward; SFINAE on the underlying.
    D_CONSTEXPR
    auto
    empty()
        -> decltype(restd::empty(m_range))
    {
        return restd::empty(m_range);
    }

    D_CONSTEXPR
    auto
    empty() const
        -> decltype(restd::empty(m_range))
    {
        return restd::empty(m_range);
    }

    D_CONSTEXPR
    auto
    size()
        -> decltype(restd::size(m_range))
    {
        return restd::size(m_range);
    }

    D_CONSTEXPR
    auto
    size() const
        -> decltype(restd::size(m_range))
    {
        return restd::size(m_range);
    }

    D_CONSTEXPR
    auto
    data()
        -> decltype(restd::data(m_range))
    {
        return restd::data(m_range);
    }

    D_CONSTEXPR
    auto
    data() const
        -> decltype(restd::data(m_range))
    {
        return restd::data(m_range);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<owning_view<_Range>>
//   trait: borrowed iff the underlying _Range is itself borrowed.
// This is the correct conditional opt-in — most owning_view
// instances are NOT borrowed (they own their storage), but when the
// underlying _Range happens to be borrowed (e.g. a subrange of
// pointers into separately-owned storage), the owning_view inherits
// that property.
template<typename _Range>
struct enable_borrowed_range<owning_view<_Range> >
    : enable_borrowed_range<_Range>
{};


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_RANGES_OWNING_VIEW_
