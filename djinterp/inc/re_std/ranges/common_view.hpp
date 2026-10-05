/*******************************************************************************
* djinterp [re_std]                                              common_view.hpp
*
* common_view view header:
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
*
* path:      /inc/re_std/ranges/common_view.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_RANGES_COMMON_VIEW_HPP
#define RE_STD_RANGES_COMMON_VIEW_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../iterator/common_iterator.hpp"
#include "./view_interface.hpp"
#include "./ranges_access.hpp"
#include "./iterator_t.hpp"
#include "./sentinel_t.hpp"

namespace re_std
{
namespace ranges
{

// common_view
//   class: a view whose begin() and end() have the same type.
template<typename View>
class common_view : public view_interface<common_view<View> >
{
    View m_base;

public:
    typedef common_iterator<iterator_t<View>, sentinel_t<View> > iterator;

    common_view() : m_base() {}
    explicit common_view(View base) : m_base(static_cast<View&&>(base)) {}

    //   Both ends are the SAME type - that is the entire point.
    iterator begin() { return iterator(ranges::begin(m_base)); }
    iterator end()   { return iterator(ranges::end(m_base)); }
};

}  // ranges
}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_RANGES_COMMON_VIEW_HPP
