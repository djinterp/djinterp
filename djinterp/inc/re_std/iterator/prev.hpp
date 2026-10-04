/***********************************************************************
* re_std                                                              prev.hpp
*
* prev(_it, _n=1) returns a copy of _it stepped backward by _n
* positions. Requires bidirectional or random-access category.
*
* implemented as advance(_it, -_n).
*
* added in std C++11.
*
*
* path:      /inc/djinterp/re_std/iterator/prev.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_PREV_
#define DJINTERP_RE_STD_ITERATOR_PREV_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "re_std/iterator/iterator_traits.hpp"
    #include "re_std/iterator/advance.hpp"


namespace re_std
{

template<typename _It>
D_CONSTEXPR _It prev
(
    _It _it,
    typename iterator_traits<_It>::difference_type _n = 1
)
{
    re_std::advance(_it, -_n);
    return _it;
}


}  // namespace re_std

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_PREV_
