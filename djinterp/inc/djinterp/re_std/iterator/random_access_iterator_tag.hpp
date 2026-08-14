/***********************************************************************
* restd                                         random_access_iterator_tag.hpp
*
* tag for random-access iterators — O(1) jump, [], +/-/+=/-=, full
* relational ordering. Derives from bidirectional_iterator_tag.
*
* raw pointers carry this tag (via the iterator_traits raw-pointer
* specialisation).
*
*
* path:      /inc/djinterp/re_std/iterator/random_access_iterator_tag.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_
#define RESTD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_ 1

#include "djinterp.hpp"
#include "restd/iterator/bidirectional_iterator_tag.hpp"


namespace restd
{

struct random_access_iterator_tag : public bidirectional_iterator_tag
{
};


}  // namespace restd

#endif  // RESTD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_
