/*******************************************************************************
* djinterp [re_std]                                                 ref_view.hpp
*
* ref_view header:
*   Provides the C++20 reference-wrapping range adaptor. ref_view<R>
* holds a pointer to an external Range and forwards begin / end /
* size / empty / data to it, presenting a view over an unowned range.
* Used by views::all when the source range is an lvalue non-view.
*
*   PORTABILITY:
*   - Requires CRTP + view_interface + trailing return types,
*     available C++11+.
*   - Explicitly deletes the rvalue-Range constructor (a ref_view
*     bound to a temporary would dangle as soon as the constructor
*     returned). Matches the C++20 contract.
*   - Specialises enable_borrowed_range<ref_view<R>> to true. The
*     ref_view's iterators are the underlying range's iterators —
*     they remain valid regardless of the ref_view's lifetime
*     (the underlying range outlives the ref_view by construction).
*
*
* path:      /inc/re_std/ranges/ref_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_REF_VIEW_HPP
#define RE_STD_RANGES_REF_VIEW_HPP 1

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
// I.   REF_VIEW
// ===========================================================================

// ref_view<Range>
//   class: view that holds a pointer to an external Range. begin /
// end / size / etc. forward to the underlying range. Construction
// from an rvalue Range is deleted.
template<typename Range>
class ref_view : public view_interface<ref_view<Range> >
{
private:
    Range* m_range;


public:
    // value ctor (lvalue ref)
    //   function: stores the address of _r. The ref_view's
    // lifetime must not exceed _r's.
    RE_STD_CONSTEXPR
    ref_view(
        Range& _r
    )
    RE_STD_NOEXCEPT
        : m_range(&_r)
    {}

    // value ctor (rvalue ref)
    //   function: explicitly deleted. Prevents binding a ref_view
    // to a temporary.
    ref_view(
        Range&&
    ) = delete;


    // base
    //   function: returns the underlying range by reference.
    RE_STD_CONSTEXPR Range&
    base() const
    RE_STD_NOEXCEPT
    {
        return *m_range;
    }


    // begin / end — forward to the underlying range's begin / end.
    RE_STD_CONSTEXPR
    auto
    begin() const
        -> decltype(re_std::begin(*m_range))
    {
        return re_std::begin(*m_range);
    }

    RE_STD_CONSTEXPR
    auto
    end() const
        -> decltype(re_std::end(*m_range))
    {
        return re_std::end(*m_range);
    }


    // empty / size / data — forward to the underlying range when
    // each operation is well-formed. SFINAE via trailing return
    // type so non-applicable members instantiate only at call.
    RE_STD_CONSTEXPR
    auto
    empty() const
        -> decltype(re_std::empty(*m_range))
    {
        return re_std::empty(*m_range);
    }

    RE_STD_CONSTEXPR
    auto
    size() const
        -> decltype(re_std::size(*m_range))
    {
        return re_std::size(*m_range);
    }

    RE_STD_CONSTEXPR
    auto
    data() const
        -> decltype(re_std::data(*m_range))
    {
        return re_std::data(*m_range);
    }
};


// ===========================================================================
// II.  ENABLE_BORROWED_RANGE OPT-IN
// ===========================================================================

// enable_borrowed_range<ref_view<Range>>
//   trait: ref_view is a borrowed_range. Its iterators are the
// underlying range's iterators, which by construction live in
// storage that outlives the ref_view.
template<typename Range>
struct enable_borrowed_range<ref_view<Range> >
    : true_type
{};


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_REF_VIEW_HPP
