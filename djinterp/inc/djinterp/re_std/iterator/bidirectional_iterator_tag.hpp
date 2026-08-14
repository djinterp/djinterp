/***********************************************************************
* restd                                         bidirectional_iterator_tag.hpp
*
* tag for bidirectional iterators — multi-pass, both-direction
* iteration via -- as well as ++. Derives from forward_iterator_tag.
*
*
* path:      /inc/djinterp/re_std/iterator/bidirectional_iterator_tag.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_
#define RESTD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_ 1

#include "djinterp.hpp"
#include "restd/iterator/forward_iterator_tag.hpp"


namespace restd
{

struct bidirectional_iterator_tag : public forward_iterator_tag
{
};


}  // namespace restd

#endif  // RESTD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_
