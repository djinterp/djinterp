/***********************************************************************
* restd                                                               next.hpp
*
* next(_it, _n=1) returns a copy of _it advanced by _n positions.
* Convenience wrapper around advance() that takes the iterator by
* value and returns the result.
*
* added in std C++11.
*
*
* path:      /inc/restd/iterator/next.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_NEXT_
#define RESTD_ITERATOR_NEXT_ 1

#include "djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

    #include "restd/iterator/iterator_traits.hpp"
    #include "restd/iterator/advance.hpp"


namespace restd
{

template<typename _It>
D_CONSTEXPR _It next
(
    _It _it,
    typename iterator_traits<_It>::difference_type _n = 1
)
{
    restd::advance(_it, _n);
    return _it;
}


}  // namespace restd

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // RESTD_ITERATOR_NEXT_
