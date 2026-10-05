/*******************************************************************************
* djinterp [re_std]                                              owning_view.hpp
*
* owning_view header:
*   Provides the C++20 ownership-wrapping range adaptor. owning_view<R>
* holds a moved-in Range by value and forwards begin / end / size /
* empty / data to it, presenting a view over a range whose storage it
* owns. Used by views::all when the source range is a movable rvalue
* non-view.
*
*   PORTABILITY:
*   - Requires CRTP + view_interface + rvalue references + ref-
*     qualified member functions, available C++11+.
*   - Move-only by design — the copy ctor and copy assignment
*     operator are deleted. The C++20 contract makes copyability
*     conditional on whether Range is copyable; re_std takes the
*     conservative deletion to avoid surprising silent copies of
*     potentially expensive ranges.
*   - enable_borrowed_range<owning_view<R>> inherits from
*     enable_borrowed_range<R>: owning_view is borrowed only when
*     the underlying Range is itself a borrowed_range.
*
*
* path:      /inc/re_std/ranges/owning_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_OWNING_VIEW_HPP
#define RE_STD_RANGES_OWNING_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../iterator/begin.hpp"
#include "../iterator/end.hpp"
#include "../iterator/size.hpp"
#include "../iterator/empty.hpp"
#include "../iterator/data.hpp"
#include "./view_interface.hpp"
#include "./enable_borrowed_range.hpp"


namespace re_std
{


// ===========================================================================
// I.   OWNING_VIEW
// ===========================================================================

// owning_view<Range>
//   class: view that owns the underlying Range by value. Moved-in
// at construction. Move-only.
template<typename Range>
class owning_view : public view_interface<owning_view<Range> >
{
private:
    Range m_range;


public:
    // default ctor
    //   function: requires Range to be default-constructible.
    RE_STD_CONSTEXPR
    owning_view()
        : m_range()
    {}

    // value ctor (move)
    //   function: takes ownership of _r by moving it in. There is
    // no lvalue overload — owning_view is the destination for
    // ranges that need ownership transferred.
    RE_STD_CONSTEXPR
    owning_view(
        Range&& _r
    )
        : m_range(static_cast<Range&&>(_r))
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
    RE_STD_CONSTEXPR_CPP14 Range&
    base() &
    RE_STD_NOEXCEPT
    {
        return m_range;
    }

    // base (const lvalue)
    RE_STD_CONSTEXPR Range const&
    base() const&
    RE_STD_NOEXCEPT
    {
        return m_range;
    }

    // base (rvalue)
    //   function: returns an rvalue reference suitable for moving
    // the underlying range out of an expiring owning_view.
    RE_STD_CONSTEXPR_CPP14 Range&&
    base() &&
    RE_STD_NOEXCEPT
    {
        return static_cast<Range&&>(m_range);
    }

    // base (const rvalue) — kept for completeness; rarely useful.
    RE_STD_CONSTEXPR Range const&&
    base() const&&
    RE_STD_NOEXCEPT
    {
        return static_cast<Range const&&>(m_range);
    }


    // begin / end — forward to the underlying range. Both lvalue
    // and const-lvalue overloads provided.
    RE_STD_CONSTEXPR_CPP14
    auto
    begin()
        -> decltype(re_std::begin(m_range))
    {
        return re_std::begin(m_range);
    }

    RE_STD_CONSTEXPR
    auto
    begin() const
        -> decltype(re_std::begin(m_range))
    {
        return re_std::begin(m_range);
    }

    RE_STD_CONSTEXPR_CPP14
    auto
    end()
        -> decltype(re_std::end(m_range))
    {
        return re_std::end(m_range);
    }

    RE_STD_CONSTEXPR
    auto
    end() const
        -> decltype(re_std::end(m_range))
    {
        return re_std::end(m_range);
    }


    // empty / size / data — forward; SFINAE on the underlying.
    RE_STD_CONSTEXPR_CPP14
    auto
    empty()
        -> decltype(re_std::empty(m_range))
    {
        return re_std::empty(m_range);
    }

    RE_STD_CONSTEXPR
    auto
    empty() const
        -> decltype(re_std::empty(m_range))
    {
        return re_std::empty(m_range);
    }

    RE_STD_CONSTEXPR_CPP14
    auto
    size()
        -> decltype(re_std::size(m_range))
    {
        return re_std::size(m_range);
    }

    RE_STD_CONSTEXPR
    auto
    size() const
        -> decltype(re_std::size(m_range))
    {
        return re_std::size(m_range);
    }

    RE_STD_CONSTEXPR_CPP14
    auto
    data()
        -> decltype(re_std::data(m_range))
    {
        return re_std::data(m_range);
    }

    RE_STD_CONSTEXPR
    auto
    data() const
        -> decltype(re_std::data(m_range))
    {
        return re_std::data(m_range);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<owning_view<Range>>
//   trait: borrowed iff the underlying Range is itself borrowed.
// This is the correct conditional opt-in — most owning_view
// instances are NOT borrowed (they own their storage), but when the
// underlying Range happens to be borrowed (e.g. a subrange of
// pointers into separately-owned storage), the owning_view inherits
// that property.
template<typename Range>
struct enable_borrowed_range<owning_view<Range> >
    : enable_borrowed_range<Range>
{};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_OWNING_VIEW_HPP
