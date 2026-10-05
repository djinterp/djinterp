/*******************************************************************************
* djinterp [re_std]                                                     prev.hpp
*
* prev function header:
* prev(_it, _n=1) returns a copy of _it stepped backward by _n
* positions. Requires bidirectional or random-access category.
*
* implemented as advance(_it, -_n).
*
* added in std C++11.
*
*
* path:      /inc/re_std/iterator/prev.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_PREV_HPP
#define RE_STD_ITERATOR_PREV_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/iterator/advance.hpp"


namespace re_std
{

template<typename It>
RE_STD_CONSTEXPR_CPP14 It prev
(
    It _it,
    typename iterator_traits<It>::difference_type _n = 1
)
{
    re_std::advance(_it, -_n);
    return _it;
}


}  // re_std
#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_PREV_HPP
