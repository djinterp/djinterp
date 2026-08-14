/***********************************************************************
* restd                                               forward_iterator_tag.hpp
*
* tag for forward iterators — multi-pass, single-direction iteration.
* Derives from input_iterator_tag, so any algorithm taking input
* iterators by tag dispatch will also accept forward iterators.
*
*
* path:      /inc/djinterp/re_std/iterator/forward_iterator_tag.hpp
* link(s):   TBA
* author(s): restd contributors                          date: 2026.05.08
***********************************************************************/

#ifndef RESTD_ITERATOR_FORWARD_ITERATOR_TAG_
#define RESTD_ITERATOR_FORWARD_ITERATOR_TAG_ 1

#include "djinterp.hpp"
#include "restd/iterator/input_iterator_tag.hpp"


namespace restd
{

struct forward_iterator_tag : public input_iterator_tag
{
};


}  // namespace restd

#endif  // RESTD_ITERATOR_FORWARD_ITERATOR_TAG_
