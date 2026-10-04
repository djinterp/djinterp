/***********************************************************************
* re_std                                               output_iterator_tag.hpp
*
* tag for output iterators — single-pass, write-only iteration.
* Standalone in the hierarchy: output_iterator_tag does not derive
* from input_iterator_tag, and forward_iterator_tag does not derive
* from output_iterator_tag.
*
* a forward (or stronger) iterator can act as an output iterator
* through usage, but the TAG hierarchy does not encode that — code
* that needs "this iterator can write" should check both branches
* of the hierarchy independently when necessary.
*
*
* path:      /inc/djinterp/re_std/iterator/output_iterator_tag.hpp
* link(s):   TBA
* author(s): re_std contributors                         date: 2026.05.08
***********************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_OUTPUT_ITERATOR_TAG_
#define DJINTERP_RE_STD_ITERATOR_OUTPUT_ITERATOR_TAG_ 1

#include "djinterp.hpp"


namespace re_std
{

struct output_iterator_tag
{
};


}  // namespace re_std

#endif  // DJINTERP_RE_STD_ITERATOR_OUTPUT_ITERATOR_TAG_
