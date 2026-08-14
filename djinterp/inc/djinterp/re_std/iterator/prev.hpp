/***********************************************************************
* restd                                                               prev.hpp
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
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_PREV_
#define RESTD_ITERATOR_PREV_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/iterator_traits.hpp"
    #include "restd/iterator/advance.hpp"


namespace restd
{

template<typename _It>
D_CONSTEXPR _It prev
(
    _It _it,
    typename iterator_traits<_It>::difference_type _n = 1
)
{
    restd::advance(_it, -_n);
    return _it;
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_PREV_
