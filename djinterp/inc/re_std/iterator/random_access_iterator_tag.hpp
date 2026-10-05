/*******************************************************************************
* djinterp [re_std]                               random_access_iterator_tag.hpp
*
* random_access_iterator_tag class header:
* tag for random-access iterators — O(1) jump, [], +/-/+=/-=, full
* relational ordering. Derives from bidirectional_iterator_tag.
*
* raw pointers carry this tag (via the iterator_traits raw-pointer
* specialisation).
*
*
* path:      /inc/re_std/iterator/random_access_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                             created: 2026.05.08
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_HPP
#define RE_STD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "re_std/iterator/bidirectional_iterator_tag.hpp"


namespace re_std
{

struct random_access_iterator_tag : public bidirectional_iterator_tag
{
};


}  // re_std
#endif  // RE_STD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_HPP
