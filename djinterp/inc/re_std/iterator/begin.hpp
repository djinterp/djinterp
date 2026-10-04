/***********************************************************************
* re_std                                                             begin.hpp
*
* free-function begin(container) - the canonical way to start a range
* iteration in generic code. Three overload categories:
*
*   1. container with a member begin()       - delegates to c.begin()
*      (both const and non-const overloads)
*   2. raw array T(&)[N]                     - returns &arr[0]
*   3. initializer_list<E>                   - returns il.begin()
*
* generic algorithms should always use re_std::begin(c) rather than
* c.begin(), because (a) it handles arrays, and (b) ADL picks up
* user-defined begin overloads for types that don't have a member.
*
* the standard idiom for ADL-aware code:
*
*   using re_std::begin;
*   auto it = begin(c);
*
* added in std C++11; size_t-based array overload existed earlier.
*
*
* path:      /inc/djinterp/re_std/iterator/begin.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_BEGIN_
#define DJINTERP_RE_STD_ITERATOR_BEGIN_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include <cstddef>
    #include <initializer_list>


namespace re_std
{

// 1. container with member begin(), non-const.
template<typename _C>
D_CONSTEXPR auto begin(_C& _c) -> decltype(_c.begin())
{
    return _c.begin();
}

// 1b. container with member begin(), const.
template<typename _C>
D_CONSTEXPR auto begin(const _C& _c) -> decltype(_c.begin())
{
    return _c.begin();
}

// 2. raw array.
template<typename _T, std::size_t _N>
D_CONSTEXPR _T* begin(_T (&_arr)[_N]) D_NOEXCEPT
{
    return _arr;
}

// 3. initializer_list.
template<typename _E>
D_CONSTEXPR const _E* begin(std::initializer_list<_E> _il) D_NOEXCEPT
{
    return _il.begin();
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_BEGIN_
