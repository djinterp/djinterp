/******************************************************************************
* re_std [ranges]                                                common_view.hpp
*
*   common_view - presents a range whose sentinel type differs from its
* iterator type as one where they match.
*
*   WHY IT IS NEEDED.  Every pre-C++20 algorithm takes two iterators of the
* SAME type.  A C++20 range may end at a sentinel of a different type - which
* is more efficient, since the sentinel can be an empty object testing a null
* terminator - and is therefore unusable with any of them.  common_view wraps
* both ends in common_iterator so the pair matches again.
*
*   IT IS NOT FREE, and that is why it is opt-in rather than automatic.  Every
* dereference and increment goes through common_iterator's tagged union, so
* the iterator is larger and each operation carries a branch.  Applying it to
* a range that is ALREADY common would pay that cost for nothing, which is why
* std requires the input not to be a common_range and why callers should reach
* for it only at the boundary with legacy code.
*
*   STD IS C++20; re_std IS C++11 - it needs only common_iterator, which
* re_std shipped at C++11 on 2026-08-13.
*
*   INTERFACE ASSUMPTIONS: see ADAPTOR_ASSUMPTIONS.txt in this directory.
*
* path:      /inc/djinterp/re_std/ranges/common_view.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_RANGES_COMMON_VIEW_
#define RESTD_RANGES_COMMON_VIEW_ 1

#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/common_iterator.hpp"
#include "./range_traits.hpp"
#include "./range_access.hpp"
#include "./view_interface.hpp"

NS_DJINTERP
NS_RESTD
D_NAMESPACE(ranges)

// common_view
//   class: a view whose begin() and end() have the same type.
template<typename _View>
class common_view : public view_interface<common_view<_View> >
{
    _View m_base;

public:
    typedef common_iterator<iterator_t<_View>, sentinel_t<_View> > iterator;

    common_view() : m_base() {}
    explicit common_view(_View base) : m_base(static_cast<_View&&>(base)) {}

    //   Both ends are the SAME type - that is the entire point.
    iterator begin() { return iterator(ranges::begin(m_base)); }
    iterator end()   { return iterator(ranges::end(m_base)); }
};

NS_END  // ranges
NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_RANGES_COMMON_VIEW_
