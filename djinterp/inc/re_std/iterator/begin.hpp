/*******************************************************************************
* djinterp [re_std]                                                    begin.hpp
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
* path:      /inc/re_std/iterator/begin.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_BEGIN_HPP
#define RE_STD_ITERATOR_BEGIN_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    // std
    #include <cstddef>                        // size_t
    // re_std
    #include "../initializer_list/begin.hpp"  // begin(initializer_list<E>)


namespace re_std
{

// 1. container with member begin(), non-const.
template<typename C>
RE_STD_CONSTEXPR auto begin(C& _c) -> decltype(_c.begin())
{
    return _c.begin();
}

// 1b. container with member begin(), const.
template<typename C>
RE_STD_CONSTEXPR auto begin(const C& _c) -> decltype(_c.begin())
{
    return _c.begin();
}

// 2. raw array.
template<typename T, std::size_t N>
RE_STD_CONSTEXPR T* begin(T (&_arr)[N]) RE_STD_NOEXCEPT
{
    return _arr;
}

// 3. initializer_list: defined once, in initializer_list/begin.hpp, where
//    std declares it (<initializer_list>), and included above.


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_BEGIN_HPP
