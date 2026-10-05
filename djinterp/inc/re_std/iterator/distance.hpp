/*******************************************************************************
* djinterp [re_std]                                                 distance.hpp
*
* distance function header:
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
* path:      /inc/re_std/iterator/distance.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_DISTANCE_HPP
#define RE_STD_ITERATOR_DISTANCE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/iterator_traits.hpp"
#include "re_std/iterator/input_iterator_tag.hpp"
#include "re_std/iterator/random_access_iterator_tag.hpp"


namespace re_std
{
namespace internal
{

    template<typename It>
    RE_STD_CONSTEXPR typename iterator_traits<It>::difference_type
    distance_impl
    (
        It _first,
        It _last,
        random_access_iterator_tag
    )
    {
        return _last - _first;
    }

    template<typename It>
    RE_STD_CONSTEXPR_CPP14 typename iterator_traits<It>::difference_type
    distance_impl
    (
        It _first,
        It _last,
        input_iterator_tag
    )
    {
        typename iterator_traits<It>::difference_type _n = 0;
        for (; _first != _last; ++_first) ++_n;
        return _n;
    }

}  // internal
template<typename It>
RE_STD_CONSTEXPR typename iterator_traits<It>::difference_type
distance(It _first, It _last)
{
    return internal::distance_impl
    (
        _first,
        _last,
        typename iterator_traits<It>::iterator_category()
    );
}


}  // re_std
#endif  // RE_STD_ITERATOR_DISTANCE_HPP
