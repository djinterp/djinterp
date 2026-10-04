/***********************************************************************
* re_std                                              forward_iterator_tag.hpp
*
* tag for forward iterators — multi-pass, single-direction iteration.
* Derives from input_iterator_tag, so any algorithm taking input
* iterators by tag dispatch will also accept forward iterators.
*
*
* path:      /inc/djinterp/re_std/iterator/forward_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_FORWARD_ITERATOR_TAG_
#define DJINTERP_RE_STD_ITERATOR_FORWARD_ITERATOR_TAG_ 1

#include "djinterp.hpp"
#include "re_std/iterator/input_iterator_tag.hpp"


namespace re_std
{

struct forward_iterator_tag : public input_iterator_tag
{
};


}  // namespace re_std

#endif  // DJINTERP_RE_STD_ITERATOR_FORWARD_ITERATOR_TAG_
