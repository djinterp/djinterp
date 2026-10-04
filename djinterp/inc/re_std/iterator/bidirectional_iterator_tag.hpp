/***********************************************************************
* re_std                                        bidirectional_iterator_tag.hpp
*
* tag for bidirectional iterators — multi-pass, both-direction
* iteration via -- as well as ++. Derives from forward_iterator_tag.
*
*
* path:      /inc/djinterp/re_std/iterator/bidirectional_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_
#define DJINTERP_RE_STD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_ 1

#include "djinterp.hpp"
#include "re_std/iterator/forward_iterator_tag.hpp"


namespace re_std
{

struct bidirectional_iterator_tag : public forward_iterator_tag
{
};


}  // namespace re_std

#endif  // DJINTERP_RE_STD_ITERATOR_BIDIRECTIONAL_ITERATOR_TAG_
