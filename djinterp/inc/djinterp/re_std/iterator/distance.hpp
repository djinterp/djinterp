/***********************************************************************
* restd                                                           distance.hpp
*
* distance(_first, _last) returns the number of increments needed to
* go from _first to _last.
*
* complexity:
*   O(1) for random-access iterators (subtraction).
*   O(distance) otherwise (count via ++).
*
* preconditions:
*   For input/forward iterators, _last must be reachable from _first.
*   For random-access iterators, no reachability requirement —
*   subtraction works regardless.
*
* added in std C++98; constexpr in C++17.
*
*
* path:      /inc/djinterp/re_std/iterator/distance.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_DISTANCE_
#define RESTD_ITERATOR_DISTANCE_ 1

#include "djinterp.hpp"

#include "restd/iterator/iterator_traits.hpp"
#include "restd/iterator/input_iterator_tag.hpp"
#include "restd/iterator/random_access_iterator_tag.hpp"


namespace restd
{
namespace internal
{

    template<typename _It>
    D_CONSTEXPR typename iterator_traits<_It>::difference_type
    distance_impl
    (
        _It _first,
        _It _last,
        random_access_iterator_tag
    )
    {
        return _last - _first;
    }

    template<typename _It>
    D_CONSTEXPR typename iterator_traits<_It>::difference_type
    distance_impl
    (
        _It _first,
        _It _last,
        input_iterator_tag
    )
    {
        typename iterator_traits<_It>::difference_type _n = 0;
        for (; _first != _last; ++_first) ++_n;
        return _n;
    }

}  // namespace internal


template<typename _It>
D_CONSTEXPR typename iterator_traits<_It>::difference_type
distance(_It _first, _It _last)
{
    return internal::distance_impl
    (
        _first,
        _last,
        typename iterator_traits<_It>::iterator_category()
    );
}


}  // namespace restd

#endif  // RESTD_ITERATOR_DISTANCE_
