/***********************************************************************
* re_std                                        random_access_iterator_tag.hpp
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
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_
#define DJINTERP_RE_STD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_ 1

#include "djinterp.hpp"
#include "re_std/iterator/bidirectional_iterator_tag.hpp"


namespace re_std
{

struct random_access_iterator_tag : public bidirectional_iterator_tag
{
};


}  // namespace re_std

#endif  // DJINTERP_RE_STD_ITERATOR_RANDOM_ACCESS_ITERATOR_TAG_
